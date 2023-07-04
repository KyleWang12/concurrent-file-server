#include "server.h"

static int socket_desc;
static char host[INET_ADDRSTRLEN] = {0};
static int port;
static USBDevice usb_devices[MAX_USB_DEVICES];
static int num_usb_devices = 0;

/**
 * @brief Signal handler for SIGINT, releases the socket and exits the program.
 * 
 * @param sig 
 */
void handle_sigint(int sig) {
    printf("\nCaught signal %d. Closing the socket and exiting.\n", sig);
    close(socket_desc);
    exit(0);
}

/**
 * @brief Loads the configuration from the specified file.
 * 
 * @param config_file 
 * @param host 
 * @param port 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void load_configuration(const char *config_file, char *host, int *port, USBDevice *usb_devices, int *num_usb_devices) {
    config_t cfg;
    config_setting_t *setting;

    config_init(&cfg);

    if (!config_read_file(&cfg, config_file)) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(EXIT_FAILURE);
    }

    // Read host
    const char *host_str;
    if (config_lookup_string(&cfg, "host", &host_str)) {
        strncpy(host, host_str, INET_ADDRSTRLEN);
    }

    // Read port
    config_lookup_int(&cfg, "port", port);

    // Read usb_devices
    setting = config_lookup(&cfg, "usb_devices");
    if (setting) {
        *num_usb_devices = config_setting_length(setting);
        if (*num_usb_devices > MAX_USB_DEVICES) {
            fprintf(stderr, "Error: The number of USB devices in the configuration exceeds the maximum allowed (%d). Please update the configuration.\n", MAX_USB_DEVICES);
            config_destroy(&cfg);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < *num_usb_devices; i++) {
            config_setting_t *usb_device_setting = config_setting_get_elem(setting, i);

            const char *label;
            const char *mount_point;
            const char *storage_folder;

            if (config_setting_lookup_string(usb_device_setting, "label", &label)) {
                strncpy(usb_devices[i].label, label, sizeof(usb_devices[i].label));
            } else {
                // If the label is not provided, set it to an empty string or a default value
                strncpy(usb_devices[i].label, "", sizeof(usb_devices[i].label));
            }

            config_setting_lookup_string(usb_device_setting, "mount_point", &mount_point);
            if (config_setting_lookup_string(usb_device_setting, "storage_folder", &storage_folder)) {
                strncpy(usb_devices[i].storage_folder, storage_folder, sizeof(usb_devices[i].storage_folder));
            } else {
                strncpy(usb_devices[i].storage_folder, "", sizeof(usb_devices[i].storage_folder));
            }

            strncpy(usb_devices[i].mount_point, mount_point, sizeof(usb_devices[i].mount_point));
        }
    }

    config_destroy(&cfg);
}

/**
 * @brief Get the mount point from the device name.
 * 
 * @param dev_name 
 * @param mount_point 
 * @return int 
 */
int get_mount_point(const char *dev_name, char *mount_point) {
    int max_retries = 5;
    int retry_interval_ms = 500;
    int found_mount_point = 0;

    for (int attempt = 0; attempt < max_retries; ++attempt) {
        FILE *fp = fopen("/proc/mounts", "r");
        if (!fp) {
            perror("fopen");
            return 0;
        }

        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            char device[64], mp[128], fs_type[64], options[128];
            sscanf(line, "%63s %127s %63s %127s", device, mp, fs_type, options);

            if (strcmp(device, dev_name) == 0) {
                strcpy(mount_point, mp);
                found_mount_point = 1;
                break;
            }
        }
        fclose(fp);

        if (found_mount_point) {
            break;
        }

        // Sleep for the specified interval before retrying
        struct timespec sleep_time;
        sleep_time.tv_sec = retry_interval_ms / 1000;
        sleep_time.tv_nsec = (retry_interval_ms % 1000) * 1000000L;
        nanosleep(&sleep_time, NULL);
    }

    if (!found_mount_point) {
        mount_point[0] = '\0';  // Set mount_point to an empty string
        return 0;
    }
    return 1;
}

/**
 * @brief Get the index of the USB device with the specified mount point.
 * 
 * @param mount_point 
 * @return int 
 */
int usb_in_list(const char *mount_point) {
    for (int i = 0; i < num_usb_devices; ++i) {
        if (strcmp(usb_devices[i].mount_point, mount_point) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Syncs the files from the source USB to the destination USB.
 * 
 * @param idx 
 * @param usb_devices 
 */
void sync_usb(int idx, USBDevice *usb_devices) {
    for (int i = 0; i < MAX_USB_DEVICES; ++i) {
        if (i != idx && usb_devices[i].mount_point[0] != '\0') {
            // sync_usb files from source USB (i) to available USB (idx)
            char src_root[256], dst_root[256];
            snprintf(src_root, sizeof(src_root), "%.*s/%.*s", (int)(sizeof(src_root) / 2 - 1), usb_devices[i].mount_point, (int)(sizeof(src_root) / 2 - 1), usb_devices[i].storage_folder);
            snprintf(dst_root, sizeof(dst_root), "%.*s/%.*s", (int)(sizeof(dst_root) / 2 - 1), usb_devices[idx].mount_point, (int)(sizeof(dst_root) / 2 - 1), usb_devices[idx].storage_folder);

            // Clean up the destination directory before sync_usbing
            if (!delete_directory(dst_root)) {
                perror("remove_directory");
            }

            // Copy the contents of the source directory to the destination directory
            if (!copy_directory(src_root, dst_root)) {
                perror("copy_directory");
            }
            break;
        }
    }
}

/**
 * @brief Monitor the /dev directory for USB device events.
 * 
 * @param arg 
 * @return void* 
 */
void *usb_monitor(void *arg) {
    int inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
        return NULL;
    }

    int watch_fd = inotify_add_watch(inotify_fd, "/dev", IN_CREATE | IN_DELETE);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return NULL;
    }

    // char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    char buffer[4096];
    while (1) {
        ssize_t len = read(inotify_fd, buffer, sizeof(buffer));
        if (len < 0) {
            perror("read");
            break;
        }

        const struct inotify_event *event;
        char *ptr;
        for (ptr = buffer; ptr < buffer + len; ptr += sizeof(struct inotify_event) + event->len) {
            event = (const struct inotify_event *)ptr;

            if (event->mask & IN_CREATE) {
                // USB device added
                char dev_path[64];
                snprintf(dev_path, sizeof(dev_path), "/dev/%s", event->name);
                char mount_point[128];
                int found = get_mount_point(dev_path, mount_point);
                if (!found) continue;
                int idx = usb_in_list(mount_point);
                if (idx >= 0) {
                    sync_usb(idx, usb_devices);
                }
            }
        }
    }

    // Clean up
    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);
    return NULL;
}

/**
 * @brief Client handler thread.
 * 
 * @param client_sock 
 * @param file_path 
 * @param usb_devices 
 * @param num_usb_devices 
 */
void *client_handler(void *arg) {
    int client_sock = *(int *)arg;
    char client_message[BUFFER_SIZE];

    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0) {
        perror("Recv failed1");
        close(client_sock);
        pthread_exit(NULL);
    }

    char command[16], file_path[2048];
    memset(command, '\0', sizeof(command));
    memset(file_path, '\0', sizeof(file_path));
    sscanf(client_message, "%s %s", command, file_path);

    if (strcmp(command, "GET") == 0) {
        handle_get_command(client_sock, file_path, usb_devices, num_usb_devices);
    } else if (strcmp(command, "INFO") == 0) {
        handle_info_command(client_sock, file_path, usb_devices, num_usb_devices);
    } else if (strcmp(command, "MD") == 0) {
        handle_md_command(client_sock, file_path, usb_devices, num_usb_devices);
    } else if (strcmp(command, "PUT") == 0) {
        handle_put_command(client_sock, file_path, usb_devices, num_usb_devices);
    } else if (strcmp(command, "RM") == 0) {
        handle_rm_command(client_sock, file_path, usb_devices, num_usb_devices);
    } else {
        printf("Unknown command: %s\n", client_message);
    }

    memset(client_message, '\0', sizeof(client_message));
    close(client_sock);

    pthread_exit(NULL);
}

/**
 * @brief Accept connections from clients.
 * 
 * @param server_socket 
 */
void accept_connections(int server_socket) {
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len)) >= 0) {
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        int *client_socket_ptr = malloc(sizeof(int));
        if (client_socket_ptr == NULL) {
            perror("Malloc failed");
            continue;
        }

        *client_socket_ptr = client_socket;

        printf("Client connected at IP: %s and port: %i\n",
               inet_ntoa(client_address.sin_addr),
               ntohs(client_address.sin_port));

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_handler, client_socket_ptr) != 0) {
            perror("Thread creation failed");
            free(client_socket_ptr);
            continue;
        }
    }
}

int main(void) {
    // Register the signal handler for SIGINT
    signal(SIGINT, handle_sigint);

    load_configuration("server.conf", host, &port, usb_devices, &num_usb_devices);

    pthread_t usb_monitor_thread;
    if (pthread_create(&usb_monitor_thread, NULL, usb_monitor, NULL) != 0) {
        printf("Failed to create USB monitor thread\n");
        return -1;
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 1024) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", port);

    accept_connections(server_socket);

    close(server_socket);  
    return 0;
}