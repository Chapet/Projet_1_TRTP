CC = gcc
CFLAGS =-Wall -Werror -std=c99 -g

sender: main.o sender.o packet_implem.o
	$(CC) -o main.out main.o sender.o packet_implem.o -lz

main.o: main.c sender.h
	$(CC) $(CFLAGS) -o main.o -c main.c

sender.o: sender.c sender.h packet_interface.h
	$(CC) $(CFLAGS) -o sender.o -c sender.c

packet_implem.o: packet_implem.c packet_interface.h
	$(CC) $(CFLAGS) -o packet_implem.o -c packet_implem.c

all:
	make
	#make tests

    # insert tests here

#.PHONY : clean tests

clean:
    rm -rf sender *.o