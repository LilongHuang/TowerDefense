#include "map.h"
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
#include <poll.h>
#include <ctype.h>

int team = -1; // team: either 1 or 2 once assigned
char* name = NULL;
char recvName[10];
char recvBuff[1024];
char sendBuff[1024];
int sockfd; // file descriptor for socket to server

char mapNameFromServer[1024];
char a_team[512];
char b_team[512];

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
  int stop_loading_screen = 1;
  mvprintw(0, 0, recvName);//printing name revieved from server after balancing names
  mvprintw(1, 0, "Count Down:");

  while(stop_loading_screen){
    char gisFromServer[1024];
    char* gameStart = "GameIsStarting!";
    if(read_from_server() != 0){
      sscanf(recvBuff, "%s %s %s %s", gisFromServer, mapNameFromServer, a_team, b_team);
      mvprintw(1, 12, recvBuff);
      if (strcmp(gisFromServer, gameStart) == 0) {
        stop_loading_screen = 0;
      }
     }
    refresh();/* Print it on to the real screen */
  }
}

void load_players(){
  mvprintw(30, 0, a_team);
  mvprintw(31, 0, b_team);
  char* a_token = strtok(a_team, " ");
  char* b_token = strtok(b_team, " ");
  int a_pos = 1;
  int b_pos = 11;
  while (a_token) {
    mvprintw(a_pos, 71, a_token);
    a_pos += 1;
    a_token = strtok(NULL, " ");
  }
  while (b_token) {
    mvprintw(b_pos, 71, b_token);
    b_pos += 1;
    b_token = strtok(NULL, " ");
  }

  refresh();
}

void control_test() {
  scrollok(stdscr, 1);       // allow the window to scroll
  noecho();                  // don't echo characters
  cbreak();                  // give me characters immediately
  refresh();                 // update the screen
  struct pollfd pfds[] = { {STDIN_FILENO, POLLIN, 0}, {sockfd, POLLIN, 0} };
  while (1) {
    switch (poll(pfds, 2, -1)) {

      case -1:
        perror("poll");
        exit(EXIT_FAILURE);

      default:
        if ((pfds[0].revents & POLLIN) != 0) {
          // client
          int c = getch();
          if (c == 81 || c == 113) {
            printw("Exiting.\n");
            return;
          }
          else if (iscntrl(c))
            printw("Client pressed '^%c'.\n", c + 64);
          else
            printw("Client pressed '%c'.\n", c);
            sendBuff[0] = c;
            sendBuff[1] = '\0';
            send_to_server();
          refresh();
        }
        if ((pfds[1].revents & POLLIN) != 0) {
          // server
          int c;
          ssize_t size = read(sockfd, &c, 1);
          if (size > 0) {
            if ((c & 0xFF) != 0) {
              printw("Server pressed '%c'.\n", c);
            }
          }
          else {
            printw("[Server closed connection.]\n");
            return;
          }
        refresh();
        }
    }
  }
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

void initBoard(){
  init_pair(1, COLOR_BLACK, COLOR_GREEN);
  init_pair(2, COLOR_BLACK, COLOR_RED);
  init_pair(3, COLOR_BLACK, COLOR_BLUE);
  init_pair(4, COLOR_BLACK, COLOR_WHITE);
  init_pair(5, COLOR_BLACK, COLOR_CYAN);

  //background team A (red)
  for(int i = 70; i < 80; i++){
    for(int j = 1; j <= 10; j++){
      attron(COLOR_PAIR(2));
      mvprintw(j, i, " ");
    }
  }

  //background team B (blue)
  for(int i = 70; i < 80; i++){
    for(int j = 11; j <= 20; j++){
      attron(COLOR_PAIR(3));
      mvprintw(j, i, " ");
    }
  }

  //description area (scrollable)
  for(int i = 0; i < 80; i++){
    for(int j = 21; j < 24; j++){
      attron(COLOR_PAIR(4));
      mvprintw(j, i, " ");
    }
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
  sscanf(recvBuff, "%s", recvName);
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
  initscr();
  loading_screen();
  start_color();
  loadMap(mapNameFromServer);
  initBoard();/* creates play board */
  refresh();/* Print it on to the real screen */
  load_players();
  getch();/* Wait for user input */
  endwin();
  close(sockfd);
  return 0;
}
