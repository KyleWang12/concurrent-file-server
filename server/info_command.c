#include <errno.h>
#include "server.h"

/**
 * @brief Handle an INFO command from the client
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void handle_info_command(int client_sock, const char *file_path, USBDevice* usb_devices, const int num_usb_devices) {

    struct stat file_stat;
    char file_info[4096];

    for(int i = 0; i < num_usb_devices; i++) {
        char full_file_path[4096];
        memset(full_file_path, '\0', sizeof(full_file_path));
        snprintf(full_file_path, sizeof(full_file_path), "%s%s%s", usb_devices[i].mount_point, usb_devices[i].storage_folder, file_path);

        if (stat(full_file_path, &file_stat) == -1) {
            continue;
        } else {
            struct passwd *user = getpwuid(file_stat.st_uid);
            struct group *group = getgrgid(file_stat.st_gid);
            char mod_time[20];
            char permissions[11];

            strftime(mod_time, sizeof(mod_time), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));

            // Convert permissions to ls -l style
            snprintf(permissions, sizeof(permissions),
                    "%c%c%c%c%c%c%c%c%c%c",
                    S_ISDIR(file_stat.st_mode) ? 'd' : '-',
                    file_stat.st_mode & S_IRUSR ? 'r' : '-',
                    file_stat.st_mode & S_IWUSR ? 'w' : '-',
                    file_stat.st_mode & S_IXUSR ? 'x' : '-',
                    file_stat.st_mode & S_IRGRP ? 'r' : '-',
                    file_stat.st_mode & S_IWGRP ? 'w' : '-',
                    file_stat.st_mode & S_IXGRP ? 'x' : '-',
                    file_stat.st_mode & S_IROTH ? 'r' : '-',
                    file_stat.st_mode & S_IWOTH ? 'w' : '-',
                    file_stat.st_mode & S_IXOTH ? 'x' : '-');

            snprintf(file_info, sizeof(file_info),
                    "File: %s\n"
                    "Size: %ld bytes\n"
                    "Permissions: %s\n"
                    "Owner: %s\n"
                    "Group: %s\n"
                    "Last modified: %s\n",
                    file_path, file_stat.st_size, permissions,
                    user->pw_name, group->gr_name, mod_time);
            char status = 1;
            if (send(client_sock, &status, sizeof(status), 0) < 0) {
                perror("ERROR: send() failed");
                return;
            }

            // Send the file information back to the client
            if (send(client_sock, file_info, strlen(file_info) + 1, 0) < 0) {
                perror("ERROR: send() failed");
            }
            return;
        }
    }
    
    // When the loop ends, an error occurred
    snprintf(file_info, sizeof(file_info), "ERROR: %s", strerror(errno));
    char status = 0;
    if (send(client_sock, &status, sizeof(status), 0) < 0) {
        perror("ERROR: send() failed");
        return;
    }
    if (send(client_sock, file_info, strlen(file_info) + 1, 0) < 0) {
        perror("ERROR: send() failed");
    }
}