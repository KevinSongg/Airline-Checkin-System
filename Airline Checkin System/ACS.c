#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define NCLERKS 5
#define NQUEUE 2

struct customer_info{
    int user_id;
    int class_type;
    int service_time;
    int arrival_time;
    struct customer_info *next;
};

/* global variables */
 
struct timeval start_time; // start time
double eco_waiting_time;//
double business_waiting_time;
struct customer_info* customers = NULL;
struct customer_info* businessQueue = NULL;
struct customer_info* economyQueue = NULL;// business class wait time
int queue_length[NQUEUE];// store queue length
int clerks[NCLERKS];
int count; // number of customers
int economy_length;
int business_length;
int customers_served = 0;

pthread_mutex_t start_time_mutex; // mutex for calculating time used
pthread_mutex_t economy_queue_mutex;
pthread_mutex_t business_queue_mutex;
pthread_mutex_t clerk1_mutex;
pthread_mutex_t clerk2_mutex;
pthread_mutex_t clerk3_mutex;
pthread_mutex_t clerk4_mutex;
pthread_mutex_t clerk5_mutex;
pthread_mutex_t clerk_access_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t economy_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t business_condition = PTHREAD_COND_INITIALIZER;

pthread_cond_t clerk1_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk2_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk3_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk4_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk5_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t clerk_access_condition = PTHREAD_COND_INITIALIZER;


//given in tutorial 4
double getCurrentSimulationTime(){
    struct timeval cur_time;
    double cur_secs, init_secs;
    pthread_mutex_lock(&start_time_mutex);
    init_secs = (start_time.tv_sec + (double) start_time.tv_usec / 1000000);
    pthread_mutex_unlock(&start_time_mutex);
    gettimeofday(&cur_time, NULL);
    cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
    return cur_secs - init_secs;
}

// add customer into queue
struct customer_info *addQueue(struct customer_info *customer){
    struct customer_info *ptr = (struct customer_info*) malloc(sizeof(struct customer_info));
    struct customer_info *temp = customer;
    ptr->user_id = temp->user_id;
    ptr->class_type = temp->class_type;
    ptr->service_time = temp->service_time;
    ptr->arrival_time = temp->arrival_time;
    
    if(temp == NULL){
        customer = ptr;
    }else{
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = ptr;
    }
    return customer;
}

// remove customer froom queue
void removeQueue(struct customer_info **customer) {
    struct customer_info *temp = *customer;
    struct customer_info *previous;
    
    if(*customer == NULL) return;
    if(temp->next == NULL){
        *customer = NULL;
    }
    while(temp->next != NULL){
        previous = temp;
        temp = temp->next;
    }
    previous->next = NULL;
    free(temp);
}

// check which clerk is available
int clerkAvail(int type) {
    int i;
    for (i = 0; i < NCLERKS; i++) {
        if (clerks[i] == type) {
            clerks[i] = -1;
            return i;
        }
    }
    return -1;
}

// enter customer
void * customer_entry(void * cus_info){
    int clerk;
    struct customer_info * p_myInfo = (struct customer_info *) cus_info;
    usleep(p_myInfo->arrival_time * 100000);
    fprintf(stdout, "A customer arrives: customer ID %2d. \n", p_myInfo->user_id);
   
    // determine if it is a economy class
    if (p_myInfo->class_type == 0){
        pthread_mutex_lock(&economy_queue_mutex);
        economyQueue = addQueue(cus_info);
        queue_length[0]++;
        pthread_cond_signal(&clerk_access_condition); // tell clerk we are ready to be served
        usleep(10000);
        fprintf(stdout,"A customer enters a queue: the economy queue, and length of the queue %2d. \n",queue_length[0]);
        
        // check if there is a clerk available
        while(1){
            pthread_cond_wait(&economy_condition, &economy_queue_mutex);
            clerk = clerkAvail(1);
            pthread_mutex_unlock(&economy_queue_mutex);
            if(clerk==0){
                pthread_mutex_lock(&clerk1_mutex);
            }else if(clerk == 1){
                pthread_mutex_lock(&clerk2_mutex);
            }else if(clerk == 2 ){
                pthread_mutex_lock(&clerk3_mutex);
            }else if(clerk == 3 ){
                pthread_mutex_lock(&clerk4_mutex);
            } else if(clerk == 4 ){
                    pthread_mutex_lock(&clerk5_mutex);
            }
            if (clerk != -1){
                break;
            }
        }
        if(clerk==0){
            pthread_mutex_unlock(&clerk1_mutex);
        }else if(clerk == 1){
            pthread_mutex_unlock(&clerk2_mutex);
        }else if(clerk == 2 ){
            pthread_mutex_unlock(&clerk3_mutex);
        }else if(clerk == 3 ){
            pthread_mutex_unlock(&clerk4_mutex);
        } else if(clerk == 4 ){
            pthread_mutex_unlock(&clerk5_mutex);
        }
        usleep(p_myInfo->service_time*100000);
        customers_served++;
        removeQueue(&economyQueue);
        
        
        //tell clerk we are done
        if(clerk == 0) pthread_cond_signal(&clerk1_condition);
        else if (clerk == 1) pthread_cond_signal(&clerk2_condition);
        else if (clerk == 2) pthread_cond_signal(&clerk3_condition);
        else if (clerk == 3) pthread_cond_signal(&clerk4_condition);
        else if (clerk == 4) pthread_cond_signal(&clerk5_condition);
        
        pthread_exit(NULL);
        
        // check if we are business class
    } else if (p_myInfo->class_type == 1){
        pthread_mutex_lock(&business_queue_mutex);
        businessQueue = addQueue(cus_info);
        queue_length[1]++;
        pthread_cond_signal(&clerk_access_condition); // signal to clerk we are ready
        usleep(10000);
        fprintf(stdout,"A customer enters a queue: the business queue, and length of the queue %2d. \n",queue_length[1]);
        // wait for the clerk to be available
        while(1){
            pthread_cond_wait(&business_condition, &business_queue_mutex);
            clerk = clerkAvail(1);
            pthread_mutex_unlock(&business_queue_mutex);
            if(clerk==0){
                pthread_mutex_lock(&clerk1_mutex);
            }else if(clerk == 1){
                pthread_mutex_lock(&clerk2_mutex);
            }else if(clerk == 2 ){
                pthread_mutex_lock(&clerk3_mutex);
            }else if(clerk == 3 ){
                pthread_mutex_lock(&clerk4_mutex);
            } else if(clerk == 4 ){
                pthread_mutex_lock(&clerk5_mutex);
            }if (clerk != -1){
                break;
            }
          
        }

        if(clerk==0){
            pthread_mutex_unlock(&clerk1_mutex);
        }else if(clerk == 1){
            pthread_mutex_unlock(&clerk2_mutex);
        }else if(clerk == 2 ){
            pthread_mutex_unlock(&clerk3_mutex);
        }else if(clerk == 3 ){
            pthread_mutex_unlock(&clerk4_mutex);
        } else if(clerk == 4 ){
                pthread_mutex_unlock(&clerk5_mutex);
        }
        
        usleep(p_myInfo->service_time*100000);
        customers_served++;
        removeQueue(&businessQueue);
        
        //tell clerk we are done
        if(clerk == 0) pthread_cond_signal(&clerk1_condition);
        else if (clerk == 1) pthread_cond_signal(&clerk2_condition);
        else if (clerk == 2) pthread_cond_signal(&clerk3_condition);
        else if (clerk == 3) pthread_cond_signal(&clerk4_condition);
        else if (clerk == 4) pthread_cond_signal(&clerk5_condition);
        pthread_exit(NULL);
    }
    
}

// function entry for clerk threads
void *clerk_entry(void * clerkNum){
    while(1){
        int clerk_id = *((int *) clerkNum);
        double start = getCurrentSimulationTime();
        if (customers_served == count)pthread_exit(NULL);
        // stop clerks from taking one id all at once
        pthread_mutex_lock(&clerk_access_mutex);
               while (queue_length[0] == 0 && queue_length[1] == 0) {
                   pthread_cond_wait(&clerk_access_condition, &clerk_access_mutex);
               }
        pthread_mutex_unlock(&clerk_access_mutex);
        
        // if it is business class deal with it first
        if(queue_length[1] > 0){
            
           
            // lock the fellow clerk that is going to help
            if (clerk_id == 0){
                pthread_mutex_lock(&clerk1_mutex);
            } else if (clerk_id == 1){
                pthread_mutex_lock(&clerk2_mutex);
           } else if (clerk_id == 2){
                pthread_mutex_lock(&clerk3_mutex);
            } else if (clerk_id == 3){
                pthread_mutex_lock(&clerk4_mutex);
            }else if (clerk_id == 4){
                pthread_mutex_lock(&clerk5_mutex);
           }
            clerks[clerk_id] = 1;
            if (clerk_id == 0){
                pthread_mutex_unlock(&clerk1_mutex);
            } else if (clerk_id == 1){
                pthread_mutex_unlock(&clerk2_mutex);
            } else if (clerk_id == 2){
                pthread_mutex_unlock(&clerk3_mutex);
            } else if (clerk_id == 3){
                pthread_mutex_unlock(&clerk4_mutex);
            }else if (clerk_id == 4){
                pthread_mutex_unlock(&clerk5_mutex);
            }
            pthread_cond_broadcast(&business_condition);
           
            //start serving business customer
            int customer_id = businessQueue->user_id;
            double end = getCurrentSimulationTime();

            printf("A clerk starts serving a business customer: start time %.2f, the customer ID %2d, the clerk ID %1d.\n", end, customer_id, clerk_id);
            queue_length[1]--;
            if (clerk_id == 0) {
                pthread_cond_wait(&clerk1_condition, &clerk1_mutex);
            }
            else if (clerk_id == 1){
                pthread_cond_wait(&clerk2_condition, &clerk2_mutex);
            }
            else if (clerk_id == 2) {
                pthread_cond_wait(&clerk3_condition, &clerk3_mutex);
            }
            else if (clerk_id == 3){
                pthread_cond_wait(&clerk4_condition, &clerk4_mutex);
            }
            else if (clerk_id == 4){
                pthread_cond_wait(&clerk5_condition, &clerk5_mutex);
                
            }

            // finished business customer and reset clerk
            double end_serving = getCurrentSimulationTime();
            business_waiting_time += end - start;
            printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d.\n", end_serving, customer_id, clerk_id);
            if (clerk_id == 0){
                pthread_mutex_unlock(&clerk1_mutex);
            } else if (clerk_id == 1){
                pthread_mutex_unlock(&clerk2_mutex);
            } else if (clerk_id == 2){
                pthread_mutex_unlock(&clerk3_mutex);
            } else if (clerk_id == 3){
                pthread_mutex_unlock(&clerk4_mutex);
            }else if (clerk_id == 4){
                pthread_mutex_unlock(&clerk5_mutex);
            }
            clerks[clerk_id] = -1;
            
        // if it is economy class deal with it second
        // same code as if it was a business class other than the fact it gets chosen second
        }else if(queue_length[0] > 0){
        
            if (clerk_id == 0){
                pthread_mutex_lock(&clerk1_mutex);
            } else if (clerk_id == 1){
                pthread_mutex_lock(&clerk2_mutex);
            } else if (clerk_id == 2){
                pthread_mutex_lock(&clerk3_mutex);
            } else if (clerk_id == 3){
                pthread_mutex_lock(&clerk4_mutex);
            }else if (clerk_id == 4){
                pthread_mutex_lock(&clerk5_mutex);
            }
            clerks[clerk_id] = 1;
            if (clerk_id == 0){
                pthread_mutex_unlock(&clerk1_mutex);
            } else if (clerk_id == 1){
                pthread_mutex_unlock(&clerk2_mutex);
            } else if (clerk_id == 2){
                pthread_mutex_unlock(&clerk3_mutex);
            } else if (clerk_id == 3){
                pthread_mutex_unlock(&clerk4_mutex);
            }else if (clerk_id == 4){
                pthread_mutex_unlock(&clerk5_mutex);
            }
            pthread_cond_broadcast(&economy_condition);
            
            int customer_id = economyQueue->user_id;
            double end = getCurrentSimulationTime();
    
            
            printf("A clerk starts serving a economy customer: start time %.2f, the customer ID %2d, the clerk ID %1d.\n", end, customer_id, clerk_id);
            queue_length[0]--;
            if (clerk_id == 0) pthread_cond_wait(&clerk1_condition, &clerk1_mutex);
            else if (clerk_id == 1) pthread_cond_wait(&clerk2_condition, &clerk2_mutex);
            else if (clerk_id == 2) pthread_cond_wait(&clerk3_condition, &clerk3_mutex);
            else if (clerk_id == 3) pthread_cond_wait(&clerk4_condition, &clerk4_mutex);
            else if (clerk_id == 4) pthread_cond_wait(&clerk5_condition, &clerk5_mutex);
            double end_serving = getCurrentSimulationTime();
            eco_waiting_time += (end - start);
            printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d.\n", end_serving, customer_id, clerk_id);
            if (clerk_id == 0){
                pthread_mutex_unlock(&clerk1_mutex);
            } else if (clerk_id == 1){
                pthread_mutex_unlock(&clerk2_mutex);
            } else if (clerk_id == 2){
                pthread_mutex_unlock(&clerk3_mutex);
            } else if (clerk_id == 3){
                pthread_mutex_unlock(&clerk4_mutex);
            }else if (clerk_id == 4){
                pthread_mutex_unlock(&clerk5_mutex);
            }
            clerks[clerk_id] = -1;
        
        }
        if (count == customers_served){
            printf("The average waiting time for all customers in the system is: %.2f seconds. \n",(eco_waiting_time+business_waiting_time)/count);
            printf("The average waiting time for all business-class customers is: %.2f seconds. \n",business_waiting_time/business_length);
            printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",eco_waiting_time/economy_length);
            exit(0);
        }
     

    }
    
    pthread_exit(NULL);
    
    return NULL;
}




int main(int argc, char *argv[]) {
    
    // error checking for valid file input
    int i;
    if (argc != 2) {                        //number of arguments required to run
        printf("ERROR: Incorrect number of arguments");
        exit(1);
    }
    char *line = NULL;                      //line storage
    size_t len = 0;
    FILE *file;
    file = fopen(argv[1], "r");
    if (file == NULL){
        printf("ERROR: Unable to open the file.\n");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "%d", &count);
    if (count <= 0) {
        fprintf(stderr, "ERROR: Number of customers must be positive\n");
        exit(EXIT_FAILURE);
    }
    struct customer_info *temp = (struct customer_info*)malloc(sizeof(struct customer_info));
    
    // read the file contents
    for (i = 0; i < count; i++) {
    

        fscanf(file, "%d:%d,%d,%d", &temp->user_id,&temp->class_type, &temp->arrival_time, &temp->service_time);

        if (temp->user_id < 0 || temp->class_type < 0 || temp->service_time < 0 || temp->arrival_time < 0) {
          printf("ERROR: Invalid input \n");
        } else {
            if (temp->class_type == 0){
                economy_length++;
            }else if(temp->class_type == 1){
                business_length++;
            }
            customers = addQueue(temp);
        }
      }
    fclose(file);
    gettimeofday(&start_time,NULL);
    pthread_t clerk_threads[NCLERKS];
    pthread_t economy_threads[count];

    //initializing all mutex locks
    pthread_mutex_init(&start_time_mutex,NULL);
    pthread_mutex_init(&economy_queue_mutex, NULL);
    pthread_mutex_init(&business_queue_mutex, NULL);
    pthread_mutex_init(&clerk1_mutex, NULL);
    pthread_mutex_init(&clerk2_mutex, NULL);
    pthread_mutex_init(&clerk3_mutex, NULL);
    pthread_mutex_init(&clerk4_mutex, NULL);
    pthread_mutex_init(&clerk5_mutex, NULL);
    
    // create threads
    
    for (i = 0; i < NQUEUE; i++) queue_length[i] = 0;
    for (i = 0; i < count; i++){
        pthread_create(&economy_threads[i],NULL, customer_entry, (void *)customers);
        customers = customers->next;
    }

    for (i=0; i < NCLERKS; i++){
        clerks[i] = -1;
        int *clerk_id = malloc(sizeof(*clerk_id));
        *clerk_id = i;
        pthread_create(&clerk_threads[i], NULL, clerk_entry, (void*)clerk_id);
    }
    // wait for threads to finish

    for (i = 0; i < count; i++){
        pthread_join(economy_threads[i],NULL);
    }

    for (i=0; i < NCLERKS; i++){
        pthread_join(clerk_threads[i], NULL);
    }
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n",(eco_waiting_time+business_waiting_time)/count);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n",business_waiting_time/business_length);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",eco_waiting_time/economy_length);
    
    // destroy conditional variables and mutex
    pthread_mutex_destroy(&start_time_mutex);
    pthread_mutex_destroy(&economy_queue_mutex);
    pthread_mutex_destroy(&business_queue_mutex);
    pthread_mutex_destroy(&clerk1_mutex);
    pthread_mutex_destroy(&clerk2_mutex);
    pthread_mutex_destroy(&clerk3_mutex);
    pthread_mutex_destroy(&clerk4_mutex);
    pthread_mutex_destroy(&clerk5_mutex);
    pthread_mutex_destroy(&clerk_access_mutex);


    pthread_cond_destroy(&economy_condition);
    pthread_cond_destroy(&business_condition);
    pthread_cond_destroy(&clerk1_condition);
    pthread_cond_destroy(&clerk2_condition);
    pthread_cond_destroy(&clerk3_condition);
    pthread_cond_destroy(&clerk4_condition);
    pthread_cond_destroy(&clerk5_condition);
    pthread_cond_destroy(&clerk_access_condition);

    return 0;
    
}



