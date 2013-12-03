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
int player_color = 0;
int sockfd; // file descriptor for socket to server
char mapNameFromServer[1024];
char a_team[512];
char b_team[512];

struct player_t {
  char name[10];
  int x;
  int y;
  int player_color;
  int symbol;
  int score;
  int score_x;
  int score_y;
};

struct player_t Aplayer_list[5];
struct player_t Bplayer_list[5];

int A_Player_Count = 0;
int B_Player_Count = 0;

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

void init_color_pairs(){
  init_pair(11, COLOR_BLACK, 45);//color 1
  init_pair(12, COLOR_BLACK, 155);//color 2
  init_pair(13, COLOR_BLACK, 65);//color 3
  init_pair(14, COLOR_BLACK, 75);//color 4
  init_pair(15, COLOR_BLACK, 85);//color 5
  init_pair(16, COLOR_BLACK, 95);//color 6
  init_pair(17, COLOR_BLACK, 105);//color 7
  init_pair(18, COLOR_BLACK, 115);//color 8
  init_pair(19, COLOR_BLACK, 125);//color 9
  init_pair(20, COLOR_BLACK, 135);//color 10
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
  clear();
}

void display_teams(){
  mvprintw(0, 0, "Team A (red):");
  mvprintw(0, 20, "Team B (blue):");

  char temp_a_team[512];
  strcpy(temp_a_team, a_team);

  char* a_token = strtok(temp_a_team, ",");
  int a_pos = 1;
  while (a_token) {
    a_token += 2;
    mvprintw(a_pos, 0, a_token);
    a_pos += 1;
    a_token = strtok(NULL, ",");
  }

  char temp_b_team[512];
  strcpy(temp_b_team, b_team);

  char* b_token = strtok(temp_b_team, ",");
  int b_pos = 1;
  while (b_token) {
    b_token += 2;
    mvprintw(b_pos, 20, b_token);
    b_pos += 1;
    b_token = strtok(NULL, ",");
  }
  refresh();

  sleep(5);
  clear();
}

void load_players(){
  refresh();
  init_color_pairs();
  char* a_token = strtok(a_team, ",");
  int a_pos = 1;

  //attron(COLOR_PAIR(11));
  while (a_token) {
    char c_pair[4];
    char c_pair2[4];
    sprintf(c_pair, "%c", a_token[0]);
    sprintf(c_pair2, "%c", a_token[1]);
    strcat(c_pair, c_pair2);
    int cp = atoi(c_pair);
    attron(COLOR_PAIR(cp));
    a_token += 2;

    if(strcmp(recvName, a_token) == 0)
      player_color = cp;

    mvprintw(a_pos, 71, a_token);
    mvprintw(a_pos+1, 71, "0");
    //a_pos += 2;

    struct player_t p;
    strcpy(p.name, a_token);
    p.score = 0;
    p.score_x = 71;
    p.score_y = a_pos+1;
    p.player_color = cp;
    p.symbol = 0;
    Aplayer_list[A_Player_Count] = p;

    a_pos += 2;

    A_Player_Count++;
    a_token = strtok(NULL, ",");
  }

  char* b_token = strtok(b_team, ",");
  int b_pos = 11;
  while (b_token) {
    char c_pair[4];
    char c_pair2[4];
    sprintf(c_pair, "%c", b_token[0]);
    sprintf(c_pair2, "%c", b_token[1]);
    strcat(c_pair, c_pair2);
    int cp = atoi(c_pair);
    attron(COLOR_PAIR(cp));
    b_token += 2;

    if(strcmp(recvName, b_token) == 0)
      player_color = cp;

    mvprintw(b_pos, 71, b_token);
    mvprintw(b_pos+1, 71, "0");
    //b_pos += 2;

    struct player_t p;
    strcpy(p.name, b_token);
    p.score = 0;
    p.score_x = 71;
    p.score_y = b_pos+1;
    p.player_color = cp;
    p.symbol = 1;
    Bplayer_list[B_Player_Count] = p;

    b_pos += 2;

    B_Player_Count++;

    b_token = strtok(NULL, ",");
  }
  mvprintw(25, 0, "");
  refresh();
}

void place_players() {
  int att_respawn_points = attackerRespawnPointCount - 1;
  int def_respawn_points = defenderRespawnPointCount - 1;
  char a[10];
  char b[10];
  sprintf(a, "A");
  sprintf(b, "B");
  for (int i = 0; i < A_Player_Count; i++) {
    struct player_t p = Aplayer_list[i];
    attron(COLOR_PAIR(p.player_color));
    int x = attacker_respawn_location_list[att_respawn_points].x;
    int y = attacker_respawn_location_list[att_respawn_points].y;
    setCharOnMap(*a, x, y);
    def_respawn_points--;
  }

  for (int i = 0; i < B_Player_Count; i++) {
    struct player_t p = Bplayer_list[i];
    attron(COLOR_PAIR(p.player_color));
    int x = defender_respawn_location_list[def_respawn_points].x;
    int y = defender_respawn_location_list[def_respawn_points].y;
    setCharOnMap(*b, x, y);
    def_respawn_points--;
  }

  refresh();
}

void parse_message(char* message) {
  // determine what flavor of message
  // possible message types:
  // RENDER_FORMAT_STRING: render a character at x,y with colors A and B
  // "castle %i": update the displayed percentage of castle walls remaining to this
  refresh();
}

void update_a_team(){
  int a_pos = 1;
  for(int i = 0; i < A_Player_Count; i++){
    struct player_t p = Aplayer_list[i];
    attron(COLOR_PAIR(p.player_color));
    mvprintw(a_pos, 71, p.name);
    char temp_score[512];
    sprintf(temp_score, "%d", p.score);
    mvprintw(a_pos+1, 71, temp_score);
    attroff(COLOR_PAIR(p.player_color));
  }
}

void update_b_team(){
  int b_pos = 1;
  for(int i = 0; i < B_Player_Count; i++){
    struct player_t p = Bplayer_list[i];
    attron(COLOR_PAIR(p.player_color));
    mvprintw(b_pos, 71, p.name);
    char temp_score[512];
    sprintf(temp_score, "%d", p.score);
    mvprintw(b_pos+1, 71, temp_score);
    attroff(COLOR_PAIR(p.player_color));
  }
}

bool is_valid_char(char c) {
  return true;
}

void control_test() {
  scrollok(stdscr, 1);       // allow the window to scroll
  noecho();                  // don't echo characters
  cbreak();                  // give me characters immediately
  refresh();                 // update the screen
  standend();
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
          /*if (c == 80 || c == 112) {
            printw("Exiting.\n");
            return;
          }
          else if (iscntrl(c))
            //printw("Client pressed '^%c'.\n", c + 64);
          else {*/
            //printw("Client pressed '%c'.\n", c);
	  if (is_valid_char(c)) {  
            sendBuff[0] = c;
            sendBuff[1] = '\0';
            send_to_server();
          }
          //}
        }
        if ((pfds[1].revents & POLLIN) != 0) {
          // server
          ssize_t size = read(sockfd, recvBuff, sizeof recvBuff);
          if (size > 0) {

	    char read_type[256];
            sscanf(recvBuff, "%s", read_type);

            //for updating player scores
            if(strcmp(read_type, "score") == 0){
              char color[10];
              char new_score[512];
              sscanf(recvBuff, "%s %s %s", read_type, color, new_score);
              for(int i = 0; i < A_Player_Count; i++){
                struct player_t temp_player = Aplayer_list[i];
                if(temp_player.player_color == atoi(color)){
                  temp_player.score = atoi(new_score);
                  //update_a_team();
		  mvprintw(temp_player.score_y, temp_player.score_x, new_score);
                  break;
                }
              }

              for(int i = 0; i < B_Player_Count; i++){
                struct player_t temp_player = Aplayer_list[i];
                if(temp_player.player_color == atoi(color)){
                  temp_player.score = atoi(new_score);
                  //update_b_team();
		  mvprintw(temp_player.score_y, temp_player.score_x, new_score);
                  break;
                }
              }
            }

            //for updating characters on battlefield
            if(strcmp(read_type, "render") == 0){
              char c[512];
              char temp_x[512];
              char temp_y[512];
              char temp_color_foreground[512];
              char temp_color_background[512];
              sscanf(recvBuff, "%s %s %s %s %s %s", read_type, c, temp_x, temp_y, temp_color_foreground, temp_color_background);
              int x = atoi(temp_x);
              int y = atoi(temp_y);
              int char_index = atoi(c);
              c[0] = (char)char_index;
              c[1] = '\0';
              int color_a = atoi(temp_color_foreground);
              int color_b = atoi(temp_color_background);
              attron(COLOR_PAIR(color_a));
              mvprintw(y+1, x, c);
              move(25, 0);
              attroff(COLOR_PAIR(color_a));
              refresh();
            }

	    //for updating timer on battlefield
            if(strcmp(read_type, "timer") == 0){
              char new_time[128];
              sscanf(recvBuff, "%s %s", read_type, new_time);
              struct round_counter rc = getRoundCounter();
              mvprintw(rc.y+1, rc.x, new_time);
              
              //printf("Time is now %s\n", new_time);
            }

            //for updating castle percentage on battlefield
            if(strcmp(read_type, "castle") == 0){
              char new_percent[128];
              sscanf(recvBuff, "%s %s", read_type, new_percent);
              struct percent_wall pw = getPercentWall();
              mvprintw(pw.y, pw.x, new_percent);
            }
	    
            /*mvprintw(30, 0, "Server sent '%s'.\n", recvBuff);
	    for(int i = 0; i < 20; i++) {
	      mvprintw(i + 1, 0, list_row[i].content);
	    }*/	   
	     
          }
          else {
            printw("[Server closed connection.]\n");
            return;
          }
        }
    }
    refresh();
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

void final_standings(){
  clear();

  mvprintw(0, 0, "Team A (red)");
  mvprintw(0, 20, "Team B (blue)");

  int a_pos = 1;
  //team a
  for(int i = 0; i < A_Player_Count; i++){
    struct player_t p = Aplayer_list[i];
    attron(COLOR_PAIR(p.player_color));
    mvprintw(a_pos, 0, p.name);
    char p_score[512];
    sprintf(p_score, "%d", p.score);
    mvprintw(a_pos+1, 0, p_score);
    attroff(COLOR_PAIR(p.player_color));
  }

  int b_pos = 1;
  //team b
  for(int i = 0; i < B_Player_Count; i++){
    struct player_t p = Aplayer_list[i];
    attron(COLOR_PAIR(p.player_color));
    mvprintw(b_pos, 20, p.name);
    char p_score[512];
    sprintf(p_score, "%d", p.score);
    mvprintw(b_pos+1, 20, p_score);
    attroff(COLOR_PAIR(p.player_color));
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
  send_to_server();
  // read reply
  read_from_server();

  //printf("Bytes written: %d. Bytes read: %d.\n%s\n", writtenbytes, readbytes, recvBuff);
  sscanf(recvBuff, "%s", recvName);
  printf("%s\n", recvBuff);
  char *ghs = "Game has already started";
  if(strcmp(ghs, recvBuff) == 0){
    close(sockfd);
    return 0;
  }

  initscr();
  loading_screen();
  display_teams();
  start_color();
  initscr();
  loadMap(mapNameFromServer);
  teamInfoMap();
  initBoard();/* creates play board */
  refresh();/* Print it on to the real screen */
  load_players();
  //place_players();
  control_test();
  final_standings();
  endwin();

  close(sockfd);
  return 0;
}
