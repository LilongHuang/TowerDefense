#include <ncurses.h>
#include "map.h"
#include <string.h>

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

int main(int argc, char* argv[]){
  initscr();/* Start curses mode   */
  start_color();
  loadMap(argc, argv);
  initBoard();/* creates play board */
  //loadMap(argc, argv);
  refresh();/* Print it on to the real screen */
  getch();/* Wait for user input */
  endwin();/* End curses mode  */

  return 0;
}

