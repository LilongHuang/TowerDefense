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

char a_team[512];
char b_team[512];

char sendBuff[1024];
char recvBuff[1024];

int clientCount = 0;

int sec_counter = 30;

struct player_t player_list[10];

pthread_mutex_t team_array_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_player(struct player_t *p){
  pthread_mutex_init( &(p->player_mutex), NULL);
  p -> team = UNASSIGNED;
  p -> fd = -1;
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

  //player.name = tempName;
  player.fd = connfd;

  if(strcmp(tempTeam, "1") == 0){
    //FIXME
    //update player list
    player.team = TEAM_A;
    //player_list[team_A_counter + team_B_counter] = player;
    //team_A_counter += 1;
    update_a_team(&player, tempName);

    snprintf(player.sendBuff, sizeof player.sendBuff, "%s %s %d", tempName, tempTeam, team_A_counter);
  }
  else if(strcmp(tempTeam, "2") == 0){
    //FIXME
    //update player list
    player.team = TEAM_B;
    //player_list[team_A_counter + team_B_counter] = player;
    //team_B_counter += 1;
    update_b_team(&player, tempName);

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
      snprintf(send, sizeof send, "%s | %s | %d", a_team, b_team, sec_counter);

      write(connfd, send, strlen(send)+1);
      sleep(1);
    }
    else if(sec_counter == 0){
      char *game_started = "Game is starting!";
      write(connfd, game_started, strlen(game_started)+1);
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
  for(int i = 30; i >= 0; i--){
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
