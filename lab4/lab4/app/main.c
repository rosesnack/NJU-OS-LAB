#include "lib.h"
#include "types.h"
void producer_customer() {
	int i = 0;
	int id = 0;
	sem_t empty, full, mutex;
	sem_init(&empty, 5);
	sem_init(&full, 0);
	sem_init(&mutex, 1);
	for (i = 0; i < 4; i++) {
		if (id == 0) id = fork();
		else if (id > 0) break;
	}
	while (1) {
		if (id == 0){
			sem_wait(&full);
			sem_wait(&mutex);
			printf("Consumer : consume\n");
			sleep(128);
			sem_post(&mutex);
			sem_post(&empty);
		}
		else{
			sem_wait(&empty);
			sem_wait(&mutex);
			printf("Producer %d: produce\n", id - 1);
			sleep(128);
			sem_post(&mutex);
			sem_post(&full);
		}
	}
}

void philosopher() {
	int id = 0;
	sem_t forks[5], mutex;
	sem_init(&mutex, 1);
	for (int i = 0; i < 4; i++) sem_init(&forks[i], 1);
	for (int i = 0; i < 4; i++) {
		if (id == 0) id = fork();
		else if (id > 0) break;
	}
	if (id > 0) id -= 1;
	while (1) {
		printf("Philosopher %d: think\n", id);
		sleep(128);
		if (id % 2 == 0) {
			sem_wait(&forks[id]);
			sem_wait(&forks[(id + 1) % 5]);
		}
		else {
			sem_wait(&forks[(id + 1) % 5]);
			sem_wait(&forks[id]);
		}
		printf("Philosopher %d: eat\n", id);
		sleep(128);
		sem_post(&forks[id]);
		sem_post(&forks[(id + 1) % 5]);
	}
}
int uEntry(void)
{
	// For lab4.1
	// Test 'scanf'
	int dec = 0;
	int hex = 0;
	char str[6];
	char cha = 0;
	int ret = 0;
	while (1)
	{
		printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
		ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
		printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
		if (ret == 4)
			break;
	}

	// For lab4.2
	// Test 'Semaphore'
	int i = 4;

	sem_t sem;
	printf("Mother Process: Semaphore Initializing.\n");
	ret = sem_init(&sem, 2);  //记得去掉int
	if (ret == -1)
	{
		printf("Mother Process: Semaphore Initializing Failed.\n");
		exit();
	}

	ret = fork();
	if (ret == 0)
	{
		while (i != 0)
		{
			i--;
			printf("Child Process: Semaphore Waiting.\n");
			sem_wait(&sem);
			printf("Child Process: In Critical Area.\n");
		}
		printf("Child Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		exit();
	}
	else if (ret != -1)
	{
		while (i != 0)
		{
			i--;
			printf("Mother Process: Sleeping.\n");
			sleep(128);
			printf("Mother Process: Semaphore Posting.\n");
			sem_post(&sem);
		}
		printf("Mother Process: Semaphore Destroying.\n");
		sem_destroy(&sem);
		//exit();
	}

	// For lab4.3
	// TODO: You need to design and test the problem.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.
	
	producer_customer();
	//philosopher();
	
	exit();

	return 0;
}
