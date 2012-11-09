#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//**** Guest elevator status*****
#define WAITING 0
#define ON 1
#define OFF 2
//*******************************
#define FILE_BUF_SIZE 1024

typedef struct
{
    int request_time;
    int at;
    int to;
    int status;
    
}guest;


int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "Syntax error, usage: simulator [test file]\n");
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "file open() error: %s\n", strerror(errno));
        return 1;
    }
    char buf[FILE_BUF_SIZE];
    int bytes_read;
    // Consider using fgets
    while(fgets(buf, FILE_BUF_SIZE, fd))
    {
        printf("%s", buf);
    }
    
    /*
    while((bytes_read = read(fd, buf, sizeof(buf))) != 0)
    {
        if (bytes_read == -1)
        {
            fprintf(stderr, "read() error: %s\n", strerror(errno));
            return 1;
        }
    }*/
    return 0;
}