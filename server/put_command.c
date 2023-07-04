#include <errno.h>
#include "server.h"

/**
 * @brief Handle a PUT command from the client
 * 
 * @param client_sock 
 * @param file_name 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void handle_put_command(int client_sock, const char *file_name, USBDevice* usb_devices, const int num_usb_devices) {
    // Send ACK to client
    char ack = 1;
    if (send(client_sock, &ack, 1, 0) < 0) {
        perror("send");
        return;
    }

    // Receive file size from the client
    long file_size;
    if (recv(client_sock, &file_size, sizeof(file_size), 0) < 0) {
        perror("recv");
        return;
    }
    file_size = ntohl(file_size);  // Convert to host byte order

    // Open the file for writing on each USB device
    int fds[num_usb_devices];
    for (int i = 0; i < num_usb_devices; i++) {
        char full_file_path[4096];
        memset(full_file_path, '\0', sizeof(full_file_path));
        snprintf(full_file_path, sizeof(full_file_path), "%s%s%s", usb_devices[i].mount_point, usb_devices[i].storage_folder, file_name);

        fds[i] = open(full_file_path, O_WRONLY | O_CREAT, 0644);
        if (fds[i] != -1) {
            lock_file_write(fds[i]);
        }
    }

    // Receive file data from the client and write to all available USB devices
    ssize_t recv_size;
    char buffer[BUFFER_SIZE];
    long bytes_received = 0;
    while (bytes_received < file_size) {
        recv_size = recv(client_sock, buffer, (BUFFER_SIZE < (file_size - bytes_received)) ? BUFFER_SIZE : (file_size - bytes_received), 0);
        if (recv_size <= 0) {
            // Connection closed or error
            break;
        }
        for (int i = 0; i < num_usb_devices; i++) {
            if (fds[i] != -1) {
                write(fds[i], buffer, recv_size);
            }
        }
        bytes_received += recv_size;
    }

    // Close the files on all USB devices
    for (int i = 0; i < num_usb_devices; i++) {
        if (fds[i] != -1) {
            unlock_file(fds[i]);
            close(fds[i]);
        }
    }

    // Send a success message to the client
    char status = (bytes_received == file_size) ? 1 : 0;
    if (send(client_sock, &status, 1, 0) < 0) {
        perror("send");
    }
}