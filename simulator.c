#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "list.h"

//**** Guest elevator status*****
#define WAITING 0
#define ON 1
#define OFF 2
//*******************************
#define FILE_BUF_SIZE 1024

typedef struct
{
    int floor;
    int cur_time;
    int direction; // 1 is up, -1 is down, 0 is stop
    list_t* guests_waiting;
    list_t* guests_onboard;
    int next_floor;
}elevator_t;


void calculate_next_floor(elevator_t * e)
{
    // calculate the next_floor and the movement direction for the elevator
}

/**
 * Open the elevator door, add 10 to the cur_time, and let in and out guests
 * When a guest goes out of the elevator, remove him/her from the list and add to the result list
 * for evaluation purposes
 * @param argc
 * @param argv
 * @return 
 */
void open(elevator_t *e)
{
    // First loop through the onboard guests and see if anyone wants to get off, remove them and put them in the restuls lists
    
    
    // Then loop through the requests and see if anyone could get on. Since we are already stopping here, we
    // don't really care which direction the guest wants to go. This is will probably save time.
    
    
    
    e->cur_time +=10;
}


list_t* results;
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
    
    int cur_floor = 1;
    elevator_t elevator;
    elevator.floor = 1;
    elevator.next_floor = -1;
    elevator.cur_time = 0;
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
    
    while(fgets(buf, FILE_BUF_SIZE, file))
    {
        int time, from, to;
        sscanf(buf, "%d, %d, %d",&time, &from, &to );
        printf("%d, from %d to %d\n", time, from, to);
        if(from ==  to || from < 0 || to > 100)
        {
            // Ignore brattish requests
            continue;
        }
        if(time > elevator.cur_time)
        {
            // Calculate the next stop floor based on the requests, at this moment, the elevator has not moved floor yet
            
            // If the next request is long apart in time from the previous one and the elevator needs to stop
            while(elevator.cur_time <= time)
            {
                elevator.cur_time ++;
                //Change the current floor
                cur_floor += elevator.direction;
                if(cur_floor == elevator.next_floor)
                {
                    open(&elevator);
                    calculate_next_floor(&elevator);
                }
            }
        }
        
        guest_t * guest = malloc(sizeof(guest_t));
        memset(guest, 0, sizeof(guest_t));
        guest->request_time = time;
        guest->at = from;
        guest->to = to;
        guest->off_time = -1;
        guest->status = WAITING;
        list_append(&elevator.guests_waiting, guest);
    }
    // Todo calculate solutions for what's left in the queue
    return 0;
}