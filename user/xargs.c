#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define BUF_SIZE 512

int main(int argc, char *argv[]) {
    char buf[BUF_SIZE];
    char *x_argv[MAXARG];
    char *arg_s, *arg_e;
    char ch;
    int read_bytes, has_arg = 0, child_pid;

    // Copy command arguments, but ignore `-n 1`
    int base_argc = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc && strcmp(argv[i + 1], "1") == 0) {
            i++;  // Skip "-n 1"
            continue;
        }
        x_argv[base_argc++] = argv[i];
    }
    
    if (base_argc == 0) {
        fprintf(2, "xargs: missing command\n");
        exit(1);
    }

    x_argv[base_argc] = 0;  // Null-terminate argument list

    arg_s = arg_e = buf;

    while ((read_bytes = read(0, &ch, sizeof(char))) == 1) {
        if (arg_e >= buf + BUF_SIZE - 1) {
            fprintf(2, "xargs: argument too long\n");
            arg_s = arg_e = buf;  // Reset buffer
            continue;
        }
        if (ch == '\n' || ch == ' ') {  
            if (has_arg) {  
                *arg_e = '\0';  // Null-terminate argument
                
                // Prepare command arguments
                x_argv[base_argc] = arg_s;
                x_argv[base_argc + 1] = 0;  // Null-terminate

                // Fork and execute immediately
                if ((child_pid = fork()) < 0) {
                    fprintf(2, "xargs: fork failed\n");
                    exit(1);
                } else if (child_pid == 0) {
                    exec(x_argv[0], x_argv);
                    fprintf(2, "xargs: exec failed (command: %s)\n", x_argv[0]);
                    exit(1);
                }

                has_arg = 0;
                wait(0);  // Wait for process
            }
            arg_s = arg_e = buf;  // Reset buffer
        } else {
            *arg_e++ = ch;
            has_arg = 1;
        }
    }

    // Handle last argument if input doesn't end with newline
    if (has_arg) {
        *arg_e = '\0';

        x_argv[base_argc] = arg_s;
        x_argv[base_argc + 1] = 0;

        if ((child_pid = fork()) < 0) {
            fprintf(2, "xargs: fork failed\n");
            exit(1);
        } else if (child_pid == 0) {
            exec(x_argv[0], x_argv);
            fprintf(2, "xargs: exec failed (command: %s)\n", x_argv[0]);
            exit(1);
        }
        wait(0);
    }

    exit(0);
}
