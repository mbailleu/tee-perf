#include <stdio.h>
#include <pthread.h>

void a() {
    for (int i = 0; i < 100; ++i) {
        printf("%lu\n", (uint64_t)pthread_self());
    }
}

void * start(void * ptr) {
    (void) ptr;
    a();
    return NULL;
}

int main() {
    void * ptr = NULL;
    __profiler_activate_trace();
    start(ptr);
    pthread_t childs[3];
    for (int i = 0; i < 3; ++i) {
        pthread_create(&childs[i], NULL, start, NULL);
    }
    for (int i = 0; i < 3; ++i) {
        pthread_join(childs[i], NULL);
    }
    start(ptr);
}
