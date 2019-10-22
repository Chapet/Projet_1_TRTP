CC = gcc
CFLAGS =-Wall -Werror -std=gnu99

sender: src/main.o src/sender.o src/packet_implem.o
	$(CC) -o sender src/main.o src/sender.o src/packet_implem.o -lz

main.o: src/main.c src/sender.h
	$(CC) $(CFLAGS) -o src/main.o -c src/main.c

sender.o: src/sender.c src/sender.h src/packet_interface.h
	$(CC) $(CFLAGS) -o src/sender.o -c src/sender.c

packet_implem.o: src/packet_implem.c src/packet_interface.h
	$(CC) $(CFLAGS) -o src/packet_implem.o -c src/packet_implem.c

tests: tests/tests.o src/sender.o src/packet_implem.o
	$(CC) -o tests tests/tests.o src/sender.o src/packet_implem.o -lz -lcunit
	make clean
	./tests
	rm tests

tests.o: tests/tests.c tests/sender.h
	$(CC) $(CFLAGS) -o tests/tests.o -c tests/tests.c -lcunit

all:
	make
	make clean

clean:
	rm -rf *.o
