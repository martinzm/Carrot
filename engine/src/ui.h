/*
 Carrot is a UCI chess playing engine by Martin Žampach.
 <https://github.com/martinzm/Carrot>     <martinzm@centrum.cz>

 Carrot is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Carrot is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#ifndef UI_H
#define UI_H
#include "bitmap.h"
int uci_loop(int second);
int uci_loop2(int second);
int tell_to_engine(char *s);
int move_filter_build(char *str, MOVESTORE *m);

int start_threads(board *b);
board* allocate_board();
int allocate_tables(board *b);
int stop_threads(board *b);
int deallocate_tables(board *b);
int deallocate_board(board *b);

#endif
