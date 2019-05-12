#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// code for ready queue. peek, isEmpty, isFull, add,remove funtions are created for the ready queue 

//ReadyQueue acts as a queue but is actually a structure with five elements in it. Every function regarding the ready queue is done on this structure

struct readyQueue{
    int task;
    int burstTime;
    int hour;
    int minute;
    int second;
};


//tempQueue is used to load all data from the task_file
struct tempQueue{
    int task;
    int burstTime;
};

//global varriables for the ready queue
int m;
int lines = 0;				//number of lines in the task_file is assigned to this. However there was an issue with loading the same file twice so, I manually setup this variable at the scheduler initialization
struct readyQueue *rq;   	//readyQueue object pointer
struct tempQueue *tq;		//tempQueue object pointer
int front = 0;				//front of the readyQueue
int rear = -1;				//rear of the readyQueue
int items = 0;				//number of items in the readyQueue
int anchor = 0;				//acts as the front of the tempQueue
int comp = 0;				//used for broadcasting tasks thread status for all three cpus

//shared variables
int num_tasks = 0;
int total_waiting_time = 0;
int total_turnaround_time = 0;

//mutex and cond
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

      if(rear == m-1) {		//resets rear to the first pointer when the end of queue is met
         rear = -1; 
      } 
      
      ++rear;		//updates rear before adding the new item
      rq[rear].task = taskNo;
      rq[rear].burstTime = cpuBurst;

            time_t T= time(NULL);
            struct  tm tm = *localtime(&T);
      items++;		//number of items are incremented by one

      rq[rear].hour = tm.tm_hour;
      rq[rear].minute = tm.tm_min;
      rq[rear].second = tm.tm_sec;  
}

int * remTask() {                                         //removes items from the ready queue
   static int qTask[5];                                   //C creates a warning when returning the address of a local variable outside the function
   
   qTask[0] = rq[front].task;
   qTask[1] = rq[front].burstTime; 
   qTask[2] = rq[front].hour;
   qTask[3] = rq[front].minute;
   qTask[4] = rq[front].second;

   front++;

   if(front == m) {				//resets front to the first pointer when the end of queue is met
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
            anchor++;  										//critical section
            time_t T= time(NULL);			//accessing the local time structure
            struct  tm tm = *localtime(&T);
            pthread_mutex_unlock(&mutex); 							//mutex unlock

            fprintf(fp1,"%d %d %02d:%02d:%02d\n", tq[anchor+l].task,tq[anchor+l].burstTime, tm.tm_hour, tm.tm_min, tm.tm_sec);    //appends task number, burst time & executed time
        }
        else
        {
            pthread_mutex_lock(&mutex);				//mutex lock
            pthread_cond_signal(&cond);             //signals CPU to release its' conditional wait
            printf("Task function waits\n");
            pthread_cond_wait(&cond1,&mutex);			//waits until signal is received from a CPU
            pthread_mutex_unlock(&mutex);			//mutex unlock
        }
    }   

    printf("Task Function Completed\n");

    pthread_mutex_lock(&mutex);			//used to change the broadcast value of comp. mutex used because it is shared by all three CPUs
    comp = 1;
    pthread_mutex_unlock(&mutex);

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
            num_tasks++;			//critical section
            
            fprintf(fp2,"\nTask %d", task);

            fprintf(fp2,"\nArrival time: %02d:%02d:%02d\n", thour, tminute, tsecond);
            fprintf(fp2,"\nService time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

            total_waiting_time += ((60*60*(tm.tm_hour - thour)) + (60*(tm.tm_min - tminute)) + tm.tm_sec - tsecond);
            pthread_mutex_unlock(&mutex);

            //printf("ID %d BT %d\n", task, burst_time);

            sleep(burst_time);

            pthread_mutex_lock(&mutex);
            time_t T1= time(NULL);                                   //process completion time
            struct  tm tm1 = *localtime(&T1);

            fprintf(fp2,"\nStatistics for CPU: %d", cpuNo);
            
            fprintf(fp2,"\nTask %d", task);

            fprintf(fp2,"\nArrival time: %02d:%02d:%02d\n", thour, tminute, tsecond);
            fprintf(fp2,"\nCompletion time: %02d:%02d:%02d\n", tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

            total_turnaround_time += ((60*60*(tm1.tm_hour - thour)) + (60*(tm1.tm_min - tminute)) + tm1.tm_sec - tsecond);

            printf("\nTask %d Completed", task);
            printf("\nArrival time: %02d:%02d:%02d\n", thour, tminute, tsecond);
            printf("\nService time: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
            printf("\nCompletion time: %02d:%02d:%02d\n", tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
            pthread_mutex_unlock(&mutex);

        }else if(isEmpty()&&comp==0){
            printf("CPU %d Waits\n", cpuNo);
            pthread_cond_signal(&cond1);				//signals task function to release its' conditional wait
            pthread_cond_wait(&cond, &mutex);			//waits until signal is received from task function
            pthread_mutex_unlock(&mutex);
        }else {
            printf("CPU %d exited\n", cpuNo); 
            pthread_mutex_unlock(&mutex);				//releases mutex before CPU exits
            break;
        }
    }

    fclose(fp2);

    pthread_exit(NULL); 
}




int main(int args, char* argv[]){ 

    m = atoi(argv[2]);  //initializing m to the passed argument

    rq = malloc(m*sizeof*rq);   //memory allocation for readyQueue

    printf("Enter Number of Tasks : ");
    scanf("%d", &lines);

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

    printf("Number of Tasks: %d\n", anchor);
    printf("Average Waiting Time = %d seconds\n", total_waiting_time/num_tasks);
    printf("Average Turnaround Time = %d seconds\n", total_turnaround_time/num_tasks);    

    return 0;
}
