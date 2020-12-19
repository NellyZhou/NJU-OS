#include "lib.h"
#include "types.h"

int main(void) {
	// TODO in lab4
	printf("bounded_buffer\n");
	
	int n = 4;
	int ret = 0;
	int value = 2;
	int buffer = 3;

	sem_t full, empty, mutex;
	sem_init(&mutex, 1);
	sem_init(&full, 0);
	sem_init(&empty, buffer);
	
	for (int t = 0; t < n; t++){
		ret = fork();
		int i = value;
		while (ret == 0){
			sem_wait(&empty);
			sleep(128);
			sem_wait(&mutex);
			sleep(128);

			printf("Producer %d: produce\n", t);
			sleep(128);

			sem_post(&mutex);
			sleep(128);
			sem_post(&full);
			sleep(128);
			i--;
			if (i == 0) exit();
		}
		if (ret == -1){
			exit();
		}
	}	
	
	while (1){
		sem_wait(&full);
		sleep(128);
		sem_wait(&mutex);
		sleep(128);

		printf("Consumer : consume\n");
		sleep(128);

		sem_post(&mutex);
		sleep(128);
		sem_post(&empty);
		sleep(128);
	}
	exit();
	return 0;
}
