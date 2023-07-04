#include "client.h"

static char host[INET_ADDRSTRLEN] = {0};
static int port;

static CommandInfo commands[] = {
    {"GET", GET, 3},
    {"GET", GET, 4},
    {"INFO", INFO, 3},
    {"MD", MD, 3},
    {"PUT", PUT, 3},
    {"PUT", PUT, 4},
    {"RM", RM, 3},
};

/**
 * @brief Prints the usage of the program.
 * 
 * @param prog_name 
 */
void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("%s GET optional[<remote_file_path>] <local_file_path>\n", prog_name);
    printf("%s INFO <remote_file_path>\n", prog_name);
    printf("%s MD <remote_folder_path>\n", prog_name);
    printf("%s PUT <local_file_path> optional[<remote_file_path>]\n", prog_name);
    printf("%s RM <remote_file_path>\n", prog_name);
}

/**
 * @brief Parses the command line arguments.
 * 
 * @param argc 
 * @param argv 
 * @param cmd 
 * @return true if the command line arguments are valid.
 * @return false if the command line arguments are invalid.
 */
bool parse_command_line(int argc, char *argv[], CommandType *cmd) {
    if(argc < 2) {
        print_usage(argv[0]);
        return false;
    }
    *cmd = INVALID;
    size_t commands_count = sizeof(commands) / sizeof(CommandInfo);
    for (size_t i = 0; i < commands_count; i++) {
        if (strcmp(argv[1], commands[i].name) == 0 && argc == commands[i].arg_count) {
            *cmd = commands[i].type;
            break;
        }
    }

    if (*cmd == INVALID) {
        print_usage(argv[0]);
        return false;
    }

    return true;
}

/**
 * @brief Loads the configuration from the config file.
 * 
 * @param config_file 
 * @param host 
 * @param port 
 */
void load_configuration(const char *config_file, char *host, int *port) {
    config_t cfg;

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

    config_destroy(&cfg);
}

int main(int argc, char *argv[]) {

    load_configuration("client.conf", host, &port);
    
    CommandType cmd;

    if (!parse_command_line(argc, argv, &cmd)) {
        return -1;
    }

    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[BUFFER_SIZE], client_message[BUFFER_SIZE];

    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        printf("Unable to create socket\n");
        close(socket_desc);
        return -1;
    }

    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    // Send connection request to server:
    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Unable to connect\n");
        close(socket_desc);
        return -1;
    }

    switch (cmd) {
        case GET: {
            // Prepare and send the command message:
            snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[2]);
            if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
                printf("Unable to send message\n");
                close(socket_desc);
                return -1;
            }

            // Receive the server's response (success or failure):
            char status;
            if (recv(socket_desc, &status, 1, 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }

            if (status == 0) {
                // Receive and print the error message from the server:
                if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
                    printf("Error while receiving server's error msg\n");
                    close(socket_desc);
                    return -1;
                }
                printf("%s\n", server_message);
                close(socket_desc);
                return -1;
            }

            // Save the file data to a local file:
            FILE *file = fopen(argv[argc - 1], "wb");
            if (file == NULL) {
                perror("fopen");
                close(socket_desc);
                return -1;
            }
            ssize_t recv_size;
            while ((recv_size = recv(socket_desc, server_message, BUFFER_SIZE, 0)) > 0) {
                fwrite(server_message, 1, recv_size, file);
                memset(server_message, '\0', sizeof(server_message));
            }
            fclose(file);
            printf("File saved successfully: %s\n", argv[argc - 1]);
            break;
        }
        case INFO: {
            // Prepare and send the command message:
            snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[2]);
            if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
                printf("Unable to send message\n");
                close(socket_desc);
                return -1;
            }

            // Receive the server's response (success or failure):
            char status;
            if (recv(socket_desc, &status, 1, 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }

            if (status == 0) {
                // Receive and print the error message from the server:
                if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
                    printf("Error while receiving server's error msg\n");
                    close(socket_desc);
                    return -1;
                }
                printf("%s\n", server_message);
                close(socket_desc);
                return -1;
            }
            // Receive the file information from the server:
            if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }
            // Print the file information from the server:
            printf("File information:\n%s\n", server_message);
            break;
        }
        case MD: {
            // Prepare and send the command message:
            snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[2]);
            if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
                printf("Unable to send message\n");
                close(socket_desc);
                return -1;
            }

            // Receive the server's response (success or failure):
            char status;
            if (recv(socket_desc, &status, 1, 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }

            if (status == 0) {
                // Receive and print the error message from the server:
                if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
                    printf("Error while receiving server's error msg\n");
                    close(socket_desc);
                    return -1;
                }
                printf("%s\n", server_message);
                close(socket_desc);
                return -1;
            }
            printf("Folder created successfully: %s\n", argv[2]);
            break;
        }
        case PUT: {
            // Prepare and send the command message:
            snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[argc - 1]);
            if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
                printf("Unable to send message\n");
                close(socket_desc);
                return -1;
            }

            // recv ack from client
            char ack;
            if (recv(socket_desc, &ack, 1, 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }

            if (ack != 1) {
                printf("Server is not ready to receive file content\n");
                close(socket_desc);
                return -1;
            }

            // Send local file
            FILE *file = fopen(argv[2], "rb");
            if (file == NULL) {
                printf("Error reading file %s\n", argv[2]);
                close(socket_desc);
                return -1;
            }

            // Send file size
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            long file_size_network = htonl(file_size);
            if (send(socket_desc, &file_size_network, sizeof(file_size_network), 0) < 0) {
                printf("Unable to send file size\n");
                close(socket_desc);
                return -1;
            }

            int read_size;
            memset(client_message, '\0', sizeof(client_message));
            while ((read_size = fread(client_message, 1, BUFFER_SIZE, file)) > 0) {
                send(socket_desc, client_message, read_size, 0);
                memset(client_message, '\0', sizeof(client_message));
            }
            fclose(file);

            // Receive the server's response (success or failure):
            char status;
            if (recv(socket_desc, &status, 1, 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }

            if (status == 0) {
                // Receive and print the error message from the server:
                if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
                    printf("Error while receiving server's error msg\n");
                    close(socket_desc);
                    return -1;
                }
                printf("%s\n", server_message);
                close(socket_desc);
                return -1;
            }

            printf("File sent successfully: %s\n", argv[2]);
            break;
        }

        case RM: {
             // Prepare and send the command message:
            snprintf(client_message, sizeof(client_message), "%s %s", argv[1], argv[2]);
            if (send(socket_desc, client_message, strlen(client_message), 0) < 0) {
                printf("Unable to send message\n");
                close(socket_desc);
                return -1;
            }

            // Receive the server's response (success or failure):
            char status;
            if (recv(socket_desc, &status, 1, 0) < 0) {
                printf("Error while receiving server's msg\n");
                close(socket_desc);
                return -1;
            }

            if (status == 0) {
                // Receive and print the error message from the server:
                if (recv(socket_desc, server_message, sizeof(server_message), 0) < 0) {
                    printf("Error while receiving server's error msg\n");
                    close(socket_desc);
                    return -1;
                }
                printf("%s\n", server_message);
                close(socket_desc);
                return -1;
            }
            printf("File deleted successfully: %s\n", argv[2]);
            break;
        }
        default:
            break;
    }
    close(socket_desc);

    return 0;
}
