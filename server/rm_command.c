#include "server.h"

/**
 * @brief Handle a RM command from the client
 * 
 * @param client_sock 
 * @param path 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void handle_rm_command(int client_sock, const char *path, USBDevice* usb_devices, const int num_usb_devices) {
    if (strcmp(path, ".") == 0 || strcmp(path, "./") == 0 || strstr(path, "..") != NULL) {
        // Send failure status to the client
        char status = (char) 0;
        if (send(client_sock, &status, 1, 0) < 0) {
            perror("send");
        }

        // Send the error message to the client
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error: Invalid argument");
        if (send(client_sock, error_msg, strlen(error_msg) + 1, 0) < 0) {
            perror("send");
        }
        return;
    }
    struct stat path_stat;
    int success = 0;

    for(int i=0; i<num_usb_devices; i++) {
        char full_file_path[4096];
        memset(full_file_path, '\0', sizeof(full_file_path));
        snprintf(full_file_path, sizeof(full_file_path), "%s%s%s", usb_devices[i].mount_point, usb_devices[i].storage_folder, path);
        if (stat(full_file_path, &path_stat) == -1) {
            continue;
        } else if (S_ISDIR(path_stat.st_mode)) {
            success = delete_directory(full_file_path); // Use full_file_path here
        } else {
            success = remove_file(full_file_path); // Use full_file_path here
        }
    }

    // Send the success status to the client
    char status = (char)success;
    if (send(client_sock, &status, 1, 0) < 0) {
        perror("send");
    }

    // If the operation failed, send the error message
    if (!success) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error: %s", strerror(errno));
        if (send(client_sock, error_msg, strlen(error_msg) + 1, 0) < 0) {
            perror("send");
        }
    }
}
