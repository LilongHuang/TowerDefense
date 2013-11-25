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
#include <signal.h>
#include <semaphore.h>

#define max_players 10

typedef enum {TEAM_A, TEAM_B, UNASSIGNED} team_t;

struct player_t
{
  pthread_mutex_t player_mutex;
  char *name;
  team_t team;
  int fd;
  char recvBuff[1024];
  char sendBuff[1024];
};

int team_A_counter = 0;
int team_B_counter = 0;

char a_team[1024];
char b_team[1024];

char sendBuff[1024];
char recvBuff[1024];

int clientCount = 0;

int sec_counter = 30;

struct player_t player_list[10];

pthread_mutex_t team_array_mutex = PTHREAD_MUTEX_INITIALIZER;

char mapName[1024];

void init_player(struct player_t *p){
  pthread_mutex_init( &(p->player_mutex), NULL);
  p -> team = UNASSIGNED;
  p -> fd = -1;
}

void update_a_team(char name){
  strcat(a_team, &name);
}

void update_b_team(char name){
  strcat(b_team, &name);
}

//not in use, found a better way
//keeping it for code reusability
/*void update_teams(int connfd){
  pthread_mutex_lock(&team_array_mutex);
  char teamA[1024];
  char teamB[1024];
  //int a_counter = 0;
  //int b_counter = 0;
  for(int i = 0; i < team_A_counter + team_B_counter; i++){
    struct player_t temp_player = player_list[i];
    if(temp_player.team == TEAM_A){
      //teamA[a_counter] = temp_player.name;
      strcat(teamA, temp_player.name);
      //strcat(teamA, " ");
    }
    else{
      //teamB[b_counter] = temp_player.name;
      strcat(teamB, temp_player.name);
      //strcat(teamB, " ");
    }
  }

  snprintf(current_teams, sizeof current_teams, "%s, %s", teamA, teamB);
  pthread_mutex_unlock(&team_array_mutex);
  }*/

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

  player.name = tempName;
  player.fd = connfd;

  if(strcmp(tempTeam, "1") == 0){
    //FIXME
    //update player list
    player.team = TEAM_A;
    player_list[team_A_counter + team_B_counter] = player;
    team_A_counter += 1;
    update_a_team(*tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s %s %d", tempName, tempTeam, team_A_counter);
  }
  else if(strcmp(tempTeam, "2") == 0){
    //FIXME
    //update player list
    player.team = TEAM_B;
    player_list[team_A_counter + team_B_counter] = player;
    team_B_counter += 1;
    update_b_team(*tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s %s %d", tempName, tempTeam, team_B_counter);
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

  int n;
  
  struct player_t player = team_setup(connfd);

  while (1)
  {
    /*if ((n = read(connfd, recvBuff, sizeof recvBuff)) != 0)
    {

      // echo all input back to client
      //write(connfd, sendBuff, strlen(sendBuff) + 1);

    }
    else
    {
      close(connfd);
      pthread_exit(0);
      }*/

    if(sec_counter > 0){
      //sends each client the newly updated player list
      char send[1024];
      //snprintf(send, sizeof send, "%s %s %d", a_team, b_team, sec_counter);
      snprintf(send, sizeof send, "%d", sec_counter);

      write(connfd, send, strlen(send)+1);
      sleep(1);
    }
    else if(sec_counter == 0){
      char *game_started = "Game is starting!";
      char gameStartAndMapName[1024];
      scanf(gameStartAndMapName, "%s %s", game_started, mapName);
      write(connfd, gameStartAndMapName, strlen(gameStartAndMapName)+1);
    }
    else{
      if ((n = read(connfd, recvBuff, sizeof recvBuff)) != 0)
	{
	  // echo all input back to client
	  write(connfd, recvBuff, n);
	}
      else {
	// They disconnected-- release the client
	//printf("Server releasing client [%d remain]\n", player_count-1);
	close(connfd);
	//player_count--;
	pthread_exit(0);
      }
    }
  }
  return NULL;
}

void *loading_thread(void *arg){
  for(int i = 3; i >= 0; i--){
    sec_counter = i;
    sleep(1);
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

  strncpy(mapName, argv[1], strlen(argv[1]) + 1);
  
  init_signals();
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  
  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof serv_addr);
  memset(sendBuff, '0', sizeof sendBuff);
  memset(recvBuff, '0', sizeof recvBuff);

  memset(a_team, '0', sizeof a_team);
  memset(b_team, '0', sizeof b_team);

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
