all: server play curses cursesColors map

server:server.c
	c99 -Wall -pthread -D_GNU_SOURCE -o serve server.c
	chmod 770 serve

play:client.c
	c99 -Wall -D_GNU_SOURCE -o play client.c -lncurses
	chmod 770 play

curses:curses.c
	c99 -Wall -o curses curses.c -lncurses
	chmod 770 curses

cursesColors:cursesColors.c
	c99 -Wall -o cursesColors cursesColors.c -lncurses
	chmod 770 cursesColors

map:map.c
	c99 -Wall -o map map.c
	chmod 770 map
