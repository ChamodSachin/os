#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// code for ready queue. peek, isEmpty, isFull, add,remove funtions are created for the ready queue 

struct readyQueue{
    int task;
    int burstTime;
};

struct tempQueue{
    int task;
    int burstTime;
};

int m;
int lines = 0;
struct readyQueue *rq;   //global varriables for the ready queue
struct tempQueue *tq;
int front = 0;
int rear = -1;
int items = 0;
int anchor = 0;

//shared variables

int num_tasks = 0;
int total_waiting_time = 0;
int total_turnaround_time = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
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

void add(int taskNo, int cpuBurst) {                //adds items to the ready queue

      if(rear == m-1) {
         rear = -1; 
      } 
      
      ++rear;
      rq[rear].task = taskNo;
      rq[rear].burstTime = cpuBurst;
      items++;
}

int * remTask() {                                         //removes items from the ready queue
   static int qTask[2];                                   //C creates a warning when returning the address of a local variable outside the function
   front++;
   qTask[0] = rq[front].task;
   qTask[1] = rq[front].burstTime;    

   if(front == m) {
      front = 0;
   }
	
   items--;
   return qTask;  
}

int numOfLines(){

    FILE *fp;       //file pointer

    fp = fopen("task_file", "r");       //open file for reading
    
    char c;

    for(c = getc(fp); c != EOF; c=getc(fp)){            //loops through each and every character in the task_file
        if(c == '\n'){                                  //checks for new line
            lines++;
        }
    }

    fclose(fp);

    return lines;
}

void loadfile(){

    tq = malloc(2*lines*sizeof(int));                   //memory allocation for tempQueue
    

    printf("lines %d\n", lines);

    FILE *fp5;       //file pointer

    fp5 = fopen("task_file", "r");       //open file for reading

    if (fp5 == NULL)                                                       //check for successfully opened file
        { 
            printf( "task_file failed to open." ) ; 
        } 
    else
        {           
            int i = 0;

            for(i=0; i<lines; i++){
                fscanf(fp5, "%d %d", &tq[i].task, &tq[i].burstTime);      //read data from task_file to the tempQueue
            }
        }
     fclose(fp5);
}

void fullQueue(){           //prints the readyQueue in full
    int a = 0;
    while(a<lines){
        printf("%d %d \n", rq[a].task, rq[a].burstTime);
        a++;
    }
}

// End of ready queue related code



//task fuction

void* task(void *arg1){
    int l; 

    FILE *fp1;
    fp1 = fopen("simulation_log", "a");         //append to simulation_log

    bool val;

    for(;;){
        //printf("s\n");
        
        val = isFull();

        printf("%d %d %d\n", val, m, items);
        if (!val){        

        printf("s");
        pthread_mutex_lock(&mutex);                             //mutex lock
        printf("sss");
        add(tq[anchor+l].task,tq[anchor+l].burstTime);     //critical section
        time_t T= time(NULL);
        struct  tm tm = *localtime(&T);
        
        //pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex); 

        fprintf(fp1,"%d %d %02d:%02d:%02d\n", tq[anchor+l].task,tq[anchor+l].burstTime, tm.tm_hour, tm.tm_min, tm.tm_sec);    //appends task number, burst time & executed time
        }
        else
        {
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);
            printf("Task function waits\n");
            pthread_cond_wait(&cond1,&mutex);
            pthread_mutex_unlock(&mutex);
        }
        anchor += m-items;
    }   

    fclose(fp1); 
}

void printTask(int t){
    FILE *fp2;
    fp2 = fopen("simulation_log", "a");
    
    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);

    fprintf(fp2,"\nNumber of tasks put into Ready-Queue %d", m-items);
    fprintf(fp2,"\nTerminate at time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(fp2); 

    pthread_exit(NULL);
}

void* cpu(void *args2){                 //cpu function
    
    int cpuNo = *(int*) args2;
    FILE *fp2;
    fp2 = fopen("simulation_log", "a");         //append to file

    for(;;){
        pthread_mutex_lock(&mutex);
        int *t = remTask();
        int burst_time = t[1];        //critical section
        int task = t[0];
        sleep(burst_time);

        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);

        fprintf(fp2,"\nStatistics for CPU: %d", cpuNo);
        fprintf(fp2,"\nTask %d", t[1]);
    }
    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);

    fprintf(fp2,"\nArrival time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(fp2,"\nService time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(fp2);

    pthread_exit(NULL); 
}




int main(int args, char* argv[]){ 

    m = atoi(argv[2]);

    rq = malloc(m*sizeof*rq);

    numOfLines();
    loadfile();
fullQueue();
    pthread_t id;
    /*pthread_t id2;
    pthread_t id3;
    pthread_t id4;*/

    
    //spawn threads
    pthread_create(&id, NULL, task, NULL);   
    
    /*int cpuNo = 1;
    pthread_create(&id2, NULL, cpu, &cpuNo);    
    
    int cpuNo2 = 2;
    pthread_create(&id3, NULL, cpu, &cpuNo2);
    
    int cpuNo3 = 3;
    pthread_create(&id4, NULL, cpu, &cpuNo3);*/

    //wait for threads to finish
    pthread_join(id,NULL);
    /*pthread_join(id2,NULL);
    pthread_join(id3,NULL);
    pthread_join(id4,NULL);*/

    return 0;
}
