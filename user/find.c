#include "kernel/types.h" // User-level type definitions
#include "kernel/stat.h" // Defines stat structure -> Used to store file metadata (e.g., type, size)
#include "user/user.h" // User-level system calls (open, read, close, stat, printf, etc.)
#include "kernel/fs.h" // File system structure (such as directory entry dirent)

// Find all files in the current directory and its subdirectories
// Find function: Recursively search for files in the current directory and its subdirectories
void find(char* path, char *target) {
    // path: string of the base dir path; target: string of the target file name to search for

    char buf[512], *p; // Buffer to store file names and a pointer to the buffer
    // Buffer 512 bytes: matches xv6 file system block size (512 bytes) -> struct dirent stores file names in fixed-size 512 byte

    int fd; // File descriptor for the searched dir

    struct dirent de; // A dirent structure to store the file dir entry (name, inode#)
    struct stat st; // A stat structure to store the file metadata (type, size)
    // Dirent vs stat: dirent stores basic file info (name, inode#), stat stores more detailed file info (type, size)

    // Open the directory
    if((fd = open(path, 0)) < 0){
        // int open(const char *path, int flags);
        // path: string of the dir path; flags: file access mode (0: read-only, 1: write-only, 2: read-write)
        // open function returns: file descriptor (fd) if successful, -1 if failed

        fprintf(2, "find: cannot open %s\n", path); // Error message if the directory cannot be opened
        return;
    }

    // Get metadata of the dir using fstat
    if(fstat(fd, &st) < 0) {
        // int fstat(int fd, struct stat *st);
        // fd: file descriptor of the dir; st: pointer to the stat structure to store the file metadata
        // fstat function returns: 0 if successful, -1 if failed

        fprintf(2, "find: cannot stat %s\n", path);
        close(fd); // Close the directory
        return;
    }

    // Check if the dir is a directory
    if (st.type != T_DIR) {
        // T_DIR: directory type defined in fs.h

        fprintf(2, "find: %s is not a directory\n", path);
        close(fd); // Close the directory
        return;
    }

    // Read the directory entries (each dirs (files, subdirs) inside the base dir) one by one
    // Each entry is of size sizeof(de) = 16 bytes
    // Each entry contains the file name and the inode number
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        // int read(int fd, void *buf, int n);
        // fd: file descriptor of the dir; buf: buffer to store the file dir entry; n: number of bytes to read
        // read function returns: number of bytes read if successful, -1 if failed

        if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            // de.inum == 0: empty entry (inode# = 0); . and ..: current and parent dir (special dirs)
            // We need to skip these since we currently search for files + subdir <- cur dir . is the base dir, parent dir .. is outside the base dir
            continue;
        }

        // Construct the full path of the cur dir entry:
        // 1. Copy the base path -> buf
        // 2. Move pointer p to the end of the base path (buf)
        // 3. Append a '/' to the end of the base path (buf)
        // 4. Append the name of the cur dir entry (de.name) to the end of the base path (buf)
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        strcpy(p, de.name);
        // The result formate looks like: path/de.name

        // Get the metadata of the cur dir entry using fstat
        int fd_stat = open(buf, 0);
        if (fd_stat < 0) {
            // open(buf, 0): open the cur dir entry (buf) in read-only mode
            // fstat function returns: 0 if successful, -1 if failed

            fprintf(2, "find: cannot open %s\n", buf);
            continue;
        }

        if (fstat(fd_stat, &st) < 0) {
            // open(buf, 0): open the cur dir entry (buf) in read-only mode
            // fstat function returns: 0 if successful, -1 if failed

            // fstat vs stat: to get the metadata, fstat requires fd, stat requires path
            // If already had both, use fstat because it's faster (no need to look up the path again)

            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }

        // Check if the cur dir entry is a directory
        if (st.type == T_DIR) {
            // T_DIR: directory type defined in fs.h
            
            find(buf, target); // Recursively search for files in the cur dir entry (subdir)
        }
        else if (st.type == T_FILE && strcmp(de.name, target) == 0) {
            // T_FILE: file type defined in fs.h
            // strcmp(de.name, target) == 0: the cur dir entry (inside a base dir) matches searched target

            printf("%s\n", buf); // Print the full path of the target file
        }

        close(fd_stat); // Close the cur dir entry
    }
    close(fd); // Close the directory
}

// Main function
int main(int argc, char *argv[]) {
    // Entry point: argc: number of arguments; argv: array of arguments
    // Expected format: find <path> <file> -> argc = 3
    if (argc != 3) {
        fprintf(2, "Usage: find <path> <file>\n");
        // 2 means stderr (standard error output)
        // stderr: used to output error messages (e.g., file not found, permission denied)
        exit(1);
    }

    // Call the find function to search for target in the base dir and its subdirs
    find(argv[1], argv[2]); // argv[1]: char* path; argv[2]: char* target

    exit(0);
}