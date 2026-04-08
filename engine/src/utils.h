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

#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include "bitmap.h"
#include <wchar.h>

int logger(char *p, char *s, char *a);
int logger2(char*, ...);
int nlogger2(char*, ...);
int open_log(char *filename);
int close_log(void);
int flush_log(void);
char* tokenizer(char *str, char *delim, char **index);
int indexer(char *str, char *delim, char **index);
int indexof(char **index, char *str);

unsigned long long int readClock(void);

unsigned long long diffClock(struct timespec start, struct timespec end);
int readClock_wall(struct timespec *t);
int readClock_proc(struct timespec *t);
int generate_log_name(char *n, char *pref, char *b);
int parse_cmd_line_check_sec(int argc, char *argv[]);

int UTF8toWchar(unsigned char *in, wchar_t *out, size_t oll);
int WchartoUTF8(wchar_t *in, unsigned char *out, size_t oll);
void log_divider(char *s);
void dump_moves(board *b, move_cont *mc, int count, int ply, char *cmt);
int copyStats(struct _statistics *source, struct _statistics *dest);

int compareBoardSilent(board *source, board *dest);
int copyBoard(board *source, board *dest);
void printboard(board *b);
void printBoardNice(board const *b);
int boardCheck(board *b, char *name);
void eval_dump(board const *, attack_model *, personality const *);
void move_cont_dump(board const *, attack_model *, move_cont *);

#endif
