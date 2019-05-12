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
    int hour;
    int minute;
    int second;
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

void fullQueue(){           //prints the readyQueue in full
    int a = 0;
    while(a<lines){
        printf("%d %d \n", rq[a].task, rq[a].burstTime);
        a++;
    }
}

void add(int taskNo, int cpuBurst) {                //adds items to the ready queue

      if(rear == m-1) {
         rear = -1; 
      } 
      
      ++rear;
      rq[rear].task = taskNo;
      rq[rear].burstTime = cpuBurst;

            time_t T= time(NULL);
            struct  tm tm = *localtime(&T);
      items++;

      rq[rear].hour = tm.tm_hour;
      rq[rear].minute = tm.tm_min;
      rq[rear].second = tm.tm_sec;
      
      //printf("CPU ID :%d CPU B :%d m %d items %d time: %02d:%02d:%02d ?= time: %02d:%02d:%02d\n",taskNo,cpuBurst, m, items, tm.tm_hour, tm.tm_min, tm.tm_sec, rq[rear].hour, rq[rear].minute, rq[rear].second);
        
}

int * remTask() {                                         //removes items from the ready queue
   static int qTask[5];                                   //C creates a warning when returning the address of a local variable outside the function
   front++;
   qTask[0] = rq[front].task;
   qTask[1] = rq[front].burstTime; 
   qTask[2] = rq[rear].hour;
   qTask[3] = rq[rear].minute;
   qTask[4] = rq[rear].second;

   if(front == m) {
      front = 0;
   }
	
   items--;
   return qTask;  
}

int numOfLines(){           //not used because there was an issue with opening the same file twice

    FILE *fp;       //file pointer

    fp = fopen("task_file", "r");       //open file for reading
    
    char c;

    for(c = getc(fp); c != EOF; c=getc(fp)){            //loops through each and every character in the task_file
        if(c == '\n'){                                  //checks for new line
            lines++;
        }
    }

    fclose(fp);

    return lines;       //number of lines in the file
}

void loadfile(){

    tq = malloc(2*lines*sizeof(int));                   //memory allocation for tempQueue

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


// End of ready queue related code



//task fuction

void* task(void *arg1){
    int l; 

    FILE *fp1;
    fp1 = fopen("simulation_log", "a");         //append to simulation_log

    bool val;

    for(l=0; l<lines;){        
        val = isFull();

        if (!val){        
            pthread_mutex_lock(&mutex);                             //mutex lock
            add(tq[anchor].task,tq[anchor].burstTime);     //critical section
            l++;
            anchor++;  
            time_t T= time(NULL);
            struct  tm tm = *localtime(&T);
            pthread_mutex_unlock(&mutex); 

            fprintf(fp1,"%d %d %02d:%02d:%02d\n", tq[anchor+l].task,tq[anchor+l].burstTime, tm.tm_hour, tm.tm_min, tm.tm_sec);    //appends task number, burst time & executed time
        }
        else
        {
            pthread_mutex_lock(&mutex);
            pthread_cond_signal(&cond);             //      signals 
            printf("Task function waits\n");
            pthread_cond_wait(&cond1,&mutex);
            pthread_mutex_unlock(&mutex);
        }
                //anchor acts as the rear of tempQueue
    }   

    printf("Task Function Completed\n");

    fclose(fp1); 

    FILE *fp2;
    fp2 = fopen("simulation_log", "a");
    
    time_t T= time(NULL);
    struct  tm tm = *localtime(&T);

    fprintf(fp2,"\nNumber of tasks put into Ready-Queue %d", anchor);
    fprintf(fp2,"\nTerminate at time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    fclose(fp2); 

    pthread_exit(NULL);
}

void* cpu(void *args2){                 //cpu function
    
    int cpuNo = *(int*) args2;
    FILE *fp2;
    fp2 = fopen("simulation_log", "a");         //append to file

    
    int l =0;    

    while(1){
        pthread_mutex_lock(&mutex);
        if(!isEmpty()){
            
            int *t = remTask();             //critical section
            int burst_time = t[1];
            int task = t[0];
            int thour = t[2];
            int tminute = t[3];
            int tsecond = t[4];

            time_t T= time(NULL);                                   //cpu fetch time
            struct  tm tm = *localtime(&T);

            fprintf(fp2,"\nStatistics for CPU: %d", cpuNo);
            num_tasks++;
            
            fprintf(fp2,"\nTask %d", t[1]);

            fprintf(fp2,"\nArrival time: %02d:%02d:%02d\n", thour, tminute, tsecond);
            fprintf(fp2,"\nService time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

            total_waiting_time += ((60*60*(tm.tm_hour - thour)) + (60*(tm.tm_min - tminute)) + tm.tm_sec - tsecond);

            sleep(burst_time);

            time_t T1= time(NULL);                                   //process completion time
            struct  tm tm1 = *localtime(&T1);

            fprintf(fp2,"\nStatistics for CPU: %d", cpuNo);
            num_tasks++;
            
            fprintf(fp2,"\nTask %d", t[1]);

            fprintf(fp2,"\nArrival time: %02d:%02d:%02d\n", thour, tminute, tsecond);
            fprintf(fp2,"\nCompletion time: %02d:%02d:%02d\n", tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

            printf("\nTask %d Completed", t[1]);
            printf("\nArrival time: %02d:%02d:%02d\n", thour, tminute, tsecond);
            printf("\nService time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
            printf("\nCompletion time: %02d:%02d:%02d\n", tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
            pthread_mutex_unlock(&mutex);

        }else{
            printf("CPU %d Waits\n", cpuNo);
            pthread_cond_signal(&cond1);
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_unlock(&mutex);
        }

        //printf("anchor %d lines %d isEmpty %d\n", anchor, lines, isEmpty());
        if((anchor+1 == lines)&&(isEmpty())){
            printf("CPU %d exited", cpuNo);
            break;
        }
    }

    fclose(fp2);

    pthread_exit(NULL); 
}




int main(int args, char* argv[]){ 

    m = atoi(argv[2]);  //initializing m to the passed argument

    rq = malloc(m*sizeof*rq);   //memory allocation for readyQueue

    lines = 50;

    loadfile();     //loading whole file to the tempQueue

    //fullQueue();    

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

    printf("Total Waiting Time ");

    return 0;
}
