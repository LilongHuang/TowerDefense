nclude <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define _POSIX_SOURCE
#include <string.h>

char buffer[8 * 1024];
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

char* getMapName() {
        return mapName;
}

char* getAuthor() {
        return authorName;
}

char* getCastleStrength() {
        char* subBuffer = strstr(castle, "strength");
        subBuffer += 9;
        char* subBuffer2 = strstr(castle, "touch");
        int diffBytes = subBuffer2 - subBuffer;

        strncpy(castleStrength, subBuffer, diffBytes - 1);
        return castleStrength;
}

char* getCastleTouch() {
        char* subBuffer = strstr(castle, "touch");
        subBuffer += 6;

        strncpy(castleTouch, subBuffer, strlen(subBuffer));
        return castleTouch;
}

char* getAttackerWin() {
        char* subBuffer = strstr(attacker, "destroy");
        subBuffer += 8;
        char* subBuffer2 = strstr(attacker, "shots");
        int diffBytes = subBuffer2 - subBuffer;

        strncpy(attackerWin, subBuffer, diffBytes - 1);
        return attackerWin;
}

char* getAttackerShots() {
        char* subBuffer = strstr(attacker, "shot");
        subBuffer += 6;
        char* subBuffer2 = strstr(attacker, "respawn");
        int diffBytes = subBuffer2 - subBuffer;

        strncpy(attackerShots, subBuffer, diffBytes - 1);
        return attackerShots;
}

char* getAttackerRespawn() {
        char* subBuffer = strstr(attacker, "respawn");
        subBuffer += 8;

        strncpy(attackerRespawn, subBuffer, strlen(subBuffer));
        return attackerRespawn;
}

char* getDefenderWin() {
        char* subBuffer = strstr(defender, "survive");
        subBuffer += 8;
        char* subBuffer2 = strstr(defender, "shot");
        int diffBytes = subBuffer2 - subBuffer;

        strncpy(defenderWin, subBuffer, diffBytes - 1);
        return defenderWin;
}

char* getDefenderShots() {
        char* subBuffer = strstr(defender, "shot");
        subBuffer += 6;
        char* subBuffer2 = strstr(defender, "respawn");
        int diffBytes = subBuffer2 - subBuffer;

        strncpy(defenderShots, subBuffer, diffBytes - 1);
        return defenderShots;
}

char* getDefenderRespawn() {
        char* subBuffer = strstr(defender, "respawn");
        subBuffer += 8;

        strncpy(defenderRespawn, subBuffer, strlen(subBuffer));
        return defenderRespawn;
}




int laodMap(int argc, char *argv[]) {


        if (argc != 2) {
                fprintf(stderr, "Syntax:  %s filename\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        FILE *file;
        file = fopen(argv[1], "r");

        if (file == NULL) perror ("Error opening file");
        else {
                int i = 0;
                while (fgets(buffer, 8 * 1024, file) != NULL && i < 6) {
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
                        }
                        i++;
                }
                fclose(file);
        };


        /*
        fprintf(stdout, "%s\n", getMapName());
        fprintf(stdout, "%s\n", getAuthor());
        fprintf(stdout, "%s", getCastleStrength());
        printf("\n");
        fprintf(stdout, "%s", getCastleStrength());
        printf("\n");
        fprintf(stdout, "%s", getCastleTouch());
        printf("\n");
        fprintf(stdout, "%s", getAttackerWin());
        printf("\n");
        fprintf(stdout, "%s", getAttackerShots());
        printf("\n");
        fprintf(stdout, "%s", getAttackerRespawn());
        printf("\n");
        fprintf(stdout, "%s", getDefenderWin());
        printf("\n");
        fprintf(stdout, "%s", getDefenderShots());
        printf("\n");
        fprintf(stdout, "%s", getDefenderRespawn());
        printf("\n");
        */



}

