CC = gcc
CFLAGS = -I. -g -Wall
OBJS = shared.o
.SUFFIXES: .c .o

all: oss user

oss: oss.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ oss.o $(OBJS) -pthread

user: user.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ user.o $(OBJS) -pthread

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONE: clean
clean:
	rm *.o *.txt oss user
