#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <libconfig.h>

#define BUFFER_SIZE 4096

typedef enum {
    INVALID,
    GET,
    INFO,
    MD,
    PUT,
    RM
} CommandType;

typedef struct {
    const char *name;
    CommandType type;
    int arg_count;
} CommandInfo;

/**
 * @brief Prints the usage of the program.
 * 
 * @param prog_name 
 */
void print_usage(const char *prog_name);

/**
 * @brief Parses the command line arguments.
 * 
 * @param argc 
 * @param argv 
 * @param cmd 
 * @return true if the command line arguments are valid.
 * @return false if the command line arguments are invalid.
 */
bool parse_command_line(int argc, char *argv[], CommandType *cmd);

/**
 * @brief Loads the configuration from the config file.
 * 
 * @param config_file 
 * @param host 
 * @param port 
 */
void load_configuration(const char *config_file, char *host, int *port);

#endif // CLIENT_H
