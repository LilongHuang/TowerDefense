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

char sendBuff[1024];
char recvBuff[1024];

void *client_thread(void *arg)
{
  int connfd = *((int *)arg);
  int n;
  while (1)
  {
    
    /*message_size = snprintf(sendBuff, sizeof sendBuff, "%.24s\r\n", ctime(&ticks));
    write(connfd, sendBuff, message_size+1);
    message_size = snprintf(sendBuff, sizeof sendBuff, "%.24s\r\n", ctime(&ticks));
    write(connfd, sendBuff, message_size+1);*/
    
    
    
    if ((n = read(connfd, recvBuff, sizeof recvBuff)) != 0)
    {
      // echo all input back to client
      write(connfd, recvBuff, n);
    }
    else
    {
      close(connfd);
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
    
    pthread_t conn_thread;
    if (pthread_create(&conn_thread, NULL, client_thread, &connfd) != 0)
    {
      fprintf(stderr, "Server unable to create thread\n");
    }
    
    sleep(1);
  }

}
