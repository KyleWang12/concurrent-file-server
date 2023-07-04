# File Transfer Client

This is a simple file transfer client that communicates with a remote file server. The client allows you to perform various operations on files, such as GET, INFO, MD, PUT, and RM.

## Features

- Get files from a remote server
- Upload files to a remote server
- Create directories on the remote server
- Retrieve information about files on the remote server
- Remove files from the remote server

## Prerequisites

- GCC or any other C compiler
- [libconfig](https://github.com/hyperrealm/libconfig) library

## Compilation

Compile the client using the following command:

```sh
$ make
$ ./fget <command> <args>