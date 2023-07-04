# Server

Server is a multi-threaded server application that allows clients to interact with USB devices connected to the server. The server monitors the addition of USB devices and syncs their contents with other connected USB devices. It also exposes a set of commands that clients can use to interact with the USB devices, such as retrieving file information, creating directories, and uploading or deleting files.

## Features

- Automatically syncs files between USB devices when a new device is connected
- Supports commands for clients to interact with USB devices:
  - `GET`: Download a file from the server
  - `INFO`: Get file information
  - `MD`: Create a directory
  - `PUT`: Upload a file to the server
  - `RM`: Delete a file or directory
- Configurable through a configuration file

## Requirements

- C compiler (e.g., GCC)
- [libconfig](https://github.com/hyperrealm/libconfig) library

## How to run

Compile and run the server with the following command:
```
$ make
$ ./server
```