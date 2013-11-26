#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <ncurses.h>

int team = -1; // team: either 1 or 2 once assigned
char* name = NULL;
char recvBuff[1024];
char sendBuff[1024];
int sockfd; // file descriptor for socket to server

// Sends whatever string's in sendBuff to the server.
int send_to_server() {
  return write(sockfd, sendBuff, strlen(sendBuff)+1);
}

// Sends arbitrary already-constructed string to server.
int send_str_to_server(char* str) {
  return write(sockfd, str, sizeof(char) * (strlen(str)+1));
}

// Reads from the server and dumps whatever it gets into recvBuff.
// Blocks until the server says something!
int read_from_server() {
  return read(sockfd, recvBuff, sizeof(recvBuff)-1);
}

void loading_screen(){
  initscr();/* Start curses mode   */

  for(int i = 0;i < 20;i++){
    mvprintw(i, 0, "|");
    mvprintw(i, 40, "|");
    mvprintw(i, 80, "|");
  }

  for(int i = 0;i < 81;i++){
    mvprintw(0, i, "*");
    mvprintw(20, i, "-");
  }

  mvprintw(2, 17, "Team A");
  mvprintw(2, 57, "Team B");

  while(1){
    if(read_from_server() != 0){
      /*char sec_left[10];
      char teamA[1024];
      char teamB[1024];*/
      
      mvprintw(30, 40, recvBuff);
    }

    //FIX-ME
    //need to add which players belong to which team here

    refresh();/* Print it on to the real screen */
    //getch();/* Wait for user input */
  }
  endwin();/* End curses mode  */
}

int server_connect(char* port)
{
  int sockfd;
  struct sockaddr_in serv_addr;
  char* server_text_address = "127.0.0.1"; 

  memset(recvBuff, '0',sizeof(recvBuff));
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
  {
    printf("\n Error : Could not create socket \n");
    return -1;
  } 

  memset(&serv_addr, '0', sizeof(serv_addr)); 

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(port)); 

  if(inet_pton(AF_INET, server_text_address, &serv_addr.sin_addr)<=0)
  {
    printf("\n inet_pton error occured\n");
    return -1;
  } 

  if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\n Error : Connect Failed \n");
    return -1;
  } 

  return sockfd;
}

void parse_settings(char* first_arg, char* second_arg)
{
  // parse setting data from arguments
  //char* first_arg = argv[2];
  if (first_arg != NULL) {
    if (strcmp(first_arg, "a") == 0 || strcmp(first_arg, "A") == 0)
    {
      team = 1;
      name = second_arg;
    }
    else if (strcmp(first_arg, "b") == 0 || strcmp(first_arg, "B") == 0)
    {
      team = 2;
      name = second_arg;
    }
    else
    {
      name = first_arg;
    }
  }
  // if not named, get user's login name
  if (name == NULL)
  {
    name = getlogin();
  }
}

void read_socket()
{
  int n;
  while ( (n = read(sockfd, recvBuff, sizeof recvBuff -1)) > 0)
  {
    printf("Bytes read: %d\n", n);
    //recvBuff[n] = 0;
    if(fputs(recvBuff, stdout) == EOF)
    {
      printf("\n Error : Fputs error\n");
    }
  } 

  if(n < 0)
  {
    printf("\n Read error \n");
  }
}

int main(int argc, char *argv[])
{
  if(argc < 2 || argc > 4)
  {
    printf("\n Usage: %s port [a|b] [name] \n",argv[0]);
    return 1;
  } 
  sockfd = server_connect(argv[1]);
  if (sockfd < 0) {
    // something happened while trying to connect-- abort mission
    return -1;
  }
  
  parse_settings(argv[2], argv[3]);

  printf("Request name %s and team %d\n", name, team);

  //construct proper sendBuff
  //(name, team)
  snprintf(sendBuff, sizeof sendBuff, "%s %d", name, team);

  // send it
  int writtenbytes = send_to_server();
  // read reply
  int readbytes = read_from_server();

  //printf("Bytes written: %d. Bytes read: %d.\n%s\n", writtenbytes, readbytes, recvBuff);
  printf("%s\n", recvBuff);
  char *ghs = "Game has already started";
  if(strcmp(ghs, recvBuff) == 0){
    close(sockfd);
    return 0;
  }

  /*char newName[10];
  char newTeam[10];
  char teamCount[10];
  sscanf(recvBuff, "%s %s %s", newName, newTeam, teamCount);
  printf("%s %s %s\n", newName, newTeam, teamCount);*/

  //printf("%s", recvBuff);

  // example: send preconstructed string to server
  //char* arbitrary_test_value = "This was a triumph.";
  // send it
  //writtenbytes = send_str_to_server(arbitrary_test_value);
  // read reply
  //readbytes = read_from_server();
  //printf("Bytes written: %d. Bytes read: %d.\n%s\n", writtenbytes, readbytes, recvBuff);

  //FIX-ME
  //need to add loop to refresh loading screen for T-Minus 30 seconds
  //for when new users are joining the game
  loading_screen();

  close(sockfd);
  return 0;
}
