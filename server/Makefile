CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lpthread -lconfig

SRCS_SERVER = server.c get_command.c info_command.c md_command.c put_command.c rm_command.c lock.c utils.c

OBJS_SERVER = $(SRCS_SERVER:.c=.o)

TARGET_SERVER = server
TARGET_CLIENT = fget

all: $(TARGET_SERVER)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJS_SERVER) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_SERVER)