#include "lib.h"
#include "types.h"
#define N 5

int main(void) {
	// TODO in lab4
	printf("philosopher\n");
	
	sem_t forki[N];
	for (int i = 0; i < N; i++)
		sem_init(&forki[i], 1);

	for (int i = 0; i < N;i++){
		int ret = fork();
		while (ret == 0){
			printf("Philosopher %d: think\n", i);
			sleep(128);
			if (i % 2 ==0){
				sem_wait(&forki[i]);
				sleep(128);
				sem_wait(&forki[(i + 1) % N]);
				sleep(128);
			} else {
				sem_wait(&forki[(i + 1) % N]);
				sleep(128);
				sem_wait(&forki[i]);
				sleep(128);
			}
			
			printf("Philosopher %d: eat\n", i);
			sleep(128);
			
			sem_post(&forki[i]);
			sleep(128);
			sem_post(&forki[(i + 1) % N]);
			sleep(128);
		}
		if (ret == -1){
			exit();
		}
	}	

	exit();
	return 0;
}
