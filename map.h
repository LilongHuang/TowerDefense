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

int respawnPointCount;

struct respawn_location {
	int x;
	int y;
};

struct row_t {
        char content[1024];
};

struct row_t list_row[20];

struct respawn_location respawn_location_list[1024];

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

char getCharOnMap(int, int);

void assignRespwanPoint(int, int);

void initBoard();

void teamInfoMap();

void loadMap(char[1024]);
