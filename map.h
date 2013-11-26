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

char* getMap();

char* getMapName();

char* getAuthor();

int getCastleStrength();

int getCastleTouch();

int getAttackerWin();

int getAttackerShots();

int getAttackerRespawn();

int getDefenderWin();

int getDefenderShots(); 

int getDefenderRespawn();

void loadMap(char[1024]);
