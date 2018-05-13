#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#include "profiler_data.h"

#define PROG_NAME "timer"

#define prog_fprint(fd, str, ...) \
	do { \
		fprintf(fd, PROG_NAME ": " str, ##__VA_ARGS__); \
	} while(0)

#define print(...) \
	prog_fprint(stdout, __VA_ARGS__)

#define print_error(...) \
	prog_fprint(stderr, __VA_ARGS__)

static struct __profiler_header * head = NULL;
static int shm_fd = -1;

static int create_shared_memory(char const * shm_name, size_t const mem_size) {
	int fd = open(shm_name, O_RDWR | O_CREAT, (mode_t)0600);
	if (fd == -1) {
		print_error("Could not open %s\n", shm_name);
		return 1;
	}
	size_t sz = lseek(fd, mem_size, SEEK_SET);
	if (sz == -1) {
		close(fd);
		print_error("Could not resize %s to %lu\n", shm_name, mem_size);
		return 2;
	}

	size_t written = write(fd, "", 1);
	if (written == -1) {
		close(fd);
		print_error("Could not write to file %s\n", shm_name);
		return 4;
	}
	
	void * ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		close(fd);
		print_error("Could not map %s\n", shm_name);
		return 3;
	}

	shm_fd = fd;
	head = (struct __profiler_header *)ptr;
	head->size = sz;
	return 0;
}

static void unmap_shared_memory() {
	if (head != NULL) {
		if (munmap(head, head->size)) {
			print_error("Could not unmap %p\n", head);
		}
		head = NULL;
	}
	if (shm_fd != -1) {
		close(shm_fd);
		shm_fd = -1;
	}
}

static pid_t start_other(char const * program, char ** args, char ** envp, int fd) {
	pid_t pid;
	if ((pid = fork()) == 0) {
		size_t sz = 0;
		while (envp[sz++] != NULL) {}
		char * env[sz + 1];
		memcpy(env, envp, sizeof(char **) * sz);
		char var[sizeof(PERF_ENV_SHM_VAR) + sizeof(fd) + 1];
		sprintf(var, PERF_ENV_SHM_VAR "=%d", fd);
		env[sz - 1] = var;
		env[sz] = NULL;
		execve(program, args, env);

		exit(127);
	}
	return pid;
}

#if defined(SOFTEXIT)
int softexit = 0;
#endif

static void * update_clock(void * ptr) {
	//struct timespec t;
	uint64_t sec = 0;
	uint64_t nsec = 0;
#if !defined(SOFTEXIT)
	for(;;) {
#else
	while(!softexit) {
#endif
		nsec++;
		if (nsec >= 1000000000) {
			sec++;
			nsec = 0;
		}
		//clock_gettime(CLOCK_MONOTONIC, &t);
		asm volatile (
			"lock cmpxchg16b %[ptr]\n"
			: [ptr] "=m" (head->sec),
			  "=m" (head->nsec)
			: "d" (head->nsec),
		 	  "a" (head->sec),
			  "c" (nsec),
			  "b" (sec)
		);
	}
}

void print_usage(char * app_name) {
	printf("Usage: %s [-h] log_filename size_in_byte app [app argument(s)]\n", app_name);
}

struct Args {
	char const * self_name;
	char const * shm_name;
	size_t shm_size;
	char const * app_name;
	char ** app_args;
};

struct Args parse_args(int argc, char ** argv) {
	for (size_t i = 0; i < argc; ++i) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argv[0]);
			printf("\n"
		       		"Starts to be profiled application and provides a monotonic clock to the application.\n"
		       		"It also writes the profiler values to the log file\n"
		       		"\n"
		       		"positional arguments:\n"
		       		"  log_filename          File to which the log should be written\n"
		       		"  size_in_byte          Size of the log file to be created\n"
    				"                        supports suffixes k,M,G,T,P,E\n"
		       		"  app                   Application to be started\n"
		       		"\n"
		       		"optional arguments:\n"
		       		"  -h, --help            show his help message and exit\n"
		       		"  app arguments         arguments which should be passed on the application\n");
			exit(0);
		}
	}
	if (argc < 3) {
		print_usage(argv[0]);
		exit((argc == 1) ? 0 : 1);
	}
	size_t size = 0;
	{
		size_t i = 0;
		char c;
		while((c = argv[2][i++])) {
			if (c >= '0' && c <= '9') {
				size = size * 10 + (c - '0');
				continue;
			}
			switch(c) {
				case 'k':	size <<= 10; break;
				case 'M':	size <<= 20; break;
				case 'G':	size <<= 30; break;
				case 'T':	size <<= 40; break;
				case 'P':	size <<= 50; break;
				case 'E':	size <<= 60; break;
				default: 	print_error("unsupported symbol: %c in the size parameter\n", c);
							exit(1);
			}
            break;
		}
	}
	struct Args res = {argv[0], argv[1], size, argv[3], argv + 3};
	return res;
}

int main(int argc, char ** argv, char ** envp) {
	struct Args args = parse_args(argc, argv);
	int ret;
	if ((ret = create_shared_memory(args.shm_name, args.shm_size))) {
		return ret;
	}

	pid_t child = start_other(args.app_name, args.app_args, envp, shm_fd);
 	pthread_t clock;
	if ((ret = pthread_create(&clock, NULL, update_clock, NULL)) != 0) {
		print_error("Could not create clock thread\n");
		return ret;
	}

	waitpid(child, &ret, 0);
	if (ret != 0) {
		print_error("Child exited with code: %d\n", ret);
	}
#if !defined(SOFTEXIT)
	//Let the OS clean up
	return 0;
#else
	softexit = 1;
	pthread_join(clock, NULL);
	unmap_shared_memory();
	return 0;
#endif
}
