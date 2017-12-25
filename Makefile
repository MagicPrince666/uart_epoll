EXEC	= test_epoll
OBJS    = com.o main.o 
SRC     = com.cpp main.cpp

CROSS	= mipsel-openwrt-linux-uclibc-
CC	     = $(CROSS)g++ 
STRIP	= $(CROSS)strip
CFLAGS	= -g -Wall
LDFLAGS += -lm -lpthread

all:  clean $(EXEC) server

server:server.o
	$(CC) $(LDFLAGS) -o $@ server.o 
	
$(EXEC):$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) 
	$(STRIP) $@
%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	-rm -f $(EXEC) server *.o
