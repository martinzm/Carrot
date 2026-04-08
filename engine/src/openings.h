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

#ifndef OPENINGS_H
#define OPENINGS_H

#include "bitmap.h"
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef struct _entry_t {
	BITVAR key;
	uint16 move;
	uint16 weight;
	uint32 learn;
} entry_t;

BITVAR computeKey(board *b, BITVAR *k);
int open_open(char *filename);
void close_open();
int find_moves(board *b, BITVAR key, MOVESTORE *e, int max);
MOVESTORE probe_book(board *b);
#endif
