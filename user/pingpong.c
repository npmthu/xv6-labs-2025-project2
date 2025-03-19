#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    int fd[2];
    pipe(fd);

    fprintf(1, "Create pipe successfully\n");
    char buff[1];

    // Fork
    if(fork() == 0){
        // Child process
        if(read(fd[0], buff, 1) == 1){
            fprintf(1, "%d: received ping\n", getpid());
        }
        if(write(fd[1], buff, 1) < 1){
            fprintf(2, "Error: child write failed\n");
        }
        exit(0);
    } else{
        // Parent
        if(write(fd[1], buff, 1) < 1){
            fprintf(2, "Error: parent write failed\n");
        }
        wait(0);
        if(read(fd[0], buff, 1) == 1){
            fprintf(1, "%d: received pong\n", getpid());
        }
    }

    exit(0);
}