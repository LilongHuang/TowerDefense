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
#define players_per_team 5
#define TIMER_START 10
#define EVENT_QUEUE_SIZE 20

typedef enum {TEAM_A, TEAM_B, UNASSIGNED} team_t;

struct player_t
{
  pthread_mutex_t player_mutex;
  char *name;
  team_t team;
  int r; // row; y-coordinate
  int c; // column; x-coordinate
  int fd;
  char recvBuff[1024];
  char sendBuff[1024];
};

typedef enum {MOVE_REL, MOVE_ABS, SHOOT} event_type_t;

struct event_t
{
  event_type_t type;
  int c;
  char* player;
  //int rows;
  //int cols;
};

int team_A_counter = 0;
int team_B_counter = 0;

char a_team[512];
char b_team[512];

char sendBuff[1024];
char recvBuff[1024];

int clientCount = 0;

int sec_counter = TIMER_START;

struct player_t player_list[10];
pthread_mutex_t team_array_mutex = PTHREAD_MUTEX_INITIALIZER;

char mapName[1024];

struct event_t next_event[EVENT_QUEUE_SIZE];
int event_count = 0;
pthread_mutex_t next_event_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t next_event_space_remaining;
sem_t next_event_messages_waiting;

void init_player(struct player_t *p){
  pthread_mutex_init( &(p->player_mutex), NULL);
  p -> team = UNASSIGNED;
  p -> fd = -1;
}

void a_to_b_team(){
  for(int i = (team_A_counter + team_B_counter)-1; i > 0; i--){
    struct player_t p = player_list[i];
    if(p.team == TEAM_A){
      p.team = TEAM_B;
      team_A_counter -= 1;
      team_B_counter += 1;
      break;
    }
  }
}

void b_to_a_team(){
  for(int i = (team_A_counter + team_B_counter)-1; i > 0; i--){
    struct player_t p = player_list[i];
    if(p.team == TEAM_B){
      p.team = TEAM_A;
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
  char temp_a_team[512];
  char temp_b_team[512];
  for(int i = 0; i < (team_A_counter + team_B_counter); i++){
    struct player_t p = player_list[i];
    if(p.team == TEAM_A){
      strcat(temp_a_team, p.name);
      strcat(temp_a_team, " ");
    }
    else{
      strcat(temp_b_team, p.name);
      strcat(temp_b_team, " ");
    }
  }
  strcpy(a_team, temp_a_team);
  strcpy(b_team, temp_b_team);
}

int balance_names(char *name){
  for(int i = 0; i < team_A_counter + team_B_counter; i++){
    struct player_t temp_player = player_list[i];

    if(strcmp(name, temp_player.name) == 0){
      int rn = rand()%9000%1000;
      char rn_string[128];
      sprintf(rn_string, "%d", rn);
      strcat(name, rn_string);

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
  //char temp[512];
  /*if(strlen(a_team) > 0){
    snprintf(temp, sizeof temp, "%s, %s", a_team, name);
  }
  else{
    snprintf(temp, sizeof temp, "%s", name);
  }
  strcpy(a_team, temp);*/
}

void update_b_team(struct player_t *p, char *name){
  while(balance_names(name) == 0){}
  p -> name = name;
  player_list[team_A_counter + team_B_counter] = *p;
  team_B_counter += 1;
  //char temp[512];
  /*if(strlen(b_team) > 0){
    snprintf(temp, sizeof temp, "%s, %s", b_team, name);
  }
  else{
    snprintf(temp, sizeof temp, "%s", name);
  }
  strcpy(b_team, temp);*/
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

  int n;

  pthread_mutex_lock(&player.player_mutex);

  n = read(connfd, player.recvBuff, sizeof player.recvBuff);

  char tempName[10];
  char tempTeam[10];
  sscanf(player.recvBuff, "%s %s", tempName, tempTeam);

  player.fd = connfd;

  if(strcmp(tempTeam, "1") == 0){
    //update player list
    if(team_A_counter < players_per_team){
      player.team = TEAM_A;
      update_a_team(&player, tempName);
    }
    else{
      player.team = TEAM_B;
      update_b_team(&player, tempName);
    }

    //update_a_team(&player, tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s", player.name);
  }
  else if(strcmp(tempTeam, "2") == 0){
    //update player list
    if(team_B_counter < players_per_team){
      player.team = TEAM_B;
      update_b_team(&player, tempName);
    }
    else{
      player.team = TEAM_A;
      update_a_team(&player, tempName);
    }

    //update_b_team(&player, tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s", player.name);
  }

  // echo all input back to client
  write(connfd, player.sendBuff, strlen(player.sendBuff) + 1);

  pthread_mutex_unlock(&player.player_mutex);

  pthread_mutex_unlock(&team_array_mutex);
  return player;
}

void *client_thread(void *arg)
{
  int connfd = *((int *)arg);

  //int length = sprintf(sendBuff, "[Clients: %d]", player_count);
  //write(connfd, sendBuff, length+1);


  if(sec_counter == 0){
    char *ghs = "Game has already started";
    write(connfd, ghs, strlen(ghs)+1);
    close(connfd);
    pthread_exit(0);
  }

  if((team_A_counter + team_B_counter) > 9){
    char *full_game = "Already 10 players joined";
    write(connfd, full_game, strlen(full_game)+1);
    close(connfd);
    pthread_exit(0);
  }

  int n;
  
  struct player_t player = team_setup(connfd);

  while (1)
  {
    if(sec_counter > 0){
      //sends each client the newly updated player list
      char send[1024];
      snprintf(send, sizeof send, "%d", sec_counter);

      write(connfd, send, strlen(send)+1);
      sleep(1);
    }
    else if(sec_counter == 0){
      //balance_teams();
      //printf("TeamA: %s\nTeamB: %s\n", a_team, b_team);
      char *game_started = "GameIsStarting!";
      char gameStartAndMapName[1024];
      sprintf(gameStartAndMapName, "%s %s %s %s", game_started, mapName, a_team, b_team);
      write(connfd, gameStartAndMapName, strlen(gameStartAndMapName)+1);
    }
    else{
      /*if ((n = read(connfd, recvBuff, sizeof recvBuff)) != 0)
	{
	  // echo all input back to client
	  write(connfd, recvBuff, n);
	  
	  // capture event queue, and prepare to add a new event
	  pthread_mutex_lock(&next_event_mutex);
	  sem_wait(&next_event_space_remaining);
	  // initialize event to put on queue
	  struct event_t event;
	  event.type = SHOOT;
	  event.c = recvBuff[0];
	  event.player = player.name;
	  printf("Added %c by %s\n", event.c, player.name);
	  // put event on queue
	  int event_count;
	  sem_getvalue(&next_event_messages_waiting, &event_count);
	  next_event[event_count] = event;
	  // release resources
	  pthread_mutex_unlock(&next_event_mutex);
	  sem_post(&next_event_messages_waiting);
	}
      else {
	// They disconnected-- release the client
	//printf("Server releasing client [%d remain]\n", player_count-1);
	close(connfd);
	//player_count--;
	pthread_exit(0);
	}*/
    }
  }
  return NULL;
}

void pop_message(void) {
  sem_wait(&next_event_messages_waiting);
  pthread_mutex_lock(&next_event_mutex);
  
  struct event_t event = next_event[0];
  
  // process message
  for (int i = 0; i < clientCount; i++) {
    struct player_t p = player_list[i];
    pthread_mutex_lock(&(p.player_mutex));
    // FIXME figure out why p.name returns "(null)"
    int n = sprintf(p.sendBuff, "Hey %i: %s hit %c\n", p.fd, event.player, event.c);
    write(player_list[i].fd, p.sendBuff, n);
    pthread_mutex_unlock(&(p.player_mutex));
  }
  
  // slide all events over to fill empty spot
  int last_event_index;
  sem_getvalue(&next_event_messages_waiting, &last_event_index);
  for (int i = 0; i < last_event_index + 1; i++) {
    next_event[i] = next_event[i+1];
  }

  // release resources
  pthread_mutex_unlock(&next_event_mutex);
  sem_post(&next_event_space_remaining);
}

void *loading_thread(void *arg){
  for(int i = TIMER_START; i >= -1; i--){
    printf("%d\n", i);
    if(i == 0){
      balance_teams();
      create_teams();
    }
    sec_counter = i;
    sleep(1);
  }
  
  /*while (1) {
    pop_message();
    }*/
  
  return NULL;
}

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf("Usage: %s mapfile port\n", argv[0]);
    return 1;
  }

  strncpy(mapName, argv[1], strlen(argv[1]) + 1);
  
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

    pthread_t conn_thread;
    if (pthread_create(&conn_thread, NULL, client_thread, &connfd) != 0)
    {
      fprintf(stderr, "Server unable to create client_thread\n");
    }
    
    clientCount += 1;
    if(clientCount == 1){
      pthread_t timer_thread;
      if (pthread_create(&timer_thread, NULL, loading_thread, &connfd) != 0)
	{
	  fprintf(stderr, "Server unable to create timer_thread\n");
	}
    }
    
    sleep(1);
  }

}
