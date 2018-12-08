#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t lock;
int global_x = 34;

void *thread(void *argp){
	pthread_mutex_lock(&lock);
	printf("thread id:[%d]\n", *(int *)argp);
	printf("global_x: %d\n", global_x);
	pthread_mutex_unlock(&lock);
}

int main(int argc, char const *argv[])
{
	pthread_mutex_init(&lock, NULL);
	pthread_t tid;
	pthread_create(&tid, NULL, thread, (void *)tid);
	pthread_join(tid, NULL);
	return 0;
}
