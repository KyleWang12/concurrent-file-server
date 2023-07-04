#include <errno.h>
#include "server.h"

/**
 * @brief Handle a MD command from the client
 * 
 * @param client_sock 
 * @param new_folder 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void handle_md_command(int client_sock, const char *new_folder, USBDevice* usb_devices, const int num_usb_devices) {
    char error_message[256];

    for(int i=0; i<num_usb_devices; i++) {
        char full_file_path[4096];
        memset(full_file_path, '\0', sizeof(full_file_path));
        snprintf(full_file_path, sizeof(full_file_path), "%s%s%s", usb_devices[i].mount_point, usb_devices[i].storage_folder, new_folder);

        if (mkdir(full_file_path, 0755) == -1) {
            continue;
        } else {
            char status = 1;
            if (send(client_sock, &status, sizeof(status), 0) < 0) {
                perror("ERROR: send() failed");
                return;
            }
            return;
        }
    }
    char status = 0;
    if (send(client_sock, &status, sizeof(status), 0) < 0) {
        perror("ERROR: send() failed");
        return;
    }
    snprintf(error_message, sizeof(error_message), "%s", strerror(errno));
    if (send(client_sock, error_message, strlen(error_message) + 1, 0) < 0) {
        perror("ERROR: send() failed");
    }
}
