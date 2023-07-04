#include "server.h"

/**
 * @brief Remove a file from the filesystem
 * 
 * @param path 
 * @return int 
 */
int remove_file(const char *path) {
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }
    if (lock_file_write(fd) == -1) {
        perror("lock_file_write");
        close(fd);
        return 0;
    }
    if (remove(path) == 0) {
        unlock_file(fd);
        close(fd);
        return 1;
    } else {
        perror("remove");
        unlock_file(fd);
        close(fd);
        return 0;
    }
}

/**
 * @brief Delete a directory from the filesystem
 * 
 * @param path 
 * @return int 
 */
int delete_directory(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    int success = 1;
    struct dirent *entry;
    char entry_path[2048];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(entry_path, sizeof(entry_path), "%s/%s", path, entry->d_name);

        struct stat entry_stat;
        if (stat(entry_path, &entry_stat) == -1) {
            perror("stat");
            success = 0;
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            success = delete_directory(entry_path) && success;
        } else {
            success = remove_file(entry_path) && success;
        }
    }

    closedir(dir);

    if (success) {
        if (rmdir(path) == -1) {
            perror("rmdir");
            success = 0;
        }
    }

    return success;
}

/**
 * @brief Copy a file from one location to another
 * 
 * @param src 
 * @param dst 
 * @return int 
 */
int copy_file(const char *src, const char *dst) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror("open src");
        return -1;
    }

    if (lock_file_read(src_fd) == -1) {
        perror("lock_file_read src");
        close(src_fd);
        return -1;
    }

    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("open dst");
        unlock_file(src_fd);
        close(src_fd);
        return -1;
    }

    if (lock_file_write(dst_fd) == -1) {
        perror("lock_file_write dst");
        unlock_file(src_fd);
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    char buf[4096];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(src_fd, buf, sizeof(buf))) > 0) {
        char *ptr = buf;
        do {
            bytes_written = write(dst_fd, ptr, bytes_read);
            if (bytes_written >= 0) {
                bytes_read -= bytes_written;
                ptr += bytes_written;
            } else {
                perror("write");
                unlock_file(src_fd);
                unlock_file(dst_fd);
                close(src_fd);
                close(dst_fd);
                return -1;
            }
        } while (bytes_read > 0);
    }

    if (bytes_read < 0) {
        perror("read");
    }

    unlock_file(src_fd);
    unlock_file(dst_fd);
    close(src_fd);
    close(dst_fd);
    return 0;
}

/**
 * @brief Copy a directory from one location to another
 * 
 * @param src 
 * @param dst 
 * @return int 
 */
int copy_directory(const char *src, const char *dst) {
    DIR *dir = opendir(src);
    if (!dir) {
        perror("opendir");
        return 0;
    }

    if (mkdir(dst, 0755) < 0 && errno != EEXIST) {
        perror("mkdir");
        closedir(dir);
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024], dst_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        struct stat st;
        if (lstat(src_path, &st) < 0) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (copy_directory(src_path, dst_path) < 0) {
                perror("copy_directory");
            }
        } else if (S_ISREG(st.st_mode)) {
            if (copy_file(src_path, dst_path) < 0) {
                perror("copy_file");
            }
        }
    }

    closedir(dir);
    return 1;
}