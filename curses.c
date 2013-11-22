#include <ncurses.h>
//#include "map.h"

void initBoard(){

  init_pair(1, COLOR_BLACK, COLOR_GREEN);
  init_pair(2, COLOR_BLACK, COLOR_RED);
  init_pair(3, COLOR_BLACK, COLOR_BLUE);
  init_pair(4, COLOR_BLACK, COLOR_WHITE);
  init_pair(5, COLOR_BLACK, COLOR_CYAN);
  
  attron(COLOR_PAIR(5));
  //for(int i = 0;i < 70; i++){
  //top bar:map name, author, defensive team
  mvprintw(0, 1, "1234");
    
  //}

  for(int i = 1;i < 80; i++){
    //bottom of play board
    mvprintw(24, i, "-");
    //play area horizontal sections
    mvprintw(20, i, "-");
  }

  for(int i = 0;i < 25; i++){
    //left side of play board
    mvprintw(i, 0, "|");
    //right side of play board
    mvprintw(i, 80, "|");
  }

  for(int i = 69;i < 80; i++){
    //top of team sections
    mvprintw(0, i, "-");
    //team section divisor
    mvprintw(10, i, "-");
  }

  for(int i = 1;i < 20; i++){
    //play area vertical sections
    mvprintw(i, 69, "|");
  }

  //background for play area
  for(int i = 1; i < 69; i++){
    for(int j = 1; j < 20; j++){
      attron(COLOR_PAIR(1));
      mvprintw(j, i, " ");
    }
  }

  //background team A (red)
  for(int i = 70; i < 80; i++){
    for(int j = 1; j < 10; j++){
      attron(COLOR_PAIR(2));
      mvprintw(j, i, " ");
    }
  }

  //background team B (blue)
  for(int i = 70; i < 80; i++){
    for(int j = 11; j < 20; j++){
      attron(COLOR_PAIR(3));
      mvprintw(j, i, " ");
    }
  }

  //description area (scrollable)
  for(int i = 1; i < 80; i++){
    for(int j = 21; j < 24; j++){
      attron(COLOR_PAIR(4));
      mvprintw(j, i, " ");
    }
  }

}

int main(int argc, char *argv[]){
  //loadMap(argc, argv[1]);
  initscr();/* Start curses mode   */
  start_color();

  initBoard();/* creates play board */

  refresh();/* Print it on to the real screen */
  getch();/* Wait for user input */
  endwin();/* End curses mode  */

  return 0;
}

