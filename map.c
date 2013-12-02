#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>

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
char map[1024];

char castle[1024];
char attacker[1024];
char defender[1024];

int respawnPointCount = 0;

struct respawn_location {
	int x;
	int y;
};

struct respawn_location respawn_location_list[1024];

struct row_t {
	char content[1024];
};

struct row_t list_row[20];

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

//randomly get a respawn point
void assignRespwanPoint(int col, int row) {
	srand(time(NULL));
        int ran = rand();
        int rand_capped = ran % respawnPointCount;  //between 0 respawnPointCount
	col = respawn_location_list[rand_capped].x;
	row = respawn_location_list[rand_capped].y;
}

void createColorPair() {
	init_pair(0, COLOR_WHITE, COLOR_BLACK);
	init_pair(1, COLOR_BLACK, COLOR_GREEN);
	init_pair(2, COLOR_BLACK, COLOR_RED);
	init_pair(3, COLOR_BLACK, COLOR_BLUE);
	init_pair(4, COLOR_BLACK, COLOR_WHITE);
	init_pair(5, COLOR_BLACK, COLOR_CYAN);
}

void initBoard(){
  createColorPair();
  //background team A (red)
  for(int i = 70; i < 80; i++){
    for(int j = 1; j <= 10; j++){
      attron(COLOR_PAIR(2));
      mvprintw(j, i, " ");
    }
  }

  //background team B (blue)
  for(int i = 70; i < 80; i++){
    for(int j = 11; j <= 20; j++){
      attron(COLOR_PAIR(3));
      mvprintw(j, i, " ");
    }
  }

  //description area (scrollable)
  for(int i = 0; i < 80; i++){
    for(int j = 21; j < 24; j++){
      attron(COLOR_PAIR(4));
      mvprintw(j, i, " ");
    }
  }

}

void teamInfoMap() {
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
				int mapRow = i - 7;
				char* printable_line = replace_str(&buffer[2], "%", "%%");
				printable_line = replace_str(printable_line, "\n", " ");
				
				struct row_t row;
				strncpy(row.content, printable_line, strlen(printable_line) + 1);
				list_row[mapRow] = row;
				while (strlen(list_row[mapRow].content) < 70) {
					strncat(list_row[mapRow].content, " ", 1);
				}
				for (int j = 0; j < 70; j++) {
					int actualLocation = j + 2;
					if (buffer[actualLocation] == '@') {
						respawn_location_list[respawnPointCount].x = j;
						respawn_location_list[respawnPointCount].y = i - 7;	
						respawnPointCount++;
					} 
				}
				mvprintw(i - 6, 0, list_row[mapRow].content);
			}
			i++;
			if (i > 27) break;
		}
		fclose(file);
	};

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
