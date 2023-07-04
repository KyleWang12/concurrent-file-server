CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lpthread -lconfig

SRCS_CLIENT = client.c

OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

TARGET_CLIENT = fget

all: $(TARGET_CLIENT)

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJS_CLIENT) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_CLIENT)