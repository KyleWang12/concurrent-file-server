#include "server.h"

int lock_file_read(int fd) {
    struct flock lock;
    lock.l_type = F_RDLCK; // Read lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock the entire file

    return fcntl(fd, F_SETLK, &lock);
}

int lock_file_write(int fd) {
    struct flock lock;
    lock.l_type = F_WRLCK; // Write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock the entire file

    return fcntl(fd, F_SETLK, &lock);
}

int unlock_file(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK; // Unlock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Unlock the entire file

    return fcntl(fd, F_SETLK, &lock);
}