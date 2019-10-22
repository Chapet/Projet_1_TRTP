CC = gcc
CFLAGS =-Wall -Werror -std=gnu99

sender: main.o sender.o packet_implem.o
	$(CC) -o sender main.o sender.o packet_implem.o -lz

main.o: main.c sender.h
	$(CC) $(CFLAGS) -o main.o -c main.c

sender.o: sender.c sender.h packet_interface.h
	$(CC) $(CFLAGS) -o sender.o -c sender.c

packet_implem.o: packet_implem.c packet_interface.h
	$(CC) $(CFLAGS) -o packet_implem.o -c packet_implem.c

tests: tests.o sender.o packet_implem.o
	$(CC) -o tests tests.o sender.o packet_implem.o -lz -lcunit
	make clean
	./tests

tests.o: tests.c sender.h
	$(CC) $(CFLAGS) -o tests.o -c tests.c -lcunit

all:
	make
	make clean

clean:
	rm -rf *.o