#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include "list.h"

#define FILE_BUF_SIZE 1024

list_t* results;


typedef struct
{
    int floor;
    int cur_time;
    int direction; // 1 is up, -1 is down, 0 is stop
    list_t* guests_waiting;
    list_t* guests_onboard;
    int next_floor;
}elevator_t;
void print_guests(elevator_t e);

void calculate_next_floor(elevator_t * e)
{
    //printf("Calculating next floor based on info:\n");
    //print_guests(*e);
    if(list_empty(e->guests_onboard) && list_empty(e->guests_waiting))
    {
        e->direction = 0;
        e->next_floor = e->floor;
        return;
    }
    
    int up_up = 0, down_down = 0, up_down = 0, down_up = 0; // counting variables for how many requests happen above the current floor and below
    int lowest_up_up = 101, lowest_up_down = 101, highest_down_down = -1, highest_down_up = -1;
    node_t* cur = e->guests_waiting->head;
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
    
    if(e->direction == 0)
    {
        // This means that the elevator is empty
        assert(list_empty(e->guests_onboard));
        assert(!list_empty(e->guests_waiting));
        
        if(up_up + up_down > down_down + down_up)
        {
            printf("Elevator is going up\n");
            e->direction = 1;
        }
        else
        {
            printf("Elevator is going down\n");
            e->direction = -1;
        }
        
        if(e->direction == 1)
            e->next_floor = up_up != 0 ? lowest_up_up: lowest_up_down;
        else
            e->next_floor = down_down !=0 ? highest_down_down: highest_down_up;
    }
    
    else if(e->direction == 1)
    {
        if(up_up + up_down > 0)
        {
            if(up_up > 0)
                e->next_floor = lowest_up_up;
            else
                e->next_floor = lowest_up_down;
        }
        else
        {
            printf("Elevator changes direction, going down\n");
            e->direction = -1;
            if(down_down > 0)
                e->next_floor = highest_down_down;
            else
                e->next_floor = highest_down_up;
        }
    }
    else
    {
        if(down_down + down_up > 0)
        {
            if(down_down > 0)
                e->next_floor = highest_down_down;
            else
                e->next_floor = highest_down_up;
        }
        else
        {
            printf("Elevator changes direction, going up\n");
            e->direction = 1;
            if(up_up > 0)
                e->next_floor = lowest_up_up;
            else
                e->next_floor = lowest_up_down;
        }
    }
    //TODO: Now calculate the nearest floor to which the on board guests need to get off over the same direction as the elevator's movement
    cur = e->guests_onboard->head;
    int highest_down = -1, lowest_up = 101, up = 0, down = 0;
    while(cur != NULL)
    {
        guest_t* g = (guest_t*) cur->data;
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
    if(e->guests_waiting->head == NULL)
    {
        e->direction = up > down? 1: -1;
        printf("Elevator direction now is :%d\n", e->direction);
        if(e->direction == 1)
            e->next_floor = lowest_up;
        else
            e->next_floor = highest_down;
    }
    else 
    {
        if (e->direction == 0)
            e->direction = up > down ? 1 : -1;
        if (e->direction == 1)
            e->next_floor = e->next_floor > lowest_up ? lowest_up : e->next_floor;
        else
            e->next_floor = e->next_floor < highest_down ? highest_down : e->next_floor;
    }
    
    printf("Next stop is calculated to be %d\n", e->next_floor);
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
    printf("%d Door opened at floor %d\n", e->cur_time, e->floor);
    // First loop through the onboard guests and see if anyone wants to get off, remove them and put them in the restuls lists
    node_t* cur = e->guests_onboard->head;
    while(cur != NULL)
    {
        guest_t * g = (guest_t*) cur->data;
        if(g->to == e->floor)
        {
            printf("Guest %d from floor %d got off, total time: %d\n",g->num, g->at, e->cur_time - g->request_time);
            g->off_time = e->cur_time;
            guest_t* guest_off = malloc(sizeof(guest_t));
            memcpy(guest_off,cur->data, sizeof(guest_t));
            list_remove_guest(e->guests_onboard, &cur);
            guest_off->total_time = guest_off->off_time - guest_off->request_time + 1;
            list_append(results, guest_off);
        }
        else
            cur = cur->next;
    }
    
    // Then loop through the requests and see if anyone could get on. Since we are already stopping here, we
    // don't really care which direction the guest wants to go. This is will probably save time.
    cur = e->guests_waiting->head;
    while(cur != NULL)
    {
        guest_t * g = (guest_t*) cur->data;
        if(g->at == e->floor)
        {
            printf("Guest %d to floor %d got on after waiting for %d seconds.\n", g->num, g->to, e->cur_time - g->request_time);
            guest_t* new_guest = malloc(sizeof(guest_t));
            memcpy(new_guest,cur->data, sizeof(guest_t));
            
            node_t* tmp = e->guests_waiting->head;
            int count = 0;
            while(tmp != NULL)
            {
                count++;
                tmp = tmp->next;
            }
                
            printf("Before remove, elements: %d\n", count);
            
            
            list_remove_guest(e->guests_waiting, &cur);
            count = 0;
            tmp = e->guests_waiting->head;
            while(tmp != NULL)
            {
                count++;
                tmp = tmp->next;
            }
            printf("After remove, elements:%d\n", count);
            list_append(e->guests_onboard, new_guest);
        }
        else
            cur = cur->next;
    }
    e->cur_time +=10;
    if(e->guests_waiting->size != 0)
        assert(e->guests_waiting->head != NULL);
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
    node_t* cur = e.guests_waiting->head;
    while(cur != NULL)
    {
        guest_t* g = (guest_t*)cur->data;
        if(g->at == e.floor)
            return 1;
        cur = cur->next;
    }
    return 0;
}

void print_guests(elevator_t e)
{
    printf("******************** guests *********************\n");
    node_t* cur = e.guests_onboard->head;
    printf("          ========= guests onboard ===========         \n");
    while(cur != NULL)
    {
        guest_t* g = (guest_t*) cur->data;
        printf("Guest %d, got on floor %d, going to floor %d\n", g->num, g->at, g->to);
        cur = cur->next;
    }
    printf("          ========= guests waiting:%d ===========         \n", e.guests_waiting->size);
    cur = e.guests_waiting->head;
    while(cur != NULL)
    {
        guest_t* g = (guest_t*) cur->data;
        printf("Guest %d waiting on floor %d, going to floor %d\n", g->num, g->at, g->to);
        cur = cur->next;
    }
    printf("**************************************************\n");
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
    
    
    char buf[FILE_BUF_SIZE];
    // Consider using fgets
    
    elevator_t elevator;
    elevator.floor = 1;
    elevator.next_floor = -1;
    elevator.cur_time = 0;
    elevator.direction = 0;
    list_init(&elevator.guests_onboard);
    list_init(&elevator.guests_waiting);
    list_init(&results);
    
    // The gist here is to read line by line from the input file, and move along the time line. If the current time
    // is the same as the time in the line, then we put the request record in a list and move on to the next record,
    // but if the time is not the same, then we "move" the elevator floor by floor and let new passengers in and old 
    // passengers out if needed.
    
    // When we open the elevator door, the current time is increased by ten, so then the current time might be larger
    // than the following request time, In this case just read through all the records until the request time is larger
    // than the current time and calculate the new next floor
    
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
        if(time > elevator.cur_time || previous_request_time < time)
        {
            previous_request_time = time;
            // In order to prevent oscillation, we only calculate the next stop floor when:
            // 1. the elevator is empty
            // 2. we processed some request
            // We don't change the course of the movement of the elevator when we have new requests.
            if(elevator.direction == 0)
            {
                //First go through the request list and see if there's anyone requesting from the current floor
                if(need_to_open(elevator))
                    open_door(&elevator);  
            }
            calculate_next_floor(&elevator);
            // If the next request is long apart in time from the previous one and the elevator needs to stop
            while(elevator.cur_time < time)
            {
                elevator.cur_time ++;
                //Change the current floor
                elevator.floor += elevator.direction;
                assert(elevator.floor < 101);
                //printf("Elevator direction is %d\n", elevator.direction);
                printf("Elevator goes to floor %d\n", elevator.floor);
                //sleep(1);
                if(elevator.floor == elevator.next_floor)
                {
                    //print_guests(elevator);
                    open_door(&elevator);
                    calculate_next_floor(&elevator);
                }
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
        list_append(elevator.guests_waiting, guest);
    }// The end of while loop for reading file
    
    
    //print_guests(elevator);
    // Todo calculate solutions for what's left in the queue
    if(elevator.direction == 0)
    {
        //First go through the request list and see if there's anyone requesting from the current floor
        if (need_to_open(elevator))
            open_door(&elevator);
        calculate_next_floor(&elevator);
    }
    while(!list_empty(elevator.guests_onboard) || !list_empty(elevator.guests_waiting))
    {
        while(elevator.floor != elevator.next_floor)
        {
            elevator.floor += elevator.direction;
            printf("Elevator goes to floor %d\n", elevator.floor);
            elevator.cur_time++;
        }
        open_door(&elevator);
        calculate_next_floor(&elevator);
        if(elevator.direction == 0)
        {
            assert(list_empty(elevator.guests_onboard) && list_empty(elevator.guests_waiting));
        }
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
    printf("================== Result ==================\n");
    printf("Average: %f, \t standard deviation: %f\n", mean, standard_devi);
    printf("============================================\n");
    list_free_guests(&results);
    return 0;
}