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

char sendBuff[1024];
char recvBuff[1024];
int player_count = 0; // note to self: mutex/semaphore this
// remember to mutex/semaphore the map data, so it's all thread-safe

#define MAX_PLAYERS 10
const char quit_signal = 'q';

void handle_sigpipe(int signal) {
  printf("%s", "Sigpipe\n");
}

void init_signals(void) {
  // use sigaction(2) to catch SIGPIPE somehow
  struct sigaction sigpipe;
  memset(&sigpipe, 0, sizeof(sigpipe));
  sigpipe.sa_handler = &handle_sigpipe;
  sigaction(SIGPIPE, &sigpipe, NULL);
  return;
}

void *client_thread(void *arg)
{
  int connfd = *((int *)arg);
  int n;
  
  int length = sprintf(sendBuff, "[Clients: %d]", player_count);
  write(connfd, sendBuff, length+1);
  while (1)
  {
    if ((n = read(connfd, recvBuff, sizeof recvBuff)) != 0)
    {
      // echo all input back to client
      write(connfd, recvBuff, n);
    }
    else {
      // They disconnected-- release the client
      printf("Server releasing client [%d remain]\n", player_count-1);
      close(connfd);
      player_count--;
      pthread_exit(0);
    }
  }
}

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf("Usage: %s mapfile port\n", argv[0]);
    return 1;
  }
  
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);

  init_signals();  

  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof serv_addr);
  memset(sendBuff, '0', sizeof sendBuff);
  memset(recvBuff, '0', sizeof recvBuff);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(strtoumax(argv[2], NULL, 10));

  bind(listenfd, (struct sockaddr*)&serv_addr, sizeof serv_addr);
  listen(listenfd, 10);

  while(1)
  {
    int connfd = accept(listenfd, (struct sockaddr*) NULL, NULL);
    
    player_count++;
    
    pthread_t conn_thread;
    if (pthread_create(&conn_thread, NULL, client_thread, &connfd) != 0)
    {
      fprintf(stderr, "Server unable to create thread\n");
    }
    
    sleep(1);
  }

}
