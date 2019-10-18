
main.out: main.o sender.o packet_implem.o
	gcc -o main.out main.o sender.o packet_implem.o -lz

main.o: main.c sender.h
	gcc -o main.o -c main.c -W -Wall -Werror

sender.o: sender.c sender.h packet_interface.h
	gcc -o sender.o -c sender.c -W -Wall -Werror

packet_implem.o: packet_implem.c packet_interface.h
	gcc -o packet_implem.o -c packet_implem.c -W -Wall -Werror

