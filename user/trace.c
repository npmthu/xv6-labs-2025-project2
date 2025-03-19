// user/trace.c
#include "kernel/types.h"   // Include type definitions from the kernel
#include "kernel/stat.h"    // Include file status definitions from the kernel
#include "kernel/param.h"   // Include parameter definitions (e.g., MAXARG)
#include "user/user.h"      // Include user-space library functions and system call declarations

int
main(int argc, char *argv[]) // Entry point of the program; takes command-line arguments
{
  int mask;                 // Variable to store the trace mask
  char *nargv[MAXARG];      // Array to hold the new argument list for the command to be executed
  int i;                    // Loop variable

  // Check if the number of arguments is less than 3
  if(argc < 3){
    // Print usage information to standard error (file descriptor 2)
    fprintf(2, "Usage: trace <mask> command [arguments...]\n");
    exit(1); // Exit with error code 1
  }

  // Convert the first argument (trace mask) from a string to an integer
  mask = atoi(argv[1]);

  // Set up the new argument list for the command to be executed
  nargv[0] = argv[2]; // The command to be executed
  for(i = 3; i < argc && i < MAXARG; i++){ // Copy additional arguments
    nargv[i - 2] = argv[i]; // Shift arguments by 2 to fit into nargv
  }
  nargv[i - 2] = 0; // Null-terminate the argument list

  // Call the trace system call with the specified mask
  trace(mask);

  // Execute the specified command with the new argument list
  exec(nargv[0], nargv);

  // exec fails
  exit(0);
}