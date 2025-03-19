#include "kernel/types.h" // Type definitions
#include "user/user.h" // User-level system

// __attribute__((noreturn)) -> Indicate that the function does not return
// => Help the compiler understand that the recursion will eventually terminate
// => Without this, the compiler will generate a warning about primes(pipefd_next) is an infinite recursion
void primes(int pipefd[2]) __attribute__((noreturn));

// Implement sieve logic: Takes a pipe file descriptor array (pipefd) as an input
// The pipe is used to communicate with the previous process in the pipeline
void primes(int pipefd[2]) {
    int prime; // Store the current prime number
    int num; // Store numbers read from the pipe
    int pipefd_next[2]; // File descriptors for the next pipe

    close(pipefd[1]); // The proccess only reads from the pipe -> Doesn't need the write end -> Close the write end

    // Read the first prime number from the pipe
    // It is guaranteed to be prime since all multiples of previous primes have been filtered out
    if (read(pipefd[0], &prime, sizeof(prime)) != sizeof(prime)) {
        // ssize_t read(int fd, void *buf, size_t count);
        // fd: File descriptor; buf: buffer to store read data; count: size in bytes of the buffer
        // return: Number of bytes read; -1 on error; 0 on EOF
        

        if (read(pipefd[0], &prime, sizeof(prime)) == 0) {
            // If read returns 0 -> the pipe has been closed
            close(pipefd[0]); // Close the read end
            exit(0);
        }
        else {
            // Actual error
            fprintf(2, "primes: read error\n");
            exit(1);
        }
    }

    printf("prime %d\n", prime); // Print the prime number

    pipe(pipefd_next); // Create a new pipe -> communicate with the next process in the pipeline

    // Fork a new process to handle the next stage of the sieve
    if (fork() == 0) {
        // Child process (fork() returns 0)
        // Child process will handle the stage of the sieve (the next prime number)

        close(pipefd[0]); // Ensure the child process only reads pipefd_next and not the initial pipefd -> Avoid interference
        primes(pipefd_next);
    }
    else {
        // Parent process (fork() returns the child's PID)
        // Parent process will filter out multiples of the current prime number (it continues here)

        close(pipefd_next[0]); // We don't want parent to read from the next pipe (It's for the child process)
        // Read numbers from the current pipe -> Filter out multiples of the current prime
        while (read(pipefd[0], &num, sizeof(num)) > 0) {

            // If a number is not a multiple of the current prime -> write it to the next pipe
            if (num % prime != 0) {
                write(pipefd_next[1], &num, sizeof(num));
            }
        }

        close(pipefd_next[1]); // Close the write end of the new pipe -> No more numbers to write (signal to the next process)

        wait(0); // Wait for the child process to finish
    }

    close(pipefd[0]); // Close the read end of the current pipe
    exit(0); // Exit the process
}

int main() {
    int pipefd[2]; // File descriptors for the initial pipe
    int i; // Loop variable for feeding numbers (2 to 280) into the pipeline

    pipe(pipefd); // Create the initial pipe

    if (fork() == 0) {
        // Child process (fork() returns 0)
        // Child process will handle the stage of the sieve (the first prime number)

        primes(pipefd); // Call the sieve logic with the initial pipe
    }
    else {
        // Parent process (fork() returns the child's PID)
        // Parent process will feed numbers into the pipeline (it continues here)

        close(pipefd[0]); // Parent process only writes to the pipe -> Close the read end

        // Feed numbers into the pipeline
        for (i = 2; i <= 280; i++) {
            write(pipefd[1], &i, sizeof(i)); // Feed numbers 2 -> 280 to the intial pipe
        }

        close(pipefd[1]); // Close the write end of the pipe -> No more numbers to write (signal to the child process)

        wait(0); // Wait for the child process to finish
    }

    exit(0); // Exit the process
}