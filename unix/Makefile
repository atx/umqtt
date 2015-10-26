
CC = gcc
RM = rm
UMQTT = ../umqtt

CFLAGS = -I$(UMQTT) -I.

SRC = $(UMQTT)/umqtt.c
SRC += main.c

umqtt: $(SRC:.c=.o)
	$(CC) $(CFLAGS) $^ -o $@

all: umqtt

clean:
	$(RM) -f *.o umqtt
