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

#ifndef SEARCH_H
#define SEARCH_H

#include "bitmap.h"
#include <limits.h>
int ABNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store *tree, int nulls, attack_model*);

void clearSearchCnt(struct _statistics *s);
void AddSearchCnt(struct _statistics*, struct _statistics*);
void DecSearchCnt(struct _statistics*, struct _statistics*, struct _statistics*);
void CopySearchCnt(struct _statistics*, struct _statistics*);
void printSearchStat(struct _statistics*);
void printSearchStat2(struct _statistics*, char*);

void sprintfPV(tree_store *tree, int depth, char *buff);
int initDBoards();
int IterativeSearchN(board *b, int alfa, int beta, int depth, int side, int start_depth, tree_store *tree);
int QuiesceNew(board *b, int, int, int, int, int, tree_store*, int,  attack_model*);
void printPV_simple_act(board*, tree_store*, int, int, struct _statistics*);

#endif
