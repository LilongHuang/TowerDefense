#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ncurses.h>

char buffer[1024];
char mapName[1024];
char authorName[1024];
char castleStrength[1024];
char castleTouch[1024];
char attackerWin[1024];
char attackerShots[1024];
char attackerRespawn[1024];
char defenderWin[1024];
char defenderShots[1024];
char defenderRespawn[1024];
char map[2048];

char castle[1024];
char attacker[1024];
char defender[1024];

char* getMap() {
	return map;
}

char* getMapName() {
	return mapName;
}

char* getAuthor() {
	return authorName;
}

int getCastleStrength() {
	char* subBuffer = strstr(castle, "strength");
	subBuffer += 9;
	char* subBuffer2 = strstr(castle, "touch");
	int diffBytes = subBuffer2 - subBuffer;
	
	strncpy(castleStrength, subBuffer, diffBytes - 1);
	int value = atoi(castleStrength);
	return value;
}

int getCastleTouch() {
	char* subBuffer = strstr(castle, "touch");
        subBuffer += 6;

	strncpy(castleTouch, subBuffer, strlen(subBuffer) - 1);
	int value = atoi(castleTouch);
        return value;
}

int getAttackerWin() {
        char* subBuffer = strstr(attacker, "destroy");
        subBuffer += 8;
	char* subBuffer2 = strstr(attacker, "shots");
        int diffBytes = subBuffer2 - subBuffer;

	strncpy(attackerWin, subBuffer, diffBytes - 2);
	int value = atoi(attackerWin);
        return value;
}

int getAttackerShots() {
        char* subBuffer = strstr(attacker, "shot");
        subBuffer += 6;
	char* subBuffer2 = strstr(attacker, "respawn");
        int diffBytes = subBuffer2 - subBuffer;

	strncpy(attackerShots, subBuffer, diffBytes - 1);
	int value = atoi(attackerShots);
        return value;
}

int getAttackerRespawn() {
        char* subBuffer = strstr(attacker, "respawn");
        subBuffer += 8;

	strncpy(attackerRespawn, subBuffer, strlen(subBuffer) - 1);
	int value = atoi(attackerRespawn);
        return value;
}

int getDefenderWin() {
        char* subBuffer = strstr(defender, "survive");
        subBuffer += 8;
	char* subBuffer2 = strstr(defender, "shot");
        int diffBytes = subBuffer2 - subBuffer;

	strncpy(defenderWin, subBuffer, diffBytes - 2);
	int value = atoi(defenderWin);
        return value;
}

int getDefenderShots() {
        char* subBuffer = strstr(defender, "shot");
        subBuffer += 6;
	char* subBuffer2 = strstr(defender, "respawn");
        int diffBytes = subBuffer2 - subBuffer;

	strncpy(defenderShots, subBuffer, diffBytes - 1);
	int value = atoi(defenderShots);
        return value;
}

int getDefenderRespawn() {
        char* subBuffer = strstr(defender, "respawn");
        subBuffer += 8;

	strncpy(defenderRespawn, subBuffer, strlen(subBuffer) - 1);
	int value = atoi(defenderRespawn);
        return value;
}

char *replace_str(char *str, char *orig, char *rep) {
  	static char buffer[4096];
  	char *p;

  	if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    	return str;

  	strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  	buffer[p-str] = '\0';

  	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  	return buffer;
}

char getCharOnMap(int x, int y) {
	return map[x + 70 * y];
}



void loadMap(char mapFile[1024]) {
	
	FILE *file;
	file = fopen(mapFile, "r");
	// initialize map to empty
	memset(&map, ' ', sizeof map);

	if (file == NULL) {
		perror ("Error opening file");
		exit(EXIT_FAILURE);
	} else {
		int i = 0;
		while (fgets(buffer, 1024, file) != NULL) {
			if (i == 0) {
				strncpy(mapName, buffer, strlen(buffer)-1);
			} else if (i == 1) {
				strncpy(authorName, buffer, strlen(buffer)-1);
			} else if (i == 2) {
				strncpy(castle, buffer, strlen(buffer)-1);
			} else if (i == 3) {
                                strncpy(attacker, buffer, strlen(buffer)-1);
                        } else if (i == 4) {
				strncpy(defender, buffer, strlen(buffer)-1);
			} else if (i >= 7) {
				//printf("%zu|%s", strlen(&buffer[2]), &buffer[2]);
				strncpy((char *)(map + 70 * (i-7)), &buffer[2], strlen(buffer)-3);
				char* printable_line = replace_str(&buffer[2], "%", "%%");
				mvprintw(i - 6, 0, printable_line);
			}
			i++;
			if (i > 27) break;
		}
		fclose(file);
		//fprintf(stdout, "%s", getMap());
	};

	init_pair(1, COLOR_BLACK, COLOR_GREEN);
  	init_pair(2, COLOR_BLACK, COLOR_RED);
  	init_pair(3, COLOR_BLACK, COLOR_BLUE);
  	init_pair(4, COLOR_BLACK, COLOR_WHITE);
  	init_pair(5, COLOR_BLACK, COLOR_CYAN);
	
	attron(COLOR_PAIR(5));

	int i = 0;
	for (i = 0; i < 80; i++) {
		mvprintw(0, i, " ");
	}

	char* mapText = "map: ";
  	char* authorText = "author: ";
  	mvprintw(0, 0, mapText);
 	mvprintw(0, strlen(mapText), getMapName());
  	mvprintw(0, strlen(mapText) + strlen(getMapName()) + 2, authorText);
  	mvprintw(0, strlen(mapText) + strlen(getMapName()) + strlen(authorText) + 2
                        , getAuthor());


	/*	
	fprintf(stdout, "%s\n", getMapName());
	fprintf(stdout, "%s\n", getAuthor());
	fprintf(stdout, "%d", getCastleStrength());
	printf("\n");
	fprintf(stdout, "%d", getCastleTouch());
	printf("\n");
	
	fprintf(stdout, "%d", getAttackerWin());
	printf("\n");
	fprintf(stdout, "%d", getAttackerShots());
        printf("\n");
	fprintf(stdout, "%d", getAttackerRespawn());
        printf("\n");
	fprintf(stdout, "%d", getDefenderWin());
        printf("\n");
	fprintf(stdout, "%d", getDefenderShots());
        printf("\n");
	fprintf(stdout, "%d", getDefenderRespawn());
        printf("\n");
	*/
}
