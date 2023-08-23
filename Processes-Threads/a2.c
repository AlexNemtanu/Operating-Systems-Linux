#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <semaphore.h>
typedef struct
{
    int threadID;
    int processID;

}IDS;

pthread_cond_t condP2;
pthread_mutex_t mutexP2;
sem_t semaphoreP6;


int thread2_4;
int thread2_1;
pthread_mutex_t mutexP6, mutex1P6;
pthread_cond_t condP6, cond1P6, cond2P6;
sem_t semaphoreP6;
pthread_mutex_t mutexP5 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condP5 = PTHREAD_COND_INITIALIZER;
int threadCountP5 = 0;
int threadCount = 0;
int allThreadsCount = 0;
int waitForAll = 0;
int n_threads = 48;

void* p6(void* arg) {
    sem_wait(&semaphoreP6);
    int thread_id = (int)(long)arg;
    info(BEGIN, 6, thread_id);

    pthread_mutex_lock(&mutexP6);
    threadCount++;

    if (threadCount == 6) {
        waitForAll = 1;
        pthread_cond_broadcast(&condP6);
    }
    else {
        while (!waitForAll) {
            pthread_cond_wait(&condP6, &mutexP6);
        }
    }

    allThreadsCount++;
    if (allThreadsCount >= 43) {
        pthread_mutex_lock(&mutex1P6);
        if (waitForAll == 1) {
            pthread_cond_broadcast(&cond1P6);
            waitForAll = 2;
        }
        pthread_mutex_unlock(&mutex1P6);
    }

    info(END, 6, thread_id);
    threadCount--;
    pthread_mutex_unlock(&mutexP6);

    sem_post(&semaphoreP6);
    return NULL;
}

void* p2(void* arg) {
    IDS* data = (IDS*)arg;

    int threadID = data->threadID;
    int processID = data->processID;

    pthread_mutex_lock(&mutexP2);

    if (threadID == 1 && processID == 2) {
        while (!thread2_4) {
            pthread_cond_wait(&condP2, &mutexP2);
        }
    }

    if (threadID == 4 && processID == 2) {
        thread2_4 = 1;
        pthread_cond_broadcast(&condP2);
    }

    info(BEGIN, processID, threadID);

    if (threadID == 4 && processID == 2) {
        while (!thread2_1) {
            pthread_cond_wait(&condP2, &mutexP2);
        }
    }

    if (threadID == 1 && processID == 2) {
        thread2_1 = 1;
        pthread_cond_broadcast(&condP2);
    }

    info(END, processID, threadID);

    pthread_mutex_unlock(&mutexP2);

    return NULL;
}

void* p5(void* arg) {
    int thread_id = (int)(long)arg;
    info(BEGIN, 5, thread_id);

    pthread_mutex_lock(&mutexP5);
    threadCountP5++;

    if (threadCountP5 == 5) {
        pthread_cond_broadcast(&condP5);  
    } else {
        while (threadCountP5 < 5) {
            pthread_cond_wait(&condP5, &mutexP5);  
        }
    }

    info(END, 5, thread_id);
    pthread_mutex_unlock(&mutexP5);

    return NULL;
}

int main(int argc, char** argv) {
    init();
    pid_t pid2, pid5, pid7;
    pthread_t tidsP5[5];
    info(BEGIN, 1, 0);
    pid2 = fork();
    if (pid2 == 0) { // p2
        info(BEGIN, 2, 0);
        pthread_t tids[4];
        IDS threadData[4];
        pthread_mutex_t mutexP2 = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t condP2 = PTHREAD_COND_INITIALIZER;

        IDS* dataPtr = &threadData[0];

	for (int i = 0; i < 4; i++, dataPtr++) {
		dataPtr->processID = 2;
		dataPtr->threadID = (i + 1);
		pthread_create(&tids[i], NULL, p2, dataPtr);
}

        pthread_t* tidPtr = &tids[0];
	for (int i = 0; i < 4; i++, tidPtr++) {
		pthread_join(*tidPtr, NULL);
}

        pthread_cond_destroy(&condP2);
        pthread_mutex_destroy(&mutexP2);

        pid_t pid3 = fork();
        if (pid3 == 0) { // p3
            info(BEGIN, 3, 0);
            pid_t pid4 = fork();
            if (pid4 == 0) { // p4
                info(BEGIN, 4, 0);
                info(END, 4, 0);
            }
            else { // p6
                pid_t pid6 = fork();
                if (pid6 == 0) {
                    info(BEGIN, 6, 0);
                    pthread_t tids[n_threads];
                    pthread_cond_t condP6 = PTHREAD_COND_INITIALIZER;
                    pthread_cond_t cond1P6 = PTHREAD_COND_INITIALIZER;
                    pthread_cond_t cond2P6 = PTHREAD_COND_INITIALIZER;
                    pthread_mutex_t mutexP6 = PTHREAD_MUTEX_INITIALIZER;
                    pthread_mutex_t mutex1P6 = PTHREAD_MUTEX_INITIALIZER;
                    sem_init(&semaphoreP6, 0, 6);


                    for (int i = 0; i < n_threads; i++) {
                        int thread_id = i + 1;
                        pthread_create(&tids[i], NULL, p6, (void*)(long)thread_id);
                    }
                    for (int i = 0; i < n_threads; i++) {
                        pthread_join(tids[i], NULL);
                    }

                    sem_destroy(&semaphoreP6);
                    pthread_cond_destroy(&cond2P6);
                    pthread_cond_destroy(&cond1P6);
                    pthread_cond_destroy(&condP6);
                    pthread_mutex_destroy(&mutexP6);
                    pthread_mutex_destroy(&mutex1P6);


                    wait(NULL);
                    info(END, 6, 0);
                }
                else {
                    wait(NULL);
                    wait(NULL);
                    info(END, 3, 0);
                }
            }
        }
        else {
            wait(NULL);
            info(END, 2, 0);
        }
    }
    else { // p5 and p7
        pid5 = fork();
        if (pid5 == 0) { // p5
            info(BEGIN, 5, 0);
            	for (int i = 0; i < 5; i++) {
    			int thread_id = i + 1;
    			pthread_create(&tidsP5[i], NULL, p5, (void*)(long)thread_id);
}

		for (int i = 0; i < 5; i++) {
    			pthread_join(tidsP5[i], NULL);
}
            info(END, 5, 0);
        }
        else { // p7
            pid7 = fork();
            if (pid7 == 0) {
                info(BEGIN, 7, 0);
                info(END, 7, 0);
            }
            else { // p1
                wait(NULL);
                wait(NULL);
                wait(NULL);
                info(END, 1, 0);
            }
        }
    }

    return 0;
}
