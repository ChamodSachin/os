#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// code for ready queue. peek, isEmpty, isFull, size, add,remove funtions are created for the ready queue 


struct readyQueue{
    int task;
    int burstTime;
};

int m;
struct readyQueue *rq;   //global varriables for the ready queue
int tempQueue[2][100];
int front = 0;
int rear = -1;
int items = 0;
int anchor = 0;



//shared variables

int num_tasks = 0;
int total_waiting_time = 0;
int total_turnaround_time = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

int peek() {                                      //returns first task number of the ready queue
   return rq[front].task;
}

bool isEmpty() {                                  //checks whether the ready queue is empty
   return items == 0;
}

bool isFull() {                                     //checks whether the ready queue is full
   return items == m;
}

int size() {                                        //returns the size of the ready queue
   return items;
}  

void add(int taskNo, int cpuBurst) {                //adds items to the ready queue

      if(rear == m-1) {
         rear = -1; 
      } 
      
      ++rear;
      rq[rear].task = taskNo;
      rq[rear].burstTime = cpuBurst;
      items++;
}

int rem() {                                         //removes items from the ready queue
   int task = rq[front++].task;
	
   if(front == m) {
      front = 0;
   }
	
   items--;
   return task;  
}

int burst() {                                         //removes items from the ready queue
   int task = rq[front++].task;
	
   if(front == m) {
      front = 0;
   }
	
   items--;
   return task;  
}

void loadfile(){
    FILE *fp;       //file pointer

    fp = fopen("task_file", "r");       //open file for reading
    
    if (fp == NULL)                                                       //check for successfully opened file
        { 
            printf( "task_file failed to open." ) ; 
        } 
    else
        {           
            int i = 0;

            for(i=0; i<100; i++){
                fscanf(fp, "%d %d", &tempQueue[0][i], &tempQueue[1][i]);      //read data from task_file to the ready queue
            }
        }
     fclose(fp);
}

void fullQueue(){           //prints the readyQueue in full
    int a = 0;
    while(a<m){
        printf("%d %d \n", rq[a].task, rq[a].burstTime);
        a++;
    }
}

// End of ready queue related code

void* task(void *arg1){                //task fuction
    int l; 

    FILE *fp1;
    fp1 = fopen("simulation_log", "a");

    for(;;){
        pthread_mutex_lock(&mutex);                             //mutex lock
        
        if (!isFull()){        

        add(tempQueue[0][anchor+l],tempQueue[1][anchor+l]);     //critical section
        
        //printf("%d %d %d rear = %d items = %d \n", tempQueue[0][anchor+l],tempQueue[1][anchor+l], anchor+l, rear, items);              //remove

        time_t T= time(NULL);
        struct  tm tm = *localtime(&T);
        
        pthread_mutex_unlock(&mutex); 

        fprintf(fp1,"%d %d %02d:%02d:%02d\n", tempQueue[0][anchor+l], tempQueue[1][anchor+l], tm.tm_hour, tm.tm_min, tm.tm_sec);    //appends task number, burst time & executed time
        }
        else
        {
            
            pthread_mutex_lock(&mutex1);
            pthread_cond_wait(&cond1, &mutex1);                      //conditional wait for cpu process completion
            pthread_mutex_unlock(&mutex1);
        }
        anchor += m-size();
    }   

    fclose(fp1); 
    

    FILE *fp2;
    fp2 = fopen("simulation_log", "a");

    fprintf(fp2,"\nTask %d", rem());
    
    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);

    fprintf(fp2,"\nNumber of tasks put into Ready-Queue %d", m-size());
    fprintf(fp2,"\nTerminate at time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(fp2); 

    pthread_exit(NULL);
}

void* cpu(void *args2){                 //cpu function
    
    int cpuNo = *(int*) args2;
    FILE *fp2;
    fp2 = fopen("simulation_log", "a");

    pthread_mutex_lock(&mutex1);

    pthread_mutex_lock(&mutex1);
    int burst_time = burst();        //critical section
    sleep(burst_time);
    pthread_cond_signal(&cond1);    //conditional signal for cpu process completion
    pthread_mutex_unlock(&mutex1);

    
    pthread_cond_wait(&cond1, &mutex1);                      //conditional wait for cpu process completion
    pthread_mutex_unlock(&mutex1);

    fprintf(fp2,"\nStatistics for CPU: %d", cpuNo);
    fprintf(fp2,"\nTask %d", rem());
    
    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);

    fprintf(fp2,"\nArrival time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(fp2,"\nService time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(fp2); 
}




int main(int args, char* argv[]){ 

    m = atoi(argv[2]);

    rq = malloc(m*sizeof*rq);

    loadfile();

    printf("%d", size());

    pthread_t id;
    pthread_t id2;
    pthread_t id3;
    pthread_t id4;

    
    //spawn threads
    pthread_create(&id, NULL, task, NULL);   
    
    int cpuNo = 1;
    pthread_create(&id2, NULL, cpu, &cpuNo);    
    
    int cpuNo2 = 2;
    pthread_create(&id3, NULL, cpu, &cpuNo2);
    
    int cpuNo3 = 3;
    pthread_create(&id4, NULL, cpu, &cpuNo3);

    //wait for threads to finish
    pthread_join(id,NULL);
    pthread_join(id2,NULL);
    pthread_join(id3,NULL);
    pthread_join(id4,NULL);

    return 0;
}