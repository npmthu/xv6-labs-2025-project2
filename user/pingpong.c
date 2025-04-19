#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int p1[2]; // pipe from parent to child
    int p2[2]; // pipe from child to parent
    pipe(p1);
    pipe(p2);

    char buf[1] = {'x'};

    if (fork() == 0) {
        // Child
        close(p1[1]); // close write end of parent-to-child
        close(p2[0]); // close read end of child-to-parent

        read(p1[0], buf, 1); // receive ping
        fprintf(1, "%d: received ping\n", getpid());

        write(p2[1], buf, 1); // send pong
        close(p1[0]);
        close(p2[1]);
        exit(0);
    } else {
        // Parent
        close(p1[0]); // close read end of parent-to-child
        close(p2[1]); // close write end of child-to-parent

        write(p1[1], buf, 1); // send ping
        read(p2[0], buf, 1); // receive pong
        fprintf(1, "%d: received pong\n", getpid());

        close(p1[1]);
        close(p2[0]);
        wait(0);
    }

    exit(0);
}

