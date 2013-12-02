all: cursesColors map server play

cursesColors:cursesColors.c
	c99 -Wall -o cursesColors cursesColors.c -lncurses
	chmod 770 cursesColors

map:map.c
	c99 -c map.c
	chmod 770 map.o
	chmod 770 map.c

server:server.c map.o
	c99 -Wall -pthread -D_GNU_SOURCE -o serve server.c map.o -lncurses
	chmod 770 serve

play:client.c map.o
	c99 -Wall -D_GNU_SOURCE -o play client.c map.o -lncurses
	chmod 770 play
