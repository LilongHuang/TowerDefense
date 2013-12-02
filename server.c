#include "map.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>

#define max_players 10
#define players_per_team 5
#define TIMER_START 5
#define EVENT_QUEUE_SIZE 20
#define MILLIS_BETWEEN_UPDATES 500

#define PLAYER_CHAR 'O'


typedef enum {TEAM_A, TEAM_B, UNASSIGNED} team_t;

struct player_t
{
  pthread_mutex_t player_read_mutex;
  pthread_mutex_t player_write_mutex;
  char name[10];
  team_t team;
  int x; // column
  int y; // row
  int score;
  int fd;
  int bullets;
  int player_color;
  char recvBuff[1024];
  char sendBuff[1024];
};

struct player_t SYSTEM_PLAYER;

const char BULLET_CHARS[4] = {'^', '>', 'v', '<'};
struct bullet_t
{
  struct player_t* owner;
  int direction; // 0: up, 1: right, 2: down, 3: left
  int x; // column
  int y; // row
};

struct bullet_t bullet_list[1024];
pthread_mutex_t bullet_array_mutex = PTHREAD_MUTEX_INITIALIZER;
int bullet_count;

typedef enum {MOVE_REL, MOVE_ABS, SHOOT} event_type_t;

struct event_t
{
  event_type_t type;
  char c;
  struct player_t* player;
  //int rows;
  //int cols;
};

int player_colors =11;

int team_A_counter = 0;
int team_B_counter = 0;

char a_team[512];
char b_team[512];

// don't remove these, we need at least sendBuff now
char sendBuff[1024];
char recvBuff[1024];

int client_count = 0;

int sec_counter = TIMER_START;
int round_index = 0;

struct player_t player_list[10];
pthread_mutex_t team_array_mutex = PTHREAD_MUTEX_INITIALIZER;

// map filename, as string
char mapPath[1024];

struct tile_t {
  char c;
  int fg_color;
  int bg_color;
};

pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

// do not use ANY of these directly unless you know what you're doing!
// all input should be handled via cases in process_message().
struct event_t next_event[EVENT_QUEUE_SIZE];
int event_count = 0;
pthread_mutex_t next_event_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t next_event_space_remaining;
sem_t next_event_messages_waiting;

// ===========================================================

void init_player(struct player_t *p){
  pthread_mutex_init(&(p->player_read_mutex), NULL);
  pthread_mutex_init(&(p->player_write_mutex), NULL);
  p -> team = UNASSIGNED;
  p -> fd = -1;
  p -> x = 3;
  p -> y = 3;
  p -> score = 0;
  p -> player_color = player_colors;
  player_colors++;
}

void a_to_b_team(){
  for(int i = (team_A_counter + team_B_counter)-1; i > 0; i--){
    struct player_t *p = &player_list[i];
    if(p->team == TEAM_A){
      p->team = TEAM_B;
      team_A_counter -= 1;
      team_B_counter += 1;
      break;
    }
  }
}

void b_to_a_team(){
  for(int i = (team_A_counter + team_B_counter)-1; i > 0; i--){
    struct player_t *p = &player_list[i];
    if(p->team == TEAM_B){
      p->team = TEAM_A;
      team_A_counter += 1;
      team_B_counter -= 1;
      break;
    }
  }
}

void balance_teams(){
  //printf("teamA: %d | teamB: %d\n", team_A_counter, team_B_counter);
  if(team_A_counter > team_B_counter){
    while((team_A_counter - team_B_counter) != 1){
      if((team_A_counter - team_B_counter) == 0)
	break;
      a_to_b_team();
    }
  }
  else if(team_A_counter < team_B_counter){
    while((team_B_counter - team_A_counter) != 1){
      if((team_A_counter - team_B_counter) == 0)
        break;
      b_to_a_team();
    }
  }
}

void create_teams(){
  balance_teams();
  char temp_a_team[512];
  char temp_b_team[512];
  for(int i = 0; i < (team_A_counter + team_B_counter); i++){
    struct player_t* p = &player_list[i];
    if(p->team == TEAM_A){
      char build_color_player[15];
      sprintf(build_color_player, "%d%s", p->player_color, p->name);
      strcat(temp_a_team, build_color_player);
      strcat(temp_a_team, ",");
    }
    else{
      char build_color_player[15];
      sprintf(build_color_player, "%d%s", p->player_color, p->name);
      strcat(temp_b_team, build_color_player);
      strcat(temp_b_team, ",");
    }
  }
  strcpy(a_team, temp_a_team);
  strcpy(b_team, temp_b_team);
  printf("%s\n%s\n", a_team, b_team);
}

int balance_names(char *name){
  for(int i = 0; i < team_A_counter + team_B_counter; i++){
    struct player_t* temp_player = &player_list[i];

    if(strcmp(name, temp_player->name) == 0){
      int rn = rand()%9000%1000;
      char rn_string[128];
      sprintf(rn_string, "%d", rn);
      strcat(name, rn_string);

      return 0;
    }
  }
  return 1;
}

struct player_t* update_a_team(struct player_t *p, char *name){
  while(balance_names(name) == 0){}
  strcpy(p -> name, name);
  player_list[team_A_counter + team_B_counter] = *p;
  team_A_counter += 1;
  return &player_list[team_A_counter + team_B_counter - 1];
}

struct player_t* update_b_team(struct player_t *p, char *name){
  while(balance_names(name) == 0){}
  strcpy(p -> name, name);
  player_list[team_A_counter + team_B_counter] = *p;
  team_B_counter += 1;
  return &player_list[team_A_counter + team_B_counter - 1];
}

void init_signals(void) {
  // use sigaction(2) to catch SIGPIPE somehow
  struct sigaction sigpipe;
  memset(&sigpipe, 0, sizeof(sigpipe));
  sigpipe.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sigpipe, NULL);
  return;
}

struct player_t* team_setup(int connfd){
  pthread_mutex_lock(&team_array_mutex);

  struct player_t temp_player;
  init_player(&temp_player);

    /*printf("TS: Locking player %s\n", player.name);
  pthread_mutex_lock(&(player.player_mutex));
  printf("%s", "TS: Locked.\n");
  pthread_mutex_unlock(&(player.player_mutex));
  printf("%s", "TS: Released.\n");*/

  read(connfd, temp_player.recvBuff, sizeof temp_player.recvBuff);

  char tempName[10];
  char tempTeam[10];
  sscanf(temp_player.recvBuff, "%s %s", tempName, tempTeam);

  temp_player.fd = connfd;
  struct player_t* player;

  if(strcmp(tempTeam, "1") == 0){
    //update player list
    if(team_A_counter < players_per_team){
      temp_player.team = TEAM_A;
      player = update_a_team(&temp_player, tempName);
    }
    else{
      temp_player.team = TEAM_B;
      player = update_b_team(&temp_player, tempName);
    }
    snprintf(player->sendBuff, sizeof player->sendBuff, "%s", player->name);
  }
  else { // if(strcmp(tempTeam, "2") == 0){
    //update player list
    if(team_B_counter < players_per_team){
      temp_player.team = TEAM_B;
      player = update_b_team(&temp_player, tempName);
    }
    else{
      temp_player.team = TEAM_A;
      player = update_a_team(&temp_player, tempName);
    }
    snprintf(player->sendBuff, sizeof player->sendBuff, "%s", player->name);
  }
  //player = &(player_list[client_count-1]);
  printf("%s has joined\n", player->name);  

  // echo all input back to client
  write(connfd, player->sendBuff, strlen(player->sendBuff) + 1);

  pthread_mutex_unlock(&team_array_mutex);

  return player;
}

int is_attacker(const struct player_t p) {
  return (p.team == TEAM_A && round_index == 1) || (p.team == TEAM_B && round_index == 2);
}

int getBackgroundColor(int x, int y) {
  return 0;
}

struct tile_t getTileAt(struct tile_t* t, int x, int y) {
  t->c = ' ';
  t->fg_color = 1;
  t->bg_color = getBackgroundColor(x, y);
  
  bool modified = false;
  
  // check if there's a player there
  for (int i = 0; i < client_count; i++) {
    struct player_t* p = &player_list[i];
    printf("Checking player %s (%d, %d) against %d, %d\n", p->name, p->x, p->y, x, y);
    if (p->x == x && p->y == y) {
      if (t->c == ' ') {
        printf("Found a match! '%c' -> '%c'\n", t->c, PLAYER_CHAR);
        t->c = PLAYER_CHAR;
        t->fg_color = p->player_color;
        modified = true;
      }
      else if (t->c == PLAYER_CHAR) {
        printf("%s", "Found overlapping players!\n");
        // two players there; assign it team color
        t->bg_color = p->player_color;
        modified = true;
      }
    }
  }
  
  // check if there's a bullet there
  pthread_mutex_lock(&bullet_array_mutex);
  for (int i = 0; i < bullet_count; i++) {
    struct bullet_t* b = &(bullet_list[i]);
    if (b->x == x && b->y == y) {
      t->c = BULLET_CHARS[b->direction];
      t->fg_color = b->owner->player_color;
      modified = true;
    }
  }
  pthread_mutex_unlock(&bullet_array_mutex);
  
  // else, get whatever's on the map
  if (!modified) {
    t->c = getCharOnMap(x, y);
  }
  return *t;
}

bool isCastleChar(char c) {
  return c == '-' || c == '|' || c == '+' || c == '*' || c == '/' || c == '\\';
}
bool isDestroyableCoverChar(char c) {
  return c >= '1' && c <='9';
}
bool isCoverChar(char c) {
  return c == '0' || isDestroyableCoverChar(c);
}

// X and Y coordinates for this function are absolute
// (because defenders can warp to absolute locations)
char* try_attacker_move(struct player_t *p, int x, int y) {
  pthread_mutex_lock(&map_mutex);
  char c = getCharOnMap(x, y);
  char* retval;
  if (x < 0 || x > 70 || y < 0 || y > 20) {
    // nope, you're trying to move off the edge of the map
    retval = NULL;
  }
  else if (isCastleChar(c) || isCoverChar(c) ) {
    // nope, you're trying to move into a wall
    retval = NULL;
  }
  else {
    // update former location
    struct tile_t oldt;
    struct tile_t newt;
    int oldx = p->x;
    int oldy = p->y;
    p->x = x;
    p->y = y;
    getTileAt(&oldt, oldx, oldy);
    getTileAt(&newt, x, y);
    //sprintf(sendBuff,
    //printf("%s moving to %d, %d\n", p->name, p->x, p->y);
    // update new location
    sprintf(sendBuff, "Render char%c at x%i y%i colora%i colorb%i\nRender char%c at x%i y%i colora%i colorb%i",
            oldt.c, oldx, oldy, oldt.fg_color, oldt.bg_color,
            newt.c, player_list[0].x, player_list[0].y, newt.fg_color, newt.bg_color);

    retval = sendBuff;
  }
  pthread_mutex_unlock(&map_mutex);
  return retval;
}

char* shoot(struct player_t* p, int direction) {
  // shoot!
  return NULL;
}

void destroy_bullet(int start) {
  for (int i = start + 1; i < bullet_count; i++) {
    bullet_list[i-1] = bullet_list[i];
  }
  bullet_count--;
}

void update_bullets(void) {
  // advance all bullets forward
  pthread_mutex_lock(&map_mutex);
  pthread_mutex_lock(&bullet_array_mutex);
  for (int i = 0; i < bullet_count; i++) {
    struct bullet_t* b = &(bullet_list[i]);
    switch (b->direction) {
      case 0: b->y--; break;
      case 1: b->x++; break;
      case 2: b->y++; break;
      case 3: b->x--; break;
    }
    bool destroyed = false;
    // destroy bullets beyond the boundaries of the map
    if (b->x < 0 || b->x > 70 || b->y < 0 || b->y > 20) {
      destroyed = true;
    }
    
    char target = getCharOnMap(b->x, b->y);
    
    // collide with destroyables
    
    // cover tiles
    if (target == '0') {
      destroyed = true;
    }
    else if (isDestroyableCoverChar(target)) {
      // check to see if attacker bullet
      target--;
      if (!isDestroyableCoverChar(target)) {
        target = ' ';
      }
      setCharOnMap(target, b->x, b->y);
      destroyed = true;
    }
    // castle tiles
    else if (isCastleChar(target)) {
      // check to see if defender bullet
    }
    
    // collide with players
    
    if (destroyed) {
      destroy_bullet(i);
      i--;
    }
  }
  pthread_mutex_unlock(&bullet_array_mutex);
  pthread_mutex_unlock(&map_mutex);
}

// returns a char pointer to the first char of a message
// that is sent to all players. this message should be the
// instructions to the client on how to update their screen
// to reflect this event occurring.

// REMEMBER: NO GAME LOGIC IN THE CLIENT!
char* process_message(struct event_t* event) {
  struct player_t *p = event->player;
  char c = event->c;
  if (c == 'U') {
    update_bullets();
  }
  else if (c == 'G') {
  // Game Start
    char *game_started = "GameIsStarting!";
    sprintf(sendBuff, "%s %s %s %s", game_started, mapPath, a_team, b_team);
    return sendBuff;
  }
  // move down
  else if (c == 'S' || c == 's') {
    // TODO: check team here
    return try_attacker_move(p, p->x, p->y+1);
  }
  // move up
  else if (c == 'W' || c == 'w') {
    // TODO: check team here
    return try_attacker_move(p, p->x, p->y-1);
  }
  // move left
  else if (c == 'A' || c == 'a') {
    // TODO: check team here
    return try_attacker_move(p, p->x-1, p->y);
  }
  // move right
  else if (c == 'D' || c == 'd') {
    // TODO: check team here
    return try_attacker_move(p, p->x+1, p->y);
  }
  else if (c == 'I' || c == 'i') {
    return shoot(p, 0);
  }
  // handle other shoot cases
  // debug: show map in server log
  else if (c == 'M' || c == 'm') {
    printf("Map:\n%s\n", getMap());
    return NULL;
  }
  else if (c == 'O' || c == 'o') {
    sprintf(sendBuff, "Render x%i y%i c%c colorA%i colorB%i", p->x, p->y, PLAYER_CHAR, 0, 1);
    return sendBuff;
  }
  else {
    sprintf(sendBuff, "Keystroke: %s hit %c", p->name, c);
    return sendBuff;
  }
  return NULL;
}

void pop_message(void) {
  printf("%s\n", "Popping message");
  sem_wait(&next_event_messages_waiting);
  pthread_mutex_lock(&next_event_mutex);
  pthread_mutex_lock(&team_array_mutex);

  // accessing element zero by using the pointer to the start of the array.
  // this is prrrrrobably a bad idea, but if I don't get a pointer to the event
  // somehow, then all our changes will be reverted as soon as we return from
  // this function
  struct event_t* event = next_event;
  char* output = process_message(event);
  
  // if that message has any output, send it to all players
  if (output != NULL) {
    printf("  Sending message to %i player(s):\n", client_count);
    for (int i = 0; i < client_count; i++) {
      struct player_t *p = &player_list[i];
      //printf("Acquiring lock on %s\n", p->name);
      pthread_mutex_lock(&(p->player_write_mutex));
      //printf("%s", "Lock acquired.\n");
      printf("    %c to %s\n", event->c, p->name);
      // print out newline-delimited instructions
      char* saveptr;
      char* nextLine = strtok_r(output, "\n", &saveptr);
      do {
        printf("%s\n", nextLine);
        write(p->fd, nextLine, strlen(nextLine)+1);
	usleep(1); // FIXME there's gotta be a better way to keep it
	// from not seeing the second line of multiline messages...
      } while((nextLine = strtok_r(NULL, "\n", &saveptr)) != NULL);
      pthread_mutex_unlock(&(p->player_write_mutex));
    }
  }
  
  // slide all events over to fill empty spot
  printf("Last event is #%i\n", event_count);
  for (int i = 0; i < event_count + 1; i++) {
    next_event[i] = next_event[i+1];
  }
  event_count--;

  // release resources
  pthread_mutex_unlock(&team_array_mutex);
  pthread_mutex_unlock(&next_event_mutex);
  sem_post(&next_event_space_remaining);
}

void push_message(const char m, struct player_t* player) {
  // WARNING: If this is updated to ever access more than the player's name,
  // update the system player's initialization appropriately!

  // capture event queue, and prepare to add a new event
  sem_wait(&next_event_space_remaining);
  pthread_mutex_lock(&next_event_mutex);
  printf(" - %s locking\n", player->name);
  // initialize event to put on queue
  struct event_t event;
  event.type = SHOOT;
  event.c = m;
  event.player = player;
  printf(">> Adding %c by %s\n", event.c, player->name);
  // put event on queue
  next_event[event_count] = event;
  event_count++;
  // release resources
  sem_post(&next_event_messages_waiting);
  printf("Completed push. Next event is #%i\n", event_count);
  pthread_mutex_unlock(&next_event_mutex);
}

void *client_thread(void *arg)
{
  int connfd = *((int *)arg);

  //int length = sprintf(sendBuff, "[Clients: %d]", player_count);
  //write(connfd, sendBuff, length+1);

  
  if(sec_counter <= 0){
    char *ghs = "Game has already started";
    write(connfd, ghs, strlen(ghs)+1);
    pthread_mutex_lock(&team_array_mutex);
    client_count--;
    pthread_mutex_unlock(&team_array_mutex);
    close(connfd);
    pthread_exit(0);
  }

  if((team_A_counter + team_B_counter) > 9){
    // what if the max isn't ten players any more?
    // hard-coding things like this is bad practice.
    char *full_game = "Already 10 players joined";
    write(connfd, full_game, strlen(full_game)+1);
    close(connfd);
    pthread_exit(0);
  }

  int n;
  
  struct player_t* player = team_setup(connfd);
  //printf("Player %s has FD %i\n", player_list[0].name, player_list[0].fd);
  //printf("Player %s has FD %i\n", player.name, player.fd);
  /*printf("%s", "Attempting lock\n");
  pthread_mutex_lock(&(player_list[0].player_mutex));
  printf("%s", "Locked\n");
  pthread_mutex_unlock(&(player_list[0].player_mutex));
  printf("%s", "Released\n");*/
  while (1)
  {
    if(sec_counter > 0){
      //sends each client the newly updated player list
      char send[1024];
      snprintf(send, sizeof send, "%d", sec_counter);

      write(connfd, send, strlen(send)+1);
      sleep(1);
    }
    else {
      //printf("Locking %s for read\n", player->name);
      pthread_mutex_lock(&(player->player_read_mutex));
      //printf("Locked %s for read\n", player->name);
      if ((n = read(connfd, player->recvBuff, sizeof player->recvBuff)) != 0) {
	  // echo all input back to client
	  //printf("Writing %d bytes to buffer\n", n);
          //write(connfd, player.recvBuff, n);
	  printf("Pushing %c onto event queue from %s\n", player->recvBuff[0], player->name);
	  push_message(player->recvBuff[0], player);
      }
      else {
	// They disconnected-- release the client
	//printf("Server releasing client [%d remain]\n", player_count-1);
	close(connfd);
	//player_count--;
	pthread_exit(0);
      }
      //printf("Unlocking %s from read\n", player->name);
      pthread_mutex_unlock(&(player->player_read_mutex));
      //printf("Unlocked %s from read\n", player->name);
    }
  }
  return NULL;
}

void* update_thread(void *arg) {
  while (1) {
    printf("%s", "Update thread pushing update\n");
    push_message('U', &SYSTEM_PLAYER);
    usleep(MILLIS_BETWEEN_UPDATES);
  }
}

void *loading_thread(void *arg){
  for(int i = TIMER_START; i >= -1; i--){
    if(i == 0){
      //balance_teams();
      create_teams();
    }
    sec_counter = i;
    sleep(1);
  }

  push_message('G', &SYSTEM_PLAYER);
  round_index++;
  loadMap(mapPath);
  
  pthread_t upd_thread;
    if (pthread_create(&upd_thread, NULL, update_thread, NULL) != 0)
    {
      fprintf(stderr, "Server unable to create update_thread\n");
    }

  while (1) {
    pop_message();
  }
  
  return NULL;
}

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf("Usage: %s mapfile port\n", argv[0]);
    return 1;
  }
  
  strcpy(SYSTEM_PLAYER.name, "System");

  strncpy(mapPath, argv[1], strlen(argv[1]) + 1);
  
  init_signals();
  sem_init(&next_event_space_remaining, 0, EVENT_QUEUE_SIZE);
  sem_init(&next_event_messages_waiting, 0, 0);
  
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  
  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof serv_addr);
  memset(sendBuff, '0', sizeof sendBuff);
  memset(recvBuff, '0', sizeof recvBuff);

  memset(a_team, 0, sizeof a_team);
  memset(b_team, 0, sizeof b_team);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(strtoumax(argv[2], NULL, 10));

  bind(listenfd, (struct sockaddr*)&serv_addr, sizeof serv_addr);
  listen(listenfd, 10);
  
  /*if(sec_counter == 0){
    int connfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
    char *ghs = "game has already started";
    write(connfd, ghs, sizeof ghs);
    }*/

  while(1)
  {
    int connfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
    //printf("Accepted a connection on FD %i\n", connfd);
    pthread_t conn_thread;
    if (pthread_create(&conn_thread, NULL, client_thread, &connfd) != 0)
    {
      fprintf(stderr, "Server unable to create client_thread\n");
    }
    
    pthread_mutex_lock(&team_array_mutex);
    client_count += 1;
    if(client_count == 1){
      pthread_t timer_thread;
      if (pthread_create(&timer_thread, NULL, loading_thread, &connfd) != 0)
	{
	  fprintf(stderr, "Server unable to create timer_thread\n");
	}
    }
    pthread_mutex_unlock(&team_array_mutex);
  }

}
