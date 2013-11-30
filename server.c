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

#define max_players 10
#define TIMER_START 3
#define EVENT_QUEUE_SIZE 20

typedef enum {TEAM_A, TEAM_B, UNASSIGNED} team_t;

struct player_t
{
  pthread_mutex_t player_mutex;
  char *name;
  team_t team;
  int x; // column
  int y; // row
  int score;
  int fd;
  char recvBuff[1024];
  char sendBuff[1024];
};

struct player_t SYSTEM_PLAYER;

typedef enum {MOVE_REL, MOVE_ABS, SHOOT} event_type_t;

struct event_t
{
  event_type_t type;
  int c;
  struct player_t* player;
  //int rows;
  //int cols;
};

int team_A_counter = 0;
int team_B_counter = 0;

char a_team[512];
char b_team[512];

// don't remove these, we need at least sendBuff now
char sendBuff[1024];
char recvBuff[1024];

int clientCount = 0;

int sec_counter = TIMER_START;
int round_index = 0;

struct player_t player_list[10];
pthread_mutex_t team_array_mutex = PTHREAD_MUTEX_INITIALIZER;

char mapPath[1024];

struct event_t next_event[EVENT_QUEUE_SIZE];
int event_count = 0;
pthread_mutex_t next_event_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t next_event_space_remaining;
sem_t next_event_messages_waiting;

void init_player(struct player_t *p){
  pthread_mutex_init(&(p->player_mutex), NULL);
  p -> team = UNASSIGNED;
  p -> fd = -1;
  p -> x = 3;
  p -> y = 3;
  p -> score = 0;
}

int balance_names(char *name){
  //srand(123);
  for(int i = 0; i < team_A_counter + team_B_counter; i++){
    struct player_t temp_player = player_list[i];
    printf("%s, %s\n", name, temp_player.name);
    if(strcmp(name, temp_player.name) == 0){
      //int rn = rand();
      //char rn_string[128];
      //sprintf(rn_string, "%d", rn);
      //strcpy(name, rn_string);
      strcat(name, "1");
      printf("%s\n", name);
      return 0;
    }
  }
  return 1;
}

void update_a_team(struct player_t *p, char *name){
  while(balance_names(name) == 0){}
  p -> name = name;
  player_list[team_A_counter + team_B_counter] = *p;
  team_A_counter += 1;
  char temp[512];
  if(strlen(a_team) > 0){
    snprintf(temp, sizeof temp, "%s, %s", a_team, name);
  }
  else{
    snprintf(temp, sizeof temp, "%s", name);
  }
  strcpy(a_team, temp);
}

void update_b_team(struct player_t *p, char *name){
  while(balance_names(name) == 0){}
  p -> name = name;
  player_list[team_A_counter + team_B_counter] = *p;
  team_B_counter += 1;
  char temp[512];
  if(strlen(b_team) > 0){
    snprintf(temp, sizeof temp, "%s, %s", b_team, name);
  }
  else{
    snprintf(temp, sizeof temp, "%s", name);
  }
  strcpy(b_team, temp);
}

void init_signals(void) {
  // use sigaction(2) to catch SIGPIPE somehow
  struct sigaction sigpipe;
  memset(&sigpipe, 0, sizeof(sigpipe));
  sigpipe.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sigpipe, NULL);
  return;
}

struct player_t team_setup(int connfd){
  pthread_mutex_lock(&team_array_mutex);

  struct player_t player;
  init_player(&player);

    /*printf("TS: Locking player %s\n", player.name);
  pthread_mutex_lock(&(player.player_mutex));
  printf("%s", "TS: Locked.\n");
  pthread_mutex_unlock(&(player.player_mutex));
  printf("%s", "TS: Released.\n");*/

  read(connfd, player.recvBuff, sizeof player.recvBuff);

  char tempName[10];
  char tempTeam[10];
  sscanf(player.recvBuff, "%s %s", tempName, tempTeam);

  player.fd = connfd;

  if(strcmp(tempTeam, "1") == 0){
    //update player list
    player.team = TEAM_A;

    update_a_team(&player, tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s", player.name);
  }
  else { // if(strcmp(tempTeam, "2") == 0){
    //update player list
    player.team = TEAM_B;

    update_b_team(&player, tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s", player.name);
  }

  // echo all input back to client
  write(connfd, player.sendBuff, strlen(player.sendBuff) + 1);

  pthread_mutex_unlock(&(player.player_mutex));
  pthread_mutex_unlock(&team_array_mutex);

  return player;
}

int is_attacker(const struct player_t p) {
  return (p.team == TEAM_A && round_index == 1) || (p.team == TEAM_B && round_index == 2);
}

// X and Y coordinates for this function are absolute
// (because defenders can warp to absolute locations)
char* try_attacker_move(struct player_t *p, int x, int y) {
  char c = getCharOnMap(x, y);
  if (x < 0 || x > 70 || y < 0 || y > 20) {
    // nope, you're trying to move off the edge of the map
    return NULL;
  }
  else if (c == '-' || c == '|' || c == '*' || c == '+' || (c >= '1' && c <='9') ) {
    // nope, you're trying to move into a wall
    return NULL;
  }
  else {
    p->x = x;
    p->y = y;
    //printf("%s moving to %d, %d\n", p->name, p->x, p->y);
    sprintf(sendBuff, "Move %s to x%i y%i", p->name, p->x, p->y);
    return sendBuff;
  }
}

char* process_message(struct event_t* event) {
    struct player_t *p = event->player;
    char c = event->c;
    // Game Start
    if (c == 'G') {
      char *game_started = "GameIsStarting!";
      sprintf(sendBuff, "%s %s", game_started, mapPath);
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
    // debug: show map in server log
    else if (c == 'M' || c == 'm') {
      printf("Map:\n%s\n", getMap());
      return NULL;
    }
    else if (c == 'O' || c == 'o') {
      sprintf(sendBuff, "%s is at x%i y%i", p->name, p->x, p->y);
      return sendBuff;
    }
    else {
      sprintf(sendBuff, "Render: %s hit %c", p->name, c);
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
    printf("  Sending message to %i player(s):\n", clientCount);
    for (int i = 0; i < clientCount; i++) {
      struct player_t *p = &(player_list[i]);
      //printf("Acquiring lock on %s\n", p->name);
      pthread_mutex_lock(&(p->player_mutex));
      //printf("%s", "Lock acquired.\n");
      printf("    %c to %s\n", event->c, p->name);
      write(p->fd, output, strlen(output)+1);
      pthread_mutex_unlock(&(p->player_mutex));
    }
  }
  
  // slide all events over to fill empty spot
  int last_event_index = 0;
  sem_getvalue(&next_event_messages_waiting, &last_event_index);
  printf("Last event is #%i\n", last_event_index);
  for (int i = 0; i < last_event_index + 1; i++) {
    next_event[i] = next_event[i+1];
  }

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
  // initialize event to put on queue
  struct event_t event;
  event.type = SHOOT;
  event.c = m;
  event.player = player;
  printf(">> Adding %c by %s\n", event.c, player->name);
  // put event on queue
  int event_count = 0;
  sem_getvalue(&next_event_messages_waiting, &event_count);
  next_event[event_count] = event;
  // release resources
  pthread_mutex_unlock(&next_event_mutex);
  sem_post(&next_event_messages_waiting);
  event_count = 0;
  sem_getvalue(&next_event_messages_waiting, &event_count);
  printf("Completed push. Next event is #%i\n", event_count);
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
    clientCount--;
    pthread_mutex_unlock(&team_array_mutex);
    close(connfd);
    pthread_exit(0);
  }

  int n;
  
  struct player_t player = team_setup(connfd);
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
      pthread_mutex_lock(&(player.player_mutex));
      if ((n = read(connfd, player.recvBuff, sizeof player.recvBuff)) != 0) {
	  // echo all input back to client
	  //printf("Writing %d bytes to buffer\n", n);
          //write(connfd, player.recvBuff, n);
	  printf("Pushing %c onto event queue from %s\n", player.recvBuff[0], player.name);
	  push_message(player.recvBuff[0], &player);
      }
      else {
	// They disconnected-- release the client
	//printf("Server releasing client [%d remain]\n", player_count-1);
	close(connfd);
	//player_count--;
	pthread_exit(0);
      }
      pthread_mutex_unlock(&(player.player_mutex));
    }
  }
  return NULL;
}

void *loading_thread(void *arg){
  for(int i = TIMER_START; i >= 0; i--){
    sec_counter = i;
    sleep(1);
  }

  push_message('G', &SYSTEM_PLAYER);
  round_index++;
  loadMap(mapPath);

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
  
  SYSTEM_PLAYER.name = "System";

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
    printf("Accepted a connection on FD %i\n", connfd);
    pthread_t conn_thread;
    if (pthread_create(&conn_thread, NULL, client_thread, &connfd) != 0)
    {
      fprintf(stderr, "Server unable to create client_thread\n");
    }
    
    pthread_mutex_lock(&team_array_mutex);
    clientCount += 1;
    if(clientCount == 1){
      pthread_t timer_thread;
      if (pthread_create(&timer_thread, NULL, loading_thread, &connfd) != 0)
	{
	  fprintf(stderr, "Server unable to create timer_thread\n");
	}
    }
    pthread_mutex_unlock(&team_array_mutex);
  }

}
