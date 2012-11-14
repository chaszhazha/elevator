#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include "list.h"

#define FILE_BUF_SIZE 1024

list_t* results;
int quit_flag = 0;
int quit_counter = 0;

list_t* guests_waiting;
int tick_counter = 0;
int global_time = 0;
pthread_mutex_t tick_counter_lock;      // Counts how many elevators and the main thread have ticked, circularly
                                        //changes from 0 to 4, when an elevator ticks and sees
                                        // a count of 4, then it knows that it is the fifth one 
                                        //so it will change the counter to 0 and signals other 
                                        //elevator threads to wake up to do their next tick
pthread_cond_t tick_condition; // Used to synchronize the four elevators temporally
 
pthread_mutex_t waiting_guest_lock;
pthread_mutex_t result_lock;

typedef struct
{
    pthread_mutex_t lock;
    pthread_t thread;
    int num;
    int floor;
    //int cur_time;
    int direction; // 1 is up, -1 is down, 0 is stop
    //list_t* guests_waiting;
    list_t* guests_onboard;
    int next_floor;
    int open_wait; // Number of "seconds" the elevator still has to wait because of opening its door
}elevator_t;

elevator_t elevators[4];

void print_guests(elevator_t e);

void calculate_next_floor(elevator_t * e)
{
    //printf("Calculating next floor based on info:\n");
    //print_guests(*e);
    if(list_empty(e->guests_onboard) && list_empty(guests_waiting))
    {
        e->direction = 0;
        e->next_floor = e->floor;
        return;
    }
    
    int up_up = 0, down_down = 0, up_down = 0, down_up = 0; // counting variables for how many requests happen above the current floor and below
    int lowest_up_up = 101, lowest_up_down = 101, highest_down_down = -1, highest_down_up = -1;
    
    //printf("Getting waiting guest lock in calculate\n");
    pthread_mutex_lock(&waiting_guest_lock);
    //printf("Waiting guest lock obtained in calculate\n");
    node_t* cur = guests_waiting->head;
    while (cur != NULL) {
        guest_t* g = (guest_t*) cur->data;

        if (g->at > e->floor && g->to > g->at) 
        {
            up_up++;
            if (g->at < lowest_up_up)
                lowest_up_up = g->at;
        } 
        else if (g->at > e->floor && g->to < g->at)
        {
            up_down++;
            if (g->at < lowest_up_down)
                lowest_up_down = g->at;
        } 
        else if (g->at < e->floor && g->to < g->at) 
        {
            down_down++;
            if (g->at > highest_down_down)
                highest_down_down = g->at;
        } 
        else if (g->at < e->floor && g->to > g->at) 
        {
            down_up++;
            if (g->at > highest_down_up)
                highest_down_up = g->at;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&waiting_guest_lock);
    
    pthread_mutex_lock(&e->lock);
    cur = e->guests_onboard->head;
    int highest_down = -1, lowest_up = 101, up = 0, down = 0;
    while(cur != NULL)
    {
        guest_t* g = (guest_t*) cur->data;
        /*
        if(global_time - g->request_time > 1000)
        {
            e->direction = g->to > e->floor ? 1 :-1;
            e->next_floor = g->to;
            pthread_mutex_unlock(&e->lock);
            return;
        }*/
        if(g->to > e->floor && g->to < lowest_up)
        {
            up++;
            lowest_up = g->to;
        }
        if(g->to < e->floor && g->to > highest_down)
        {
            down++;
            highest_down = g->to;
        }
        cur = cur->next;
    }
    //printf("Getting elevator lock for e%d in calculate\n", e->num);
    
    //printf("Elevator lock %d aquired in calculate\n", e->num);
    if(e->direction == 0)
    {   
        if(up_up + up_down + up > down_down + down_up + down)
        {
            printf("Elevator %d is going up\n", e->num);
            e->direction = 1;
        }
        else
        {
            printf("Elevator %d is going down\n", e->num);
            e->direction = -1;
        }
        
        if(e->direction == 1)
            e->next_floor = up_up != 0 ? lowest_up_up: lowest_up_down;
        else
            e->next_floor = down_down !=0 ? highest_down_down: highest_down_up;
    }
    
    else if(e->direction == 1)
    {
        if(up_up + up_down + up > 0)
        {
            if(up_up > 0)
            {
                if(up > 0)
                        e->next_floor = lowest_up_up > lowest_up ? lowest_up : lowest_up_up;
                else e->next_floor = lowest_up_up;
            }
            else
            {
                if(up > 0)
                        e->next_floor = lowest_up_down > lowest_up ? lowest_up : lowest_up_down;
                else
                    e->next_floor = lowest_up_down;
            }
        }
        else
        {
            printf("Elevator %d changes direction, going down\n", e->num);
            e->direction = -1;
            if(down_down > 0)
            {
                if(down > 0)
                    e->next_floor = highest_down_down > highest_down ? highest_down_down : highest_down;
                else    
                    e->next_floor = highest_down_down;
            }
            else
            {
                if(down > 0)
                    e->next_floor = highest_down_up > highest_down ? highest_down_up:highest_down;
                else
                    e->next_floor = highest_down_up;
            }
        }
    }
    else
    {
        if(down_down + down_up + down > 0)
        {
            if(down_down > 0)
            {
                if(down > 0)
                    e->next_floor = highest_down_down > highest_down ? highest_down_down : highest_down;
                else    
                    e->next_floor = highest_down_down;
            }
            else
            {
                if(down > 0)
                    e->next_floor = highest_down_up > highest_down ? highest_down_up:highest_down;
                else
                    e->next_floor = highest_down_up;
            }
        }
        else
        {
            printf("Elevator %d changes direction, going up\n", e->num);
            e->direction = 1;
            if(up_up > 0)
            {
                if(up > 0)
                        e->next_floor = lowest_up_up > lowest_up ? lowest_up : lowest_up_up;
                else e->next_floor = lowest_up_up;
            }
            else
            {
                if(up > 0)
                        e->next_floor = lowest_up_down > lowest_up ? lowest_up : lowest_up_down;
                else
                    e->next_floor = lowest_up_down;
            }
        }
    }
    
    pthread_mutex_unlock(&e->lock);
    printf("Next stop for elevator %d is calculated to be %d\n",e->num, e->next_floor);
    //print_guests((*e));
    if(!(e->next_floor >=0 && e->next_floor <= 100))
        print_guests(*e);
    assert(e->next_floor >=0 && e->next_floor <= 100);
}

/**
 * Open the elevator door, add 10 to the cur_time, and let in and out guests
 * When a guest goes out of the elevator, remove him/her from the list and add to the result list
 * for evaluation purposes
 * @param argc
 * @param argv
 * @return 
 */
void open_door(elevator_t *e)
{
    assert(e->open_wait == 0);
    e->open_wait = 10;
    printf("%d Elevator %d: door opened at floor %d\n", global_time,e->num, e->floor);
    //printf("%d Door opened at floor %d\n", e->cur_time, e->floor);
    // First loop through the onboard guests and see if anyone wants to get off, remove them and put them in the restuls lists
    node_t* cur = e->guests_onboard->head;
    while(cur != NULL)
    {
        guest_t * g = (guest_t*) cur->data;
        if(g->to == e->floor)
        {
            printf("One passenger from floor %d got off elevator %d, total time: %d\n", g->at, e->num, global_time - g->request_time);
            //printf("Guest %d from floor %d got off, total time: %d\n",g->num, g->at, e->cur_time - g->request_time);
            g->off_time = global_time;
            guest_t* guest_off = malloc(sizeof(guest_t));
            memcpy(guest_off,cur->data, sizeof(guest_t));
            list_remove_guest(e->guests_onboard, &cur);
            guest_off->total_time = guest_off->off_time - guest_off->request_time + 1;
            
            //printf("");
            pthread_mutex_lock(&result_lock);
            list_append(results, guest_off);
            pthread_mutex_unlock(&result_lock);
        }
        else
            cur = cur->next;
    }
    
    // Then loop through the requests and see if anyone could get on. Since we are already stopping here, we
    // don't really care which direction the guest wants to go. This is will probably save time.
    
    pthread_mutex_lock(&waiting_guest_lock);
    cur = guests_waiting->head;
    while(cur != NULL)
    {
        guest_t * g = (guest_t*) cur->data;
        if(g->at == e->floor)
        {
            printf("Guest %d to floor %d got on elevator %d after waiting for %d seconds.\n", g->num, e->num ,g->to, global_time - g->request_time);
            guest_t* new_guest = malloc(sizeof(guest_t));
            memcpy(new_guest,cur->data, sizeof(guest_t));
            
            node_t* tmp = guests_waiting->head;
            int count = 0;
            while(tmp != NULL)
            {
                count++;
                tmp = tmp->next;
            }
                
            list_remove_guest(guests_waiting, &cur);
            count = 0;
            tmp = guests_waiting->head;
            while(tmp != NULL)
            {
                count++;
                tmp = tmp->next;
            }
            pthread_mutex_lock(&e->lock);
            list_append(e->guests_onboard, new_guest);
            pthread_mutex_unlock(&e->lock);
        }
        else
            cur = cur->next;
    }
    //e->cur_time +=10;
    pthread_mutex_unlock(&waiting_guest_lock);
    
    if(guests_waiting->size != 0)
        assert(guests_waiting->head != NULL);
    printf("\n");
}

/**
 * Return whether there are requests happening on the current floor.
 * This function is called before calculate_next_floor when calculate_next_floor is called without
 * calling open_door() beforehand. This is the scenario where the elevator is not moving and then has a 
 * request. If there are requests happening on the same floor then it should let the new guests in
 * before calculation the new movement parameters of the elevator
 * @param e
 * @return 
 */
int need_to_open(elevator_t e)
{
    pthread_mutex_lock(&waiting_guest_lock);
    node_t* cur = guests_waiting->head;
    while(cur != NULL)
    {
        guest_t* g = (guest_t*)cur->data;
        if(g->at == e.floor)
        {
            pthread_mutex_unlock(&waiting_guest_lock);
            return 1;
        }
        cur = cur->next;
    }
    pthread_mutex_unlock(&waiting_guest_lock);
    return 0;
}

void print_guests(elevator_t e)
{
    printf("******************** guests *********************\n");
    pthread_mutex_lock(&e.lock);
    node_t* cur = e.guests_onboard->head;
    printf("          ========= guests onboard ===========         \n");
    while(cur != NULL)
    {
        guest_t* g = (guest_t*) cur->data;
        printf("Guest %d, got on floor %d, going to floor %d\n", g->num, g->at, g->to);
        cur = cur->next;
    }
    pthread_mutex_unlock(&e.lock);
    
    printf("          ========= guests waiting:%d ===========         \n", guests_waiting->size);
    pthread_mutex_lock(&waiting_guest_lock);
    cur = guests_waiting->head;
    while(cur != NULL)
    {
        guest_t* g = (guest_t*) cur->data;
        printf("Guest %d waiting on floor %d, going to floor %d\n", g->num, g->at, g->to);
        cur = cur->next;
    }
    pthread_mutex_unlock(&waiting_guest_lock);
    printf("**************************************************\n");
} 

/**
 * Make the elevator move to the next temporal spot
 * @param e
 */
void tick(elevator_t *e)
{
    if(e->open_wait >0)
    {
        e->open_wait -- ;
    }
    else
    {
        if(e->direction == 0 && guests_waiting->head != NULL)
        {
            //printf("Checking to see if need open elevator %d\n", e->num);
            if(need_to_open(*e))
                open_door(e);
            calculate_next_floor(e);
        }
        e->floor += e->direction;
        assert(e->floor < 101);
        printf("Elevator %d goes to floor %d, next stop is %d\n",e->num, e->floor, e->next_floor);
        //sleep(1);
        if (e->floor == e->next_floor)
        {
            int guests_waiting_before = guests_waiting->size;
            open_door(e);
            calculate_next_floor(e);
            if(guests_waiting->size != guests_waiting_before)
            {
                int i;
                for(i = 0; i < 4; i ++)
                {
                    if(i + 1 == e->num)
                        continue;
                    calculate_next_floor(&elevators[i]);
                }
            }   
        }
    }
    
    //printf("Getting ticker lock for elevator %d\n", e->num);
    pthread_mutex_lock(&tick_counter_lock);
    //printf("Ticker lock aquired for elevator %d\n", e->num);
    if(tick_counter == 4)
    {
        //printf("Elevator moved ticker back to 0, signaling other threads to wake up\n", e->num);
        tick_counter = 0;
        global_time++;
        pthread_cond_broadcast(&tick_condition);
    }
    else
    {
        tick_counter++;
        assert(tick_counter <= 4);
        //printf("Elevator %d moved tick counter to %d, waiting to wake up\n", e->num, tick_counter);
        pthread_cond_wait(&tick_condition, & tick_counter_lock);
    }
    pthread_mutex_unlock(&tick_counter_lock);
}

void* elevator_thr_func(void* arg)
{
    elevator_t* e = (elevator_t*) arg;
    while(quit_flag == 0 || e->guests_onboard->size >0 || guests_waiting->size > 0)
    {
        tick(e);
    }
    // Continue to syn with unfinished elevators
    quit_counter++;
    if(quit_counter == 4)
    {
        pthread_mutex_lock(&tick_counter_lock);
        pthread_cond_broadcast(&tick_condition);
        pthread_mutex_unlock(&tick_counter_lock);
    } 
    while(quit_counter < 4)
    {
        // Wait for all elevators to finish their job
        pthread_mutex_lock(&tick_counter_lock);
        if (tick_counter == 4) {
            tick_counter = 0;
            global_time++;
            pthread_cond_broadcast(&tick_condition);
        } else {
            tick_counter++;
            assert(tick_counter <= 4);
            pthread_cond_wait(&tick_condition, & tick_counter_lock);
        }
        pthread_mutex_unlock(&tick_counter_lock);
    }
    //printf("quit counter is %d\n", quit_counter);
    printf("Elevator %d finished\n", e->num);
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "Syntax error, usage: simulator [test file]\n");
        return 1;
    }
    FILE* file = fopen(argv[1], "r");
    if (file == NULL)
    {
        fprintf(stderr, "file fopen() error: %s\n", strerror(errno));
        return 1;
    }
    
    pthread_mutex_init(&result_lock, NULL);
    pthread_mutex_init(&waiting_guest_lock, NULL);
    char buf[FILE_BUF_SIZE];
    // Consider using fgets
    int i;
    for(i = 0; i < 4; i++)
    {
        elevators[i].floor = 1;
        elevators[i].next_floor = -1;
        elevators[i].direction = 0;
        elevators[i].num = (i + 1);
        list_init(&elevators[i].guests_onboard);
        pthread_mutex_init(&elevators[i].lock, NULL);
        pthread_create(&elevators[i].thread, NULL, elevator_thr_func, &elevators[i]);
    }
    /*    elevator_t elevator;
    elevator.floor = 1;
    elevator.next_floor = -1;
    elevator.cur_time = 0;
    elevator.direction = 0;
    list_init(&elevator.guests_onboard);
    list_init(&elevator.guests_waiting);*/
    
    list_init(&results);
    list_init(&guests_waiting);
    
    int previous_request_time = -1;
    int n = 0;
    while(fgets(buf, FILE_BUF_SIZE, file))
    {
        int time, from, to;
        sscanf(buf, "%d, %d, %d",&time, &from, &to );
        printf("%d, from %d to %d\n", time, from, to);
        if(from ==  to || from < 0 || to > 100)
        {
            // Ignore brattish requests
            fprintf(stderr, "Bad request\n");
            continue;
        }
        if(previous_request_time == -1)
            previous_request_time = time;
        if(time > global_time || previous_request_time < time)
        {
            previous_request_time = time;
            // If the next request is long apart in time from the previous one and the elevator needs to stop
            while(global_time < time)
            {
                //sleep(1);
                // synchronize time across threads
                //printf("Getting ticker lock in main\n");
                pthread_mutex_lock(&tick_counter_lock);
                //printf("Ticker lock aquired in main\n");
                if (tick_counter == 4) {
                    //printf("Main thread moved ticker to 0, signaling other threads to wake up\n");
                    tick_counter = 0;
                    global_time++;
                    pthread_cond_broadcast(&tick_condition);
                } else {
                    tick_counter++;
                    assert(tick_counter <= 4);
                    //printf("Main thread moved tick counter to %d, waiting to wake up\n", tick_counter);
                    pthread_cond_wait(&tick_condition, & tick_counter_lock);
                }
                pthread_mutex_unlock(&tick_counter_lock);
                
            }
        }
        previous_request_time = time;
        guest_t * guest = malloc(sizeof(guest_t));
        memset(guest, 0, sizeof(guest_t));
        guest->request_time = time;
        guest->at = from;
        guest->to = to;
        guest->off_time = -1;
        guest->num = n++;
        
        pthread_mutex_lock(&waiting_guest_lock);
        list_append(guests_waiting, guest);
        pthread_mutex_unlock(&waiting_guest_lock);
    }// The end of while loop for reading file
    
    
    //print_guests(elevator);
    // Todo calculate solutions for what's left in the queue

    quit_flag = 1;
    while(quit_counter < 4)
    {
        // Wait for all elevators to finish their job
        pthread_mutex_lock(&tick_counter_lock);
        if (tick_counter == 4) {
            tick_counter = 0;
            global_time++;
            pthread_cond_broadcast(&tick_condition);
        } else {
            tick_counter++;
            assert(tick_counter <= 4);
            pthread_cond_wait(&tick_condition, & tick_counter_lock);
        }
        pthread_mutex_unlock(&tick_counter_lock);
    }
    
    //Calculate mean and deviation
    node_t* cur = results->head;
    uint32_t sum = 0;
    int count = 0;
    guest_t* g;
    while(cur != NULL)
    {
        count++;
        g = (guest_t*)cur->data;
        sum+= g->total_time;
        cur = cur->next;
    }
    double mean = sum *1.0 / count;
    uint32_t devi = 0;
    cur = results->head;
    while(cur != NULL)
    {
        g = (guest_t*)cur->data;
        devi += ((g->total_time - mean) * (g->total_time - mean));
        cur = cur->next;
    }
    double standard_devi = sqrt(devi * 1.0 / count);
    printf("====================== Result ======================\n");
    printf("Average: %f, \t standard deviation: %f\n", mean, standard_devi);
    printf("====================================================\n");
    
    for(i = 0; i < 4; i++)
        list_free_guests(&elevators[i].guests_onboard);
    list_free_guests(&guests_waiting);
    list_free_guests(&results);
    return 0;
}