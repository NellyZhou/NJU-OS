#include "lib.h"
#include "types.h"
#define N 3



int main(void) {
	// TODO in lab4
	printf("reader_writer\n");
	
	sem_t WriteMutex, CountMutex;
	sem_init(&WriteMutex, 1);
	sem_init(&CountMutex, 1);
	int ret = 0;
	int Rcount = 0;
	write(SH_MEM, (uint8_t *)&Rcount, 4, 0);

	for (int i = 0; i < N; i++){
		//Writer
		ret = fork();
		if (ret == -1)
			exit();
		while (ret == 0){
			sem_wait(&WriteMutex);
			sleep(128);

			printf("Writer %d: write\n", i);
			sleep(128);
			
			sem_post(&WriteMutex);
			sleep(128);
		}

		//Reader
		ret = fork();
		if (ret == -1)
			exit();
		while (ret == 0){
			sem_wait(&CountMutex);
			sleep(128);
			read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			if (Rcount == 0){
				sem_wait(&WriteMutex);
				sleep(128);
			}
			Rcount++;
			write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			sleep(128);
			sem_post(&CountMutex);
			sleep(128);

			printf("Reader %d: read, total %d reader\n", i, Rcount);
			sleep(128);
			
			sem_wait(&CountMutex);
			sleep(128);
			read(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			Rcount--;
			write(SH_MEM, (uint8_t *)&Rcount, 4, 0);
			sleep(128);
			if (Rcount == 0){
				sem_post(&WriteMutex);
				sleep(128);
			}
			sem_post(&CountMutex);
			sleep(128);
		}
	}

	exit();
	return 0;
}
