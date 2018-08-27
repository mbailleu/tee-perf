#include <stdio.h>
#include <pthread.h>

void * start(void * ptr) {
    (void) ptr;
    printf("%d\n", pthread_self());
}

int main() {
    void * ptr;
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
