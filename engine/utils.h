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

//#define DEBUG_FILENAME "./"
#define DEBUG_FILENAME "../logs/debug"

#define LOGGER_0(...) logger2(__VA_ARGS__)
#define NLOGGER_0(...) nlogger2(__VA_ARGS__)

#define L0 LOGGER_0 
#define L1 LOGGER_1
#define L2 LOGGER_2 
#define L3 LOGGER_3 
#define L4 LOGGER_4 

#define DEB_0(x) x
#define DEB_X(x)

#define LOGGER LOGGER_0

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG1) || defined (DEBUG4)
	#define LOGGER_1(...) logger2(__VA_ARGS__)
	#define NLOGGER_1(...) nlogger2(__VA_ARGS__)
#else
#define LOGGER_1(...)
#define NLOGGER_1(...)
#endif

#if defined (DEBUG2) || defined (DEBUG3) || defined (DEBUG4)
	#define LOGGER_2(...) logger2(__VA_ARGS__)
	#define NLOGGER_2(...) nlogger2(__VA_ARGS__)
#else
#define LOGGER_2(...)
#define NLOGGER_2(...)
#endif

#if defined (DEBUG3) || defined (DEBUG4)
	#define LOGGER_3(...) logger2(__VA_ARGS__)
	#define NLOGGER_3(...) nlogger2(__VA_ARGS__)
#else
#define LOGGER_3(...)
#define NLOGGER_3(...)
#endif

#if defined (DEBUG4)
	#define LOGGER_4(...) logger2(__VA_ARGS__)
	#define NLOGGER_4(...) nlogger2(__VA_ARGS__)
#else
#define LOGGER_4(...)
#define NLOGGER_4(...)
#endif

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG1) || defined (DEBUG4)
	#define DEB_1(x) x
#else
#define DEB_1(x)
#endif

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG4)
	#define DEB_2(x) x
#else
#define DEB_2(x)
#endif

#if defined (DEBUG3) || defined (DEBUG4)
	#define DEB_3(x) x
#else
#define DEB_3(x)
#endif

#if defined (DEBUG4)
	#define DEB_4(x) x
#else
#define DEB_4(x)
#endif

#if defined (SEDEBUG)
	#define DEB_SE(x) x
	#define LOGGER_SE(...) logger2(__VA_ARGS__)
	#define NLOGGER_SE(...) nlogger2(__VA_ARGS__)
#else
#define DEB_SE(x)
#define LOGGER_SE(...)
#define NLOGGER_SE(...)
#endif

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

#endif
