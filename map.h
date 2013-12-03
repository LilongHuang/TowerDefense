// Signals
#define RENDER_FORMAT_STRING "render %i %i %i %i %i"
#define BACKGROUND_COLOR 0

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

int attackerRespawnPointCount;
int defenderRespawnPointCount;
int wallCount;

struct point_t {
        int x;
        int y;
};

struct row_t {
        char content[1024];
};

struct percent_wall {
        int x;
        int y;
};

struct round_counter {
        int x;
        int y;
};

struct round_counter round_counter_location[10];
struct percent_wall percent_wall_location[10];
struct point_t attacker_respawn_location_list[1024];
struct point_t defender_respawn_location_list[1024];
struct row_t list_row[20];

char* getMap();

char* getMapName();
char* getAuthor();

int getDefaultCastleStrength();
int getCastleTouch();
void set_castle_strength(int, int, int);
int get_castle_strength(int, int);

int getAttackerWin();
int getAttackerShots();
int getAttackerRespawn();

int getDefenderWin();
int getDefenderShots(); 
int getDefenderRespawn();

char getCharOnMap(int, int);

struct point_t getAttackerRespawnPoint();

struct point_t getDefenderRespawnPoint();

struct percent_wall getPercentWall();

struct round_counter getRoundCounter();

int getWallCount();

void setCharOnMap(char, int, int);

void createColorPair();

void initBoard();

void teamInfoMap();

void loadMap(char[1024]);
