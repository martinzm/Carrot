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

#include "ui.h"
#include "bitmap.h"
#include "generate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tests.h"
#include "hash.h"
#include "pers.h"
#include "utils.h"
#include "evaluate.h"
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include "tuner.h"

//#define NUM_THREADS 1
#define INPUT_BUFFER_SIZE 16384

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

void sleep_ms(int milliseconds)  // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
	usleep(milliseconds * 1000);
#endif
}

// komunikace mezi enginem a uci rozhranim
// bs je nastavovano uci rozhranim a je pouzivano enginem na vypocty

// engine state rika
// uci state dtto pro uci stav
// engine_stop - 0 ok, 1 zastav! pouziti pokud je engine "THINKING"
int engine_state;
int uci_state;

int tell_to_engine(char *s)
{
	fprintf(stdout, "%s\n", s);
	LOGGER_4("TO_E: %s\n",s);
	return 0;
}

int uci_send_bestmove(MOVESTORE b)
{
	char buf[50], b2[100];
	LOGGER_4("INFO: bestmove sending\n");
	sprintfMoveSimple(b, buf);
	sprintf(b2, "bestmove %s\n", buf);
	tell_to_engine(b2);
	LOGGER_4("INFO: bestmove sent");
	return 0;
}

/*
 * engine states
 * 0 - make_quit
 * 4 - stop_thinking
 * 1 - stopped
 * 3 - start_thinking
 * 2 - thinking
 */

void* engine_thread(void *arg)
{
	tree_store *moves;
	struct _statistics *stat;
	board *b;

	moves = (tree_store*) malloc(sizeof(tree_store));
	stat = allocate_stats(1);
	moves->tree_board.stats = (stat);
	b = (board*) arg;
	engine_stop = 1;
	LOGGER_4("THREAD: started\n");
	while (engine_state != MAKE_QUIT) {
		switch (engine_state) {
		case START_THINKING:
			LOGGER_4("THREAD: Thinking\n");
			engine_stop = 0;
			engine_state = THINKING;
			IterativeSearchN(b, 0 - iINFINITY, iINFINITY,
				b->uci_options->depth, b->side,
				b->pers->start_depth, moves);
			LOGGER_4("THREAD: Iter Left\n");
			engine_stop = 4;
			engine_state = STOPPED;
			uci_state = 2;
			if (b->bestmove != 0)
				uci_send_bestmove(b->bestmove);
			else {
				LOGGER_3("INFO: no bestmove!\n");
				uci_send_bestmove(moves->tree[0][0].move);
			}
			break;
		case STOPPED:
			sleep_ms(1);
			LOGGER_4("THREAD: Stopped\n");
			break;
		}
	}
	deallocate_stats(stat);
	free(moves);
	LOGGER_4("THREAD: quit\n");
	return arg;
}

int handle_uci()
{
	char buff[1024];
	sprintf(buff, "id name %s v%s, REL %s, %s %s\n", eNAME, eVERS, eREL,
		__DATE__, __TIME__);
	tell_to_engine(buff);
	sprintf(buff, "id author Martin Zampach\n");
	tell_to_engine(buff);
	sprintf(buff, "uciok\n");
	tell_to_engine(buff);
	return 0;
}

int handle_newgame(board *bs)
{
	setup_normal_board(bs);
	LOGGER_4("INFO: newgame\n");
	return 0;
}

/* promotion field has special meaning
 values - KNIGHT, BISHOP, ROOK, QUEEN mean promotion to
 ER_PIECE no promotion
 KING indicates castling
 PAWN indicates EP
 ER_PIECE+1 indicates DoublePush
 */

// ep - spec flag
// castle - spec flag, fix dest pole
int move_filter_build(char *str, MOVESTORE *m)
{
	char *tok, *b2;
	int i, a, b, c, d, q, spec;
	int from, to, prom;
	MOVESTORE v;
	size_t l;
	// replay moves
	i = 0;
	m[0] = 0;
	if (!str)
		return 0;
	tok = tokenizer(str, " \n\r\t", &b2);
	
// move validation is missing ! FIXME!!
	
	if (tok == NULL)
		return 0;
	do {
		// tah je ve tvaru AlphaNumAlphaNum(Alpha*)

		l = strlen(tok);
		a = b = c = d = q = spec = 0;
		if (l >= 4 && l <= 5) {
			if (isalpha(tok[0]))
				if ((tolower(tok[0]) >= 'a')
					&& (tolower(tok[0]) <= 'h')) {
					a = tolower(tok[0]) - 'a';
				}
			if (isdigit(tok[1]))
				if ((tok[1] >= '0') && (tok[1] <= '9')) {
					b = tok[1] - '0' - 1;
				}
			if (isalpha(tok[2]))
				if ((tolower(tok[2]) >= 'a')
					&& (tolower(tok[2]) <= 'h')) {
					c = tolower(tok[2]) - 'a';
				}
			if (isdigit(tok[3]))
				if ((tok[3] >= '0') && (tok[3] <= '9')) {
					d = tok[3] - '0' - 1;
				}
// promotion
			q = ER_PIECE;
			if (l == 5) {
				if (isalpha(tok[4]))
					switch (tolower(tok[4])) {
					case 'b':
						q = BISHOP;
						break;
					case 'n':
						q = KNIGHT;
						break;
					case 'r':
						q = ROOK;
						break;
					case 'q':
						q = QUEEN;
						break;
					default:
						spec = 0;
						break;
					}
			}
			
			from = b * 8 + a;
			to = d * 8 + c;
			prom = q;

			// ep from, to, PAWN
			// castling e1g1 atd
			v = PackMove(from, to, prom, 0);
			m[i++] = v;
		}
		if (!b2)
			break;
		tok = tokenizer(b2, " \n\r\t", &b2);
	} while (tok != NULL);

	m[i++] = 0;
	return i;
}

int handle_position(board *bs, char *str)
{

	char *tok, *b2, bb[100];
	char *fen, *side, *roch, *ep, *half, *full;
	int i, a;
	MOVESTORE m[MAXPLYHIST], mm[MAXPLYHIST];
	int from, pos[4];
	int oldp;
	UNDO u;

	if (engine_state != STOPPED) {
		LOGGER_3("UCI: INFO: Not stopped!, E:%d U:%d\n", engine_state, uci_state);
		engine_state = STOP_THINKING;

		sleep_ms(10);
		while (engine_state != STOPPED) {
			LOGGER_4("UCI: INFO: Stopping!, E:%d U:%d\n", engine_state, uci_state);
			engine_state = STOP_THINKING;
			sleep_ms(10);
		}
	}

	LOGGER_4("B.T.: %s\n", str);
	bs->uci_options->newgame=0;
	
	tok = tokenizer(str, " \n\r\t", &b2);
	while (tok) {
		LOGGER_4("PARSE: %s\n",tok);

		if (!strcasecmp(tok, "fen")) {
			LOGGER_4("INFO: FEN+moves %s\n",b2);
			setup_FEN_board(bs, b2);
			fen = b2;
			tok = tokenizer(fen, " \n\r\t", &side);
			tok = tokenizer(side, " \n\r\t", &roch);
			tok = tokenizer(roch, " \n\r\t", &ep);
			tok = tokenizer(ep, " \n\r\t", &half);
			tok = tokenizer(half, " \n\r\t", &full);
			tok = tokenizer(full, " \n\r\t", &b2);
			sprintf(bb,"%s %s %s %s %s %s", fen, side, roch, ep, half, full);
			if(strncmp(bs->uci_options->oldfen, bb, 99) == 0) bs->uci_options->newgame=0;
			else {
//				L0("%s\n",bb);
//				L0("%s\n", bs->uci_options->oldfen);
				strncpy(bs->uci_options->oldfen, bb, 100);
				bs->uci_options->newgame=1;
				LOGGER_1("INFO: UCI new game fen\n");
			}
		} else if (!strcasecmp(tok, "startpos")) {
			LOGGER_4("INFO: startpos %s\n",b2);
			setup_normal_board(bs);
			if(strncmp(bs->uci_options->oldfen, STANDARD_FEN, 99) == 0) bs->uci_options->newgame=0;
			else {
				strncpy(bs->uci_options->oldfen, STANDARD_FEN, 100);
				bs->uci_options->newgame=1;
				LOGGER_1("INFO: UCI new game st fen\n");
			}
			DEB_4(printBoardNice(bs);)
		} else if (!strcasecmp(tok, "moves")) {
// build filter moves
			LOGGER_4("MOVES: %s\n", b2);
			move_filter_build(b2, m);
			a = 0;
			mm[1] = 0;
			while (m[a] != 0) {
				mm[0] = m[a];
				DEB_3(sprintfMoveSimple(mm[0], bb);)
				i = alternateMovGen(bs, mm);
				if (i != 1) {
					DEB_4(printBoardNice(bs);)
					LOGGER_4("%d:%s\n",a, b2); 
					LOGGER_4("INFO3x1: move problem! %d\n", i);
					LOGGER_4("INFO3x1: %o\n", m[a]);
					close_log();
					abort();
				}
				MakeMoveNew(bs, mm[0], pos, &u);
				a++;
			}
			if(bs->uci_options->oldlen!= (a-2)) {
				bs->uci_options->newgame=1;
				LOGGER_1("INFO: UCI new game len %d vs %d\n", a-2, bs->uci_options->oldlen);
			}
			bs->uci_options->oldlen=a;
int f;
			if(bs->uci_options->newgame!=1) {
// check moves	
				for(f=0; f<(a-2);f++) {
					if(m[f]!=bs->uci_options->old[f]) {
						LOGGER_1("INFO: UCI new game moves\n");
						bs->uci_options->newgame=1;
						break;
					}
				}
			}
			for(f=0; f<a;f++) bs->uci_options->old[f]=m[f];
//			break;
		}
		tok = tokenizer(b2, " \n\r\t", &b2);
	}
return 0;
}

int ttest_def(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = -1;
timed2_def(i, 24, 100);
return 0;
}

int ttest_remis(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 10000;
timed2_remis(i, 24, 100);
return 0;
}

int ttest_def2(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 10000;
timed2Test("../tests/test_a.epd", i, 99, 9999);
return 0;
}

int ttest_iq(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 10000;
timed2Test_IQ("../tests/test_iq.epd", i,999, 9999);
return 0;}

int thash_def(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 90000;
timed2Test("../tests/test_hash.epd", i, 200, 1000);
return 0;
}

int tpawn_def(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 90000;
pawnEvalTest("../tests/test_pawn.epd", i);
return 0;
}

int thash_def_comp(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 90000;
timed2Test_comp("../tests/test_hash.epd", i, 200, 999);
return 0;
}

int tac_def_comp(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 90000;
timed2Test_comp("../tests/test_suite_bk.epd", i, 200, 999);
return 0;
}

int ttsts_def(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 1000;
timed2STS(i, 10000, 100, "pers.xml", NULL);
return 0;
}

int ttsts_spec(char *str)
{
int time, res, positions;
char pers2[256], *s;
// max time per test, num of tests in category, second personality
res = sscanf(str, "%d %d %s", &time, &positions, pers2);
if (res < 3)
	s = NULL;
else
	s = pers2;
if (res < 2)
	positions = 100;
if (res < 1)
	time = 200;

timed2STS(time, 10000, positions, "pers.xml", s);
return 0;
}

int ttsts_specn(char *str)
{
int time, res, positions;
char pers2[256], *s;
int mint;
// max time per test, num of tests in category, second personality
res = sscanf(str, "%d %d %d %s", &time, &positions, &mint, pers2);
if (res < 4) s = NULL;
else s = pers2;
if (res < 3) mint=0;
if (res < 2) positions = 1500;
if (res < 1) time = 1000;
printf ("%d %d\n", time, positions);
timed2STSn(time, 10000, positions, mint, "pers.xml", s);
return 0;
}

int ttsts_specnex(char *str)
{
int time, res, positions;
char pers2[256], *s;
char *tt[] = {"../tests/ests.epd"};
int tl[] = { 0, 1500, -1, -1 };

// max time per test, num of tests in category, second personality
res = sscanf(str, "%d %d %s", &time, &positions, pers2);
if (res < 3)
	s = NULL;
else
	s = pers2;
if (res < 2)
	positions = 1500;
if (res < 1)
	time = 1000;
printf ("%d %d\n", time, positions);
timed2STSex(tt, tl, time, 10000, positions, 0, "pers.xml", s);
return 0;
}

int ttest_wac(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 5000;
timed2Test("../tests/test_wac.epd", i, 90, 999);
return 0;
}

int ttest_wac2(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 60000;
timed2Test("../tests/test_a.epd", i, 90, 200);
return 0;
}

int ttest_null(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 60000;
timed2Test("../tests/test_suite_nullmove.epd", i, 90, 100);
return 0;
}

int ttest_swap_eval(char *str)
{
int i;
i = atoi(str);
if (i == 0)
	i = 4;
timed2Test_x("../texel/1-0.txt", 999, 90, i);
return 0;
}

/*
 * go options
 * searchmoves X Y Z, limit search to X Y Z moves only
 * ponder
 
 * wtime, btime X - side msec on clock
 * winc, binc X - increment in msec
 * movestogo X moves to next time control
 * 		without it, with winc, binc - sudden death
 * depth X, plies to search
 * nodes X, search X nodes
 * mate X, mate in X moves
 * movetime X, search exactly X msec
 * infinite
 */

int handle_go(board *bs, char *str)
{
int n, moves, time, inc, basetime, lag, cm;

//int movdiv[] = { 378, 149, 62, 34, 23, 18, 16, 15, 14, 13, 11, 10, 9, 7, 6 , 6, 5, 4, 1 };
//int movdiv[] =    { 84, 78, 70, 62, 54, 47, 41, 37, 32   , 28, 23, 18, 13, 9, 5, 4, 3, 3, 3, 3 };
int movdiv[] =    { 61, 54, 47, 40, 33, 27, 21, 15, 10   , 10, 10, 10, 10, 9, 5, 4, 3, 3, 3, 3 };

char *i[100];

if (engine_state != STOPPED) {
	LOGGER_4("UCI: INFO: Not stopped!, E:%d U:%d\n", engine_state, uci_state);
	engine_stop = 1;

	sleep_ms(10);
	while (engine_state != STOPPED) {
		LOGGER_4("UCI: INFO: Stopping!, E:%d U:%d\n", engine_state, uci_state);
		engine_stop = 1;
		sleep_ms(10);
	}
}
basetime = 0;

// ulozime si aktualni cas co nejdrive...
bs->run.time_start = readClock();

lag = 00;  //miliseconds
//	initialize ui go options

bs->uci_options->engine_verbose = 1;

bs->uci_options->binc = 0;
bs->uci_options->btime = 0;
bs->uci_options->depth = 999999;
bs->uci_options->infinite = 1;
bs->uci_options->mate = 0;
bs->uci_options->movestogo = 0;
bs->uci_options->movetime = 0;
bs->uci_options->ponder = 0;
bs->uci_options->winc = 0;
bs->uci_options->wtime = 0;
bs->uci_options->search_moves[0] = 0;

bs->uci_options->nodes = 0;

bs->run.time_move = 0;
bs->run.time_crit = 0;
moves=999;

// if option is not sent, such option should not affect/limit search

LOGGER_4("PARSEx: %s\n",str);
n = indexer(str, " \n\r\t", i);
LOGGER_4("PARSE: indexer %i\n",n);

if ((n = indexof(i, "wtime")) != -1) {
// this time is left on white clock
	bs->uci_options->wtime = atoi(i[n + 1]);
	bs->uci_options->infinite = 0;
	LOGGER_4("PARSE: wtime %s\n",i[n+1]);
}
if ((n = indexof(i, "btime")) != -1) {
	bs->uci_options->btime = atoi(i[n + 1]);
	bs->uci_options->infinite = 0;
	LOGGER_4("PARSE: btime %s\n",i[n+1]);
}
if ((n = indexof(i, "winc")) != -1) {
	bs->uci_options->winc = atoi(i[n + 1]);
	LOGGER_4("PARSE: winc %s\n",i[n+1]);
}
if ((n = indexof(i, "binc")) != -1) {
	bs->uci_options->binc = atoi(i[n + 1]);
	LOGGER_4("PARSE: binc %s\n",i[n+1]);
}
if ((n = indexof(i, "movestogo")) != -1) {
// this number of moves till next time control
	bs->uci_options->movestogo = atoi(i[n + 1]);
	LOGGER_4("PARSE: movestogo %s\n",i[n+1]);
}
if ((n = indexof(i, "depth")) != -1) {
// limit search do this depth
	bs->uci_options->depth = atoi(i[n + 1]);
	LOGGER_4("PARSE: depth %s\n",i[n+1]);
}
if ((n = indexof(i, "nodes")) != -1) {
// limit search to this number of nodes
	bs->uci_options->nodes = atoi(i[n + 1]);
	LOGGER_4("PARSE: nodes %s\n",i[n+1]);
}
if ((n = indexof(i, "mate")) != -1) {
// search for mate this deep
	bs->uci_options->mate = atoi(i[n + 1]);
	LOGGER_4("PARSE: mate %s\n",i[n+1]);
}
if ((n = indexof(i, "movetime")) != -1) {
// search exactly for this long
	bs->uci_options->movetime = atoi(i[n + 1]);
	bs->uci_options->infinite = 0;
	LOGGER_4("PARSE: movetime %s\n",i[n+1]);
}
if ((n = indexof(i, "infinite")) != -1) {
// search forever
	bs->uci_options->infinite = 1;
	bs->run.time_crit = 0;
	LOGGER_4("PARSE: infinite\n");
}
if ((n = indexof(i, "ponder")) != -1) {
	bs->uci_options->ponder = 1;
	LOGGER_4("PARSE: ponder\n");
}
if ((n = indexof(i, "searchmoves")) != -1) {
	LOGGER_4("PARSE: searchmoves %s IGNORED",i[n+1]);
}

// pred spustenim vypoctu jeste nastavime limity casu
if (bs->uci_options->infinite == 1) {
// infinite
	bs->run.time_crit = 0;
} else if (bs->uci_options->movetime != 0) {
// exact movetime
	bs->run.time_move = bs->uci_options->movetime - lag;
	bs->run.time_crit = bs->uci_options->movetime - lag;
} else {
// variable move time
	if (bs->uci_options->movestogo == 0) {
// sudden death

/* best - pro 2+s 
#define SX1 0.0
#define SY1 80.0
#define xSX2 40.0, 
#define SX2 80.0
#define SY2 10.0
 
 * alternativa - pro 1s ok, pro 2s uz staci 
#define SX1 0.0
#define SY1 80.0
#define SX2 100.0
#define SY2 10.0

 */

#define SX1 0.0
#define SY1 80.0
#define SX2 68.0
#define SY2 10.0

#define SA ((SY2-SY1)/(SX2-SX1))
#define SB (SY1-SA*SX1)

#if 1
		if (bs->move >= SX2)
			moves = (int) SY2;
		else
			moves = (int) (SA * bs->move + SB);
		LOGGER_1("time moves %f, %f, %d, %d\n", SA, SB, moves,
			bs->move);
#endif
#if 0
		if (bs->move > 180)
			moves = 2;
		else
			moves = movdiv[bs->move/10] ;
			
//		if((bs->move>=0)&&(bs->move<=40)) moves-=20; else moves=10;
//		if(moves<=1) moves=2;
#endif
#if 0
		if(bs->move >= 140 ) moves=20;
		else moves= 48 - bs->move / 5;
#endif

	} else {
		moves = bs->uci_options->movestogo;
	}
	if ((bs->side == 0)) {
		time = bs->uci_options->wtime;
		inc = bs->uci_options->winc;
//		cm= (bs->uci_options->wtime-bs->uci_options->btime)*100;
		cm= (bs->uci_options->wtime*100/bs->uci_options->btime);
	} else {
		time = bs->uci_options->btime;
		inc = bs->uci_options->binc;
//		cm= (bs->uci_options->btime-bs->uci_options->wtime)*100;
		cm= (bs->uci_options->btime*100/bs->uci_options->wtime);
	}
	// average movetime
	basetime = ((time - inc) / moves + inc - lag);
//	basetime = (time / moves - lag);
//	basetime *= 100;
//	basetime /= 100;
//	if(cm<=90) {
//		basetime *= (cm-5);
//		basetime /= 100;
//	}

	if (basetime < 0)
		basetime = 0;
	if (basetime > time)
		basetime = time;
	bs->run.time_move = basetime;
	if (moves == 1)
		bs->run.time_crit = Min(5 * basetime, time - lag);
	else
		bs->run.time_crit = Min(5 * basetime, time / 2 - lag);
}
// pres time_crit nejede vlak a okamzite konec
// time_move je cil kam bychom meli idealne mirit a nemel by byt prekrocen pokud neni program v problemech
// time_move - target time

DEB_2(printBoardNice(bs);)
	LOGGER_1(
	"TIME: wtime: %llu, btime: %llu, time_crit %llu, time_move %llu, basetime %llu, side %c, moves %d, bsmoves %d\n",
	bs->uci_options->wtime, bs->uci_options->btime, bs->run.time_crit,
	bs->run.time_move, basetime, (bs->side == 0) ? 'W' : 'B', moves, bs->move);
 
	bs->move_ply_start = bs->move;
	bs->pers->start_depth = 1;
	uci_state = 4;
	engine_state = START_THINKING;

	LOGGER_4("UCI: go activated\n");
	sleep_ms(10);

return 0;
}

int handle_stop()
{
LOGGER_4("UCI: INFO: STOP has been received from UI\n");
while (engine_state != STOPPED) {
	LOGGER_4("UCI: INFO: running, E:%d U:%d\n", engine_state, uci_state);
	engine_stop = 1;
	sleep_ms(10);
} LOGGER_4("UCI: INFO: stopped, E:%d U:%d", engine_state, uci_state);
return 0;
}

int start_threads(board *b)
{
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
pthread_create(&(b->run.engine_thread), &attr, engine_thread, (void*) b);
pthread_attr_destroy(&attr);
return 0;
}

board* allocate_board()
{
board *b;
b = malloc(sizeof(board) * 1);
 
b->stats = allocate_stats(1);
clearALLSearchCnt(STATS);
b->uci_options = malloc(sizeof(struct _ui_opt));
strncpy(b->uci_options->oldfen, "XXXXX", 99);

b->hht = allocateHHTable();
b->kmove = allocateKillerStore();
engine_state = STOPPED;
return b;
}

int allocate_tables(board *b)
{
b->hs = NULL;
b->hps = NULL;
if (b->pers->use_ttable != 0) {
	b->hs = allocateHashStore(HASHSIZE * 1024L * 1024L, 2048);
	b->hps = allocateHashPawnStore(HASHPAWNSIZE * 1024L * 1024L);
}
return 0;
}

int stop_threads(board *b)
{
void *status;
engine_state = MAKE_QUIT;
sleep_ms(10);
pthread_join(b->run.engine_thread, &status);
DEB_1(printALLSearchCnt(STATS);)

return 0;
}

int deallocate_tables(board *b)
{
if (b->hps != NULL)
	freeHashPawnStore(b->hps);
if (b->hs != NULL)
	freeHashStore(b->hs);
return 0;
}

int deallocate_board(board *b)
{
freeHHTable(b->hht);
freeKillerStore(b->kmove);
deallocate_stats(b->stats);
free(b->uci_options);
free(b);
return 0;
}

int uci_loop(int second)
{
char *buff, *tok, *b2;

int16_t move_o;
size_t inp_len;
int bytes_read;
int position_setup = 0;
board *b;

buff = (char*) malloc(INPUT_BUFFER_SIZE + 1);
inp_len = INPUT_BUFFER_SIZE;

LOGGER_4("INFO: UCI started\n");

b = allocate_board();

/*
 *setup personality
 */

if (second) {
	b->pers = (personality*) init_personality("pers2.xml");
} else {
	b->pers = (personality*) init_personality("pers.xml");
}

allocate_tables(b);
start_threads(b);
uci_state = 1;

move_o = -1;
/*
 * parse and dispatch UCI protocol/commands
 * uci_states
 * 0 - quit
 * 1 - non uci/waiting for uci
 * 2 - uci handled, idle
 * 3 - computing
 */

while (uci_state != 0) {

	/*
	 * wait & get line from standard input
	 */
	bytes_read = (int) getline(&buff, (&inp_len), stdin);
	if (bytes_read == -1) {
		LOGGER_1("INFO: input read error!\n");
		break;
	} else {
		reentry: tok = tokenizer(buff, " \n\r\t", &b2);
		while (tok) {
			LOGGER_4("PARSE: %d %s\n",uci_state,tok);
			if (!strcasecmp(tok, "quit")) {
				handle_stop();
				uci_state = 0;
				engine_stop = 1;
				break;
			} else if (!strcasecmp(tok, "isready")) {
				tell_to_engine("readyok\n");
				break;
			}
			if (!strcmp(tok, "uci")) {
				handle_uci();
				uci_state = 2;
				break;
			}
			if (!strcmp(tok, "perft")) {
				perft2_def(1, 7, 0);
				break;
			}
			if (!strcmp(tok, "perft10")) {
				perft2("../tests/test_perft.epd", 1, 8, 0);
			}
			if (!strcmp(tok, "perft11")) {
				perft2("../tests/test_perft.epd", 1, 8, 2);
			}
			if (!strcmp(tok, "perft1x")) {
				perft2x("../tests/test_perft.epd", 1, 8, 0, 10);
				perft2x("../tests/test_perft.epd", 1, 8, 2, 10);
			}
			if (!strcmp(tok, "perft12")) {
				perft2("../tests/test_perft.epd", 1, 7, 4);
			}
			if (!strcmp(tok, "perft20")) {
				perft2("../tests/test_perft.epd", 1, 7, 1);
			}
			if (!strcmp(tok, "perft21")) {
				perft2("../tests/test_perft.epd", 1, 7, 3);
			}
			if (!strcmp(tok, "perft22")) {
				perft2("../tests/test_perft.epd", 1, 7, 5);
			}
			if (!strcmp(tok, "perft30")) {
				perft2("../tests/test_perftsuite.epd", 1, 8, 0);
			}
			if (!strcmp(tok, "perft31")) {
				perft2("../tests/test_perftsuite.epd", 1, 8, 2);
			}
			if (!strcmp(tok, "perft3x")) {
				perft2x("../tests/test_perftsuite.epd", 1, 8, 0,
					10);
				perft2x("../tests/test_perftsuite.epd", 1, 8, 2,
					10);
			}
			if (!strcmp(tok, "perft32")) {
				perft2("../tests/test_perftsuite.epd", 1, 8, 4);
			}
			if (!strcmp(tok, "testsee")) {
				see_test();
			}
#ifdef TUNING
			if (!strcmp(tok, "ttev")) {
				eval_checker("../tests/test_ev3.epd", 1000);
			}
			if (!strcmp(tok, "ttevv")) {
				eval_checker3("../tests/STS1-STS15_LAN_v6.epd", 1000);
			}
			if (!strcmp(tok, "ttev3")) {
				eval_checker3("../tests/test_ev3.epd", 1000);
			}
#endif
			if (!strcmp(tok, "ttev2")) {
				eval_checker2("../tests/STS1-STS15_LAN_v6.epd", 1000);
			}
			
			if (!strcmp(tok, "ttqui")) {
				eval_qui_checker("../tests/data.epd","../tests/data_s.epd", -1);
			}
			if (!strcmp(tok, "ttew")) {
				eval_checker2("../tests/test_ev3.epd", 1000);
			}
			if (!strcmp(tok, "tte1")) {
				eval_checker2("../tests/annon_b.epd", 1000);
			}
			if (!strcmp(tok, "tte2")) {
				eval_checker2("../tests/annotated.epd", 1000);
			}
			if (!strcmp(tok, "tta1")) {
				analyzer_1("../tests/annon_b.epd", 999999, 1, 1000, 0, 0, "../tests/annon_b_out.epd");
			}
			if (!strcmp(tok, "tta2")) {
				analyzer_1("../tests/annotated.epd", 999999, 4, 10000, 0, 0, "../tests/annotated_out.epd");
			}
			if (!strcmp(tok, "testee")) {
				char *pp[] = { "pers.xml", "pers1.xml",
					"pers17.xml" };
				char *tt[] = { "../texel/lichess-quiet.txt",
					"../texel/quiet-labeled.epd",
					"../texel/e12_33.epd",
					"../texel/e12_41.epd",
					"../texel/e12_52.epd" };
				EvalCompare(pp, 2, tt, 1, 5000);
			}
			if (!strcmp(tok, "testsee0")) {
				see0_test();
			}
			if (!strcmp(tok, "testmovesort")) {
				movegensort_test("r1n3k1/1P6/n7/1B3pP1/1p1R2q1/3p1N2/7P/R3K3 w Q f6 0 1", "pers.xml");
			}
			if (!strcmp(tok, "ttswap")) {
				ttest_swap_eval(b2);
			}
			if (!strcmp(tok, "ttdef")) {
				ttest_def(b2);
				break;
			}
			if (!strcmp(tok, "ttremis")) {
				ttest_remis(b2);
				break;
			}
			if (!strcmp(tok, "ttfile")) {
				ttest_def2(b2);
				break;
			}
			if (!strcmp(tok, "ttiq")) {
				ttest_iq(b2);
				break;
			}
			if (!strcmp(tok, "wac")) {
				ttest_wac(b2);
				break;
			}
			if (!strcmp(tok, "wac2")) {
				ttest_wac2(b2);
				break;
			}
			if (!strcmp(tok, "ttnull")) {
				ttest_null(b2);
				break;
			}
			if (!strcmp(tok, "tthash")) {
				thash_def(b2);
				break;
			}
			if (!strcmp(tok, "ttpawn")) {
				tpawn_def(b2);
				break;
			}
			if (!strcmp(tok, "tthashc")) {
				thash_def_comp(b2);
				break;
			}
			if (!strcmp(tok, "ttsts")) {
				ttsts_def(b2);
				break;
			}
			if (!strcmp(tok, "ttst2")) {
				ttsts_spec(b2);
				break;
			}
			if (!strcmp(tok, "ttstn")) {
				ttsts_specn(b2);
				break;
			}
			if (!strcmp(tok, "ttstn")) {
				ttsts_specn(b2);
				break;
			}
			if (!strcmp(tok, "ttste")) {
				ttsts_specnex(b2);
				break;
			}
			if (!strcmp(tok, "tttt")) {
				timed2STSn(100, 10000, 1500, 0, "pers.xml",
					"pers2.xml");
				break;
			}
#ifdef TUNING
			if (!strcmp(tok, "texel")) {
				texel_test(b2);
				break;
			}
#endif
			if (!strcmp(tok, "ttfill")) {
				fill_test();
				break;
			}
			if (!strcmp(tok, "wpers")) {
				write_personality(b->pers, "pers_test.xml");
				break;
			}
			if (!strcmp(tok, "mtst")) {
				strcpy(buff,
					"position startpos moves d2d3 d7d5 c1f4");
				uci_state = 2;
				goto reentry;
			} else if (!strcasecmp(tok, "my")) {
				strcpy(buff,
					"position startpos moves e2e4 c7c5 g1f3 b8c6 f1b5 e7e6 e1g1 g8e7 f1e1 a7a6 b5c6 e7c6 b2b3 f8e7 c1b2 e8g8 d2d4 c5d4 f3d4 d7d6 b1d2 c8d7 d4c6 d7c6 e1e3 e7g5 d1g4 g5h6 e3h3 d8g5 g4g5 h6g5 h3d3 g5d2 d3d2 c6e4 d2d6 f8d8 a1d1 d8d6 d1d6 e4c2 d6d7 c2e4 a2a4 a8c8 a4a5 e4c6 d7d1 c6d5 d1d3 c8c2 d3c3 c2c3 b2c3 d5b3 g1f1 b3d5 g2g3 f7f6 f1e1 g8f7 e1d2 e6e5 c3b4 e5e4 d2c3 f7e6 b4c5 e6f5 c3d4 f5e6 d4c3 g7g6 c3c2 d5c4 c2c3 c4d3 c3b3 e6d5 c5b6 d5e6 b6e3 g6g5 e3d4 g5g4 d4e3 d3e2 b3c3 e2b5 c3b2 b5c6 b2b3 c6d5 b3a3 e6f7 a3b2 f7e6 e3c5 e6f5 c5b6 f5e5 b6e3 e5e6 e3c5 e6e5 c5e3 e5f5 e3b6 d5c4 b2c3 c4f7 b6e3 f7a2 e3d4 a2d5 d4c5 f5e5 c5e3 e5e6 e3d4 e6f5 d4c5 f5e5 c5e3 e5e6 e3d4 d5c6 d4b6 c6b5 b6e3 e6d5 c3b3 b5c4 b3b2 c4b5 b2c2 b5d3 c2b3 d3b5 b3a3 b5f1 e3d2 d5c4 a3b2 c4d5 d2c3 d5e6 c3b4 e6e5 b4c3 e5e6 c3b4 e6e5 b2b3 f1d3 b4c3 e5e6 c3d4 d3f1 d4e3 f1b5 b3b2 e6e5 e3f4 e5f5 f4c7 b5c6 b2c3 f5e6 c3d4 e6f5 d4c3 f5e6 c3d4 e4e3 f2e3 e6f5 c7d8 f5e6 d8c7 e6f5 c7d6 c6f3 d6b4 f5e6 b4c5 f3d5 e3e4 d5c6 c5a7 c6a4 a7c5 a4e8 d4e3 e8a4 e3d4 a4e8 d4e3 e8g6 e3d4 g6f7 c5a3 f7e8 d4e3 e8a4 a3b2 a4d1 b2d4 d1c2 e3f4 c2d3 f4g4 d3e4 g4f4 f6f5 f4e3 e6d6 e3f4 d6e6 f4e3 e4d5 d4c5 d5c6 c5a7 c6b5 a7c5 b5c6 c5a7 c6b5 a7c5 b5c4 c5b4 e6e5 b4c3 e5e6 c3b2 c4b3 b2a3 e6e5 a3b4 b3f7 b4c3 e5e6 e3d4 f7e8 d4c5 e8c6 h2h4 c6f3 c3d2 e6e7 d2g5 e7e6 c5d4 f3d1 g5h6 d1c2 h6g5 c2d1 g5h6 d1c2 h6e3 h7h5 e3c1 c2e4 c1a3 e4f3 a3b4 f3c6 b4d2 c6g2 d4c4 g2c6 d2f4 c6d5 c4c5 d5g2 f4d2 g2h1 d2c3 h1d5 c3b2 d5e4 c5d4 e6f6 d4c4 f6e6 c4d4 e6f6 b2c3 e4h1 c3b4 f6e6 b4f8 h1f3 f8b4 f3g2 b4c5 g2c6 c5b6 c6f3 d4c4 f3g2");
				uci_state = 2;
				goto reentry;
			} else if (!strcasecmp(tok, "mytst")) {
				strcpy(buff,
					"position fen 5k2/ppp2r1p/2p2ppP/8/2Q5/2P1bN2/PP4P1/1K1R4 w - - 0 1");
				uci_state = 2;
				LOGGER_3("setup mytst");
				goto reentry;
			} else if (!strcasecmp(tok, "myts2")) {
				strcpy(buff,
					"position fen rnbq1rk/1p3pp/p4b1p/3p3n/8/1NP2N2/PPQ2PPP/R1B1KBR b Q - 6 13");
				uci_state = 2;
				LOGGER_3("setup myts2");
				goto reentry;
			} else if (!strcasecmp(tok, "myts3")) {
				strcpy(buff,
					"position fen 5k2/ppp2r1p/2p2ppP/8/2Q5/2P1bN2/PP4P1/1K1R4 w - - 0 1 ");
				uci_state = 2;
				LOGGER_3("setup myts3");
				goto reentry;
			} else if (!strcasecmp(tok, "myts4")) {
				strcpy(buff,
					"position fen 6rk/1p1R3p/2n3p1/p7/8/P3Q3/1qP2PPP/4R1K1 w - - 0 1");
				uci_state = 2;
				LOGGER_3("setup myts4");
				goto reentry;
			} else if (!strcasecmp(tok, "myts5")) {
				strcpy(buff,
					"position fen k7/b7/b7/8/8/8/7Q/7K w - - 0 1");
				uci_state = 2;
				LOGGER_3("setup myts5");
				goto reentry;
			} else if (!strcasecmp(tok, "myts7")) {
				strcpy(buff,
					"position fen 5rkr/pppb2pp/2nb4/3p3B/4p2q/8/PPPPPPPP/RNBQK1NR w KQ - 0 1");
				uci_state = 2;
				LOGGER_3("setup myts7");
				goto reentry;
			} else if (!strcasecmp(tok, "mytsp")) {
				strcpy(buff,
					"position fen 2r2r2/p2qppkp/3p2p1/3P1P2/2n2R2/7R/P5PP/1B1Q2K1 w - -");
				uci_state = 2;
				printf("go depth 8 should give you 9645 or less nodes (Arasan) with proper pruning. SF17 has 4.9kN\n");
				LOGGER_3("setup mytsp");
				goto reentry;
			} else if (!strcasecmp(tok, "mytss")) {
				strcpy(buff,
					"position fen 5rk/bb3p1p/1p1p1qp/pBpP4/N1Pp2P/P2Q3P/1P3PK/3R4 w - c6 1 31 moves d5c6");
				uci_state = 2;
				LOGGER_3("setup mytss");
				goto reentry;
			}
			if (!strcasecmp(tok, "ucinewgame")) {
				LOGGER_1("INFO: UCI game new\n");
				L0("** Running TOTALS **\n");
				printSearchStat(&(STATS[MAXPLY]));
				handle_newgame(b);
				b->uci_options->newgame=1;
				position_setup = 1;
				break;
			} else if (!strcasecmp(tok, "position")) {
				handle_position(b, b2);
				position_setup = 1;
// check position
// detekce jestli je to pokracovani stejne hry nebo ne
				break;
			} else if (!strcasecmp(tok, "go")) {
				if (!position_setup) {
					handle_newgame(b);
					b->uci_options->newgame=1;
					position_setup = 1;
				}
				if ((b->pers->ttable_clearing >= 1)) {
//					invalidateHash(b->hs);
					invalidatePawnHash(b->hps);
//					b->uci_options->newgame=1;
					LOGGER_1("INFO: UCI hash reset\n");
				} LOGGER_4("INFO: UCI hash reset DONE\n");
				move_o = b->move;
				invalidateHash(b->hs);
				if(b->uci_options->newgame==1) {
					LOGGER_1("INFO: UCI new game detected\n");
					if(b->pers->ttable_clearing < 1) invalidatePawnHash(b->hps);
//					clearHHTable(b->hht);
					clearHHTable2(b->hht,b->pers->piecetosquare);
				} else {
					LOGGER_1("INFO: UCI game cont\n");
					reduceHHTable(b->hht);
//					clearHHTable(b->hht);
				}
				handle_go(b, b2);
				break;
			} else if (!strcasecmp(tok, "gox")) {
				strcpy(buff, "go movetime 1000");
				goto reentry;
			}
			if (!strcasecmp(tok, "stop")) {
				handle_stop();
				uci_state = 2;
				break;
			} else {
			}
			tok = tokenizer(b2, " \n\r\t", &b2);
		}
	}
} LOGGER_4("INFO: exiting...\n");
stop_threads(b);
deallocate_tables(b);
free(b->pers);
deallocate_board(b);
LOGGER_2("INFO: UCI stopped\n");
free(buff);
return 0;
}

int uci_loop2(int second)
{
return 0;
}
