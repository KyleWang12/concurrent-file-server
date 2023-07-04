#include <errno.h>
#include "server.h"

/**
 * @brief handle a GET command from the client
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void handle_get_command(int client_sock, const char *file_path, USBDevice *usb_devices, int num_usb_devices) {
    char file_buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    FILE *file = NULL;
    char status = 0;

    for (int i = 0; i < num_usb_devices; i++) {
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s%s%s", usb_devices[i].mount_point, usb_devices[i].storage_folder, file_path);

        file = fopen(full_path, "r+");
        if (file) {
            break;
        }
    }

    char message[1024];
    if (!file) {
        snprintf(message, sizeof(message), "%s", strerror(errno));
        send(client_sock, &status, 1, 0); // Send failure status
        send(client_sock, message, strlen(message), 0); // Send errno value
        return;
    }

    // Add read lock on the file
    int fd = fileno(file);
    if (lock_file_read(fd) == -1) {
        perror("ERROR: lock_file_read() failed");
        return;
    }
    status = 1;
    send(client_sock, &status, 1, 0); // Send success status

    while ((bytes_read = fread(file_buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
        if (send(client_sock, file_buffer, bytes_read, 0) < 0) {
            printf("Error: Failed to send file.\n");
            break;
        }
    }

    // Unlock the file
    unlock_file(fd);

    fclose(file);
}