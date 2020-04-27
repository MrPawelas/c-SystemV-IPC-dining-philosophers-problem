#include<stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>


volatile int *eatCount;
const int philosphersCount=4;
const short thinking=0;
const short hungry=1;
const short eating=2;
const int timeOfThinking = 2; //seconds
const int timeOfEating = 3; //seconds
volatile short *philosphersStates;
int philosopherID,mutex,semaphores;

int leftPhilosopher(int id){
    return (id+philosphersCount-1)%philosphersCount;
}
int rightPhilosopher(int id){
    return (id+1)%philosphersCount;
}

void think(){
    printf("Philosopher number %d is thinking.\n", philosopherID);
    sleep(timeOfThinking);
}
void eat(){
    printf("Philosopher number %d is eating.\n", philosopherID);
    sleep(timeOfEating);
    eatCount[philosopherID]+=rand()%5+1;
}
void philosopherDoneSignal(int signal)
{
    printf("Philosopher number %d has eaten %d units of food\n", philosopherID,eatCount[philosopherID]);
    exit(0);

}

void testIfPhilospherCanEat(int id){
    if(philosphersStates[id]==hungry && philosphersStates[leftPhilosopher(id)]!=eating && philosphersStates[rightPhilosopher(id)]!=eating){
        philosphersStates[id]=eating;
        struct sembuf v = { id, +1, 0};
        semop(semaphores,&v,1);
    }
}

void takeForks(){
    struct sembuf sb = { 0, -1, 0};
    semop(mutex,&sb,1);    //critical Section starts
    philosphersStates[philosopherID]=hungry;
    testIfPhilospherCanEat(philosopherID);
    sb.sem_op=1;
    semop(mutex,&sb,1);     //critical Section ends
    sb.sem_op=-1; sb.sem_num=philosopherID;
    semop(semaphores,&sb,1); //philosopher cant grab forks, and waits until he will be able to

}

void putForks(){
    struct sembuf sb = { 0, -1, 0};
    semop(mutex,&sb,1);    //critical Section starts

    philosphersStates[philosopherID]=thinking;
    //now philosopher puts down both forks, but the order depends on the amount of food his neighbours had eaten.
    if(eatCount[leftPhilosopher(philosopherID)]<eatCount[rightPhilosopher(philosopherID)]){
        testIfPhilospherCanEat(leftPhilosopher(philosopherID));
        testIfPhilospherCanEat(rightPhilosopher(philosopherID));
    }else{
        testIfPhilospherCanEat(rightPhilosopher(philosopherID));
        testIfPhilospherCanEat(leftPhilosopher(philosopherID));
    }
    sb.sem_op=1;
    semop(mutex,&sb,1);     //critical Section ends
}
void philosopher(){
    eatCount[philosopherID]=0;
    signal(SIGINT, philosopherDoneSignal);
    while(1){
       think();
       takeForks();
       eat();
       putForks();
    }

}



int main() {
    int shmIDStates,smhIDEatingCounter;


    shmIDStates = shmget(IPC_PRIVATE, sizeof(short)*philosphersCount, IPC_CREAT | 0777);
    philosphersStates=(short*) shmat(shmIDStates,NULL,0);

    smhIDEatingCounter = shmget(IPC_PRIVATE, sizeof(int)*philosphersCount, IPC_CREAT | 0777);
    eatCount=(int*) shmat(smhIDEatingCounter,NULL,0);

    semaphores = semget(IPC_PRIVATE,  philosphersCount, IPC_CREAT | 0777);
    for (int i = 0; i < philosphersCount; i++)
        semctl(semaphores,i,SETVAL,0); //philosophers' semaphores

    mutex = semget(IPC_PRIVATE,  1, IPC_CREAT | 0777);

    semctl(mutex,0,SETVAL,1); // mutex providing atomicity

    for (int i = 0; i < philosphersCount; i++)
        philosphersStates[i]=thinking;

    int pid;
    philosopherID=0;

    while(philosopherID<philosphersCount){
        pid=fork();
        if(pid==0) {
            break;
        }
        philosopherID++;
    }

    if(pid==0) {
        printf("created philosopher number: %d\n", philosopherID);
        philosopher();
    }
    if(pid!=0)
        wait(NULL);
    return 0;
}
