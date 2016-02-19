.SUFFIXES:.c .o
CC=gcc
SRCS=server.c\
	pub.c\
	work.c\
	crud.c
OBJS=$(SRCS:.c=.o)
FLAGS= -Wall -g
EXEC=minihttpd
start:$(OBJS)
	$(CC) $(OBJS) -o $(EXEC) -lpthread -lmysqlclient
	@echo '----------link ok--------------'

.c.o:
	$(CC) $(FLAGS) -c $< -o $@
	@echo '----------complile ok--------------'
clean:
	rm -f $(OBJS)
	rm -f core*
	rm -f $(EXEC)
