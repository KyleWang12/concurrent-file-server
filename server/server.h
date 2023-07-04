#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <libconfig.h>
#include <sys/inotify.h>

#define BUFFER_SIZE 4096
#define MAX_USB_DEVICES 16

typedef struct USBDevice {
    char label[256];
    char mount_point[256];
    char storage_folder[256];
} USBDevice;

int lock_file_read(int fd);
int lock_file_write(int fd);
int unlock_file(int fd);

/**
 * @brief Handle a GET command from the client
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices
 * @return int 
 */
void handle_get_command(int client_sock, const char *file_path, USBDevice* usb_devices, const int num_usb_devices);

/**
 * @brief Handle a PUT command from the client
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices
 * @return int 
 */
void handle_info_command(int client_sock, const char *file_path, USBDevice* usb_devices, const int num_usb_devices);

/**
 * @brief Handle a PUT command from the client
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices
 * @return int 
 */
void handle_md_command(int client_sock, const char *new_folder, USBDevice* usb_devices, const int num_usb_devices);

/**
 * @brief Handle a PUT command from the client
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices
 * @return int 
 */
void handle_put_command(int client_sock, const char *file_path, USBDevice* usb_devices, const int num_usb_devices);

/**
 * @brief Handle a RM command from the client
 * 
 * @param client_sock 
 * @param path 
 * @param usb_devices 
 * @param num_usb_devices
 * @return int 
 */
void handle_rm_command(int client_sock, const char *path, USBDevice* usb_devices, const int num_usb_devices);

/**
 * @brief Remove a file from the filesystem
 * 
 * @param path 
 * @return int 
 */
int remove_file(const char *path);

/**
 * @brief Delete a directory from the filesystem
 * 
 * @param path 
 * @return int 
 */
int delete_directory(const char *path);

/**
 * @brief Copy a file from one location to another
 * 
 * @param src 
 * @param dst 
 * @return int 
 */
int copy_file(const char *src, const char *dst);

/**
 * @brief Copy a directory from one location to another
 * 
 * @param src 
 * @param dst 
 * @return int 
 */
int copy_directory(const char *src, const char *dst);

#endif