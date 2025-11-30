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

#include "utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include "bitmap.h"
#include "evaluate.h"
#include "movgen.h"
#include "generate.h"
#include "ui.h"
#include <iconv.h>

#include "globals.h"
#include "inlines.h"

FILE *debugf;

int logger2(char *fmt, ...)
{
	char buf[512];
	int n;
	int hh, mm, ss, nn;
	unsigned long long en;
	va_list ap;

	en = readClock();
	nn = (int) (en % 1000);
	en = en / 1000;
	ss = (int) (en % 60);
	en = en / 60;
	mm = (int) (en % 60);
	en = en / 60;
	hh = (int) (en % 24);

	va_start(ap, fmt);
	vsnprintf(buf, 512, fmt, ap);
	va_end(ap);
	fprintf(debugf, "%02d:%02d:%02d:%04d  %s", hh, mm, ss, nn, buf);
	return 0;
}

int nlogger2(char *fmt, ...)
{
	char buf[512];
	int n;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, 512, fmt, ap);
	va_end(ap);
	fprintf(debugf, "%s", buf);
	return 0;
}

int logger(char *p, char *s, char *a)
{
	int hh, mm, ss, nn;
	unsigned long long en;
	en = readClock();

	nn = (int) (en % 1000);

	en = en / 1000;
	ss = (int) (en % 60);

	en = en / 60;
	mm = (int) (en % 60);

	en = en / 60;
	hh = (int) (en % 24);

	fprintf(debugf, "%02d:%02d:%02d:%04d  %s%s%s", hh, mm, ss, nn, p, s, a);
	return 0;
}

int open_log(char *filename)
{
	debugf = fopen(filename, "w+");
	setvbuf(debugf, NULL, _IONBF, 16384);
	return 0;
}

int close_log(void)
{
	fclose(debugf);
	return 0;
}

int flush_log(void)
{
	fflush(debugf);
	return 0;
}

char* tokenizer(char *str, char *delim, char **next)
{
	char *s, *t;
	size_t i;
	s = NULL;
	if (str != NULL) {
// preskocit zacatek stringu s delimitory
		i = strspn(str, delim);
// we have start of token
		s = str + i;
		if (isprint(*s)) {
// najit dalsi delimitory nebo konec, co bude driv
			t = strpbrk(s, delim);
			if (t != NULL) {
				*t = 0x0;
				t++;
				*next = t;
			} else
				*next = NULL;
			LOGGER_4("=>%s<=\n",s);
		} else
			s = NULL;
	}
	return s;
}

int indexer(char *str, char *delim, char **index)
{
	char *tok, *b2;
	int i;

	i = 0;
	index[i] = NULL;
	tok = tokenizer(str, delim, &b2);
	while (tok) {
		index[i] = tok;
		i++;
		index[i] = NULL;
		tok = tokenizer(b2, delim, &b2);
	}
	return i;
}

int indexof(char **index, char *str)
{
	int i;
	i = 0;
	while (index[i] != NULL) {
		if (!strcmp(str, index[i])) {
			return i;
		}
		i++;
	}
	return -1;
}

// clock in miliseconds
unsigned long long int readClock(void)
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);

	return ((unsigned long long) t.tv_sec * 1000
		+ (unsigned long long) t.tv_nsec / 1000000);
}

// returns diff in times in microseconds
unsigned long long diffClock(struct timespec start, struct timespec end)
{
	struct timespec temp;
	temp.tv_sec = end.tv_sec - start.tv_sec;
	if (end.tv_nsec < start.tv_nsec) {
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
		temp.tv_sec--;
	} else
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	return (unsigned long long) temp.tv_sec * 1000000
		+ (unsigned long long) temp.tv_nsec / 1000;
}

int readClock_wall(struct timespec *t)
{
	clock_gettime(CLOCK_MONOTONIC, t);
	return 0;
}

int readClock_proc(struct timespec *t)
{
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, t);
	return 0;
}

int generate_log_name(char *n, char *pref, char *b)
{

	time_t rawtime;
	struct tm *info;
	struct timespec ti;
	char buffer[80];
	int r;
	pid_t pp;

	time(&rawtime);
	info = localtime(&rawtime);
	readClock_wall(&ti);
	r = (int) ((ti.tv_nsec) / 100) % 0x8000;
	pp = getpid();
	strftime(buffer, 80, "%y%m%d-%H%M%S", info);
	sprintf(b, "%s-%s%s-%X-%ld.txt", n, buffer, pref, r, (long) pp);
	return 0;
}

int parse_cmd_line_check_sec(int argc, char *argv[])
{
	int i;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-second"))
			return 1;
	}
	return 0;
}

int computeMATIdx(board *b)
{
	int idx;
	int bwl, bwd, bbl, bbd, pw, pb, nw, nb, rw, rb, qw, qb;

	bwd = BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE]) & BLACKBITMAP);
	bbd = BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK]) & BLACKBITMAP);
	bwl = BitCount((b->maps[BISHOP]) & (b->colormaps[WHITE]) & WHITEBITMAP);
	bbl = BitCount((b->maps[BISHOP]) & (b->colormaps[BLACK]) & WHITEBITMAP);
	pw = BitCount((b->maps[PAWN]) & (b->colormaps[WHITE]));
	pb = BitCount((b->maps[PAWN]) & (b->colormaps[BLACK]));
	nw = BitCount((b->maps[KNIGHT]) & (b->colormaps[WHITE]));
	nb = BitCount((b->maps[KNIGHT]) & (b->colormaps[BLACK]));
	rw = BitCount((b->maps[ROOK]) & (b->colormaps[WHITE]));
	rb = BitCount((b->maps[ROOK]) & (b->colormaps[BLACK]));
	qw = BitCount((b->maps[QUEEN]) & (b->colormaps[WHITE]));
	qb = BitCount((b->maps[QUEEN]) & (b->colormaps[BLACK]));
	idx = MATidx(pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb);

	return idx;
}

// UTF-8 character is stored as variable length sequence of bytes
// wchar_t is bigger, but it depends on encoding whether it is variable-length. Lets assume that it is fixed size

int UTF8toWchar(unsigned char *in, wchar_t *out, size_t oll)
{
	iconv_t cd;
	size_t il;
	size_t ol;
	size_t ret;
	char *ooo;
	ooo = (char*) out;
	cd = iconv_open("WCHAR_T", "UTF-8");
	if ((iconv_t) - 1 == cd) {
		return -1;
	}
	ret = iconv(cd, NULL, NULL, NULL, NULL);
	ol = oll - sizeof(wchar_t);
	il = strlen((const char*) in) + 1;

	ret = iconv(cd, (char**) &in, &il, &ooo, &ol);
	iconv_close(cd);
	*ooo = L'\0';
	if ((size_t) - 1 == ret) {
		return -2;
	}
	return 0;
}

int WchartoUTF8(wchar_t *in, unsigned char *out, size_t oll)
{
	iconv_t cd;
	size_t il;
	size_t ol;
	size_t ret;
	char *ooo;
	ooo = (char*) out;
	cd = iconv_open("UTF-8", "WCHAR_T");
	if ((iconv_t) - 1 == cd) {
		perror("iconv_open");
		return -1;
	}
	il = wcslen(in) * sizeof(wchar_t);
	ol = oll - 1;

	ret = iconv(cd, (char**) &in, &il, &ooo, &ol);
	iconv_close(cd);
	*ooo = '\0';
	if ((size_t) - 1 == ret) {
		perror("iconv");
		return -2;
	}
	return 0;
}
void log_divider(char *s)
{
	if (s != NULL) {
		LOGGER_1("****: %s\n",s);
	} else {
		LOGGER_1("****:\n");
	}
}

void dump_moves(board *b, move_cont *mc, int count, int ply, char *cmt)
{
	char b2[2048];
	int i;
	move_entry *m;
	m= mc->move;

	LOGGER_0("MOV_DUMP: * Start *\n");
	if (cmt != NULL)
		LOGGER_0("MOV_DUMP: Comments %s\n", cmt);
	i=0;
	while(m<mc->lastp) {
		sprintfMove(b, m->move, b2);
		LOGGER_0("%*d, MVD , %d: %s, iord: %d, rscore: %d, ford %d, phase: %d, state: %d, nodes: %lld\n", 2 + ply, ply, i, b2,
			m->qorder, m->real_score, m->ord, m->phase, m->state, m->nodes);
		m++;
		i++;
	}
	m=mc->bad;
	while(m<mc->badp) {
		sprintfMove(b, m->move, b2);
		LOGGER_0("%*d, BAD , %d: %s, iord: %d, rscore: %d, ford %d, phase: %d, state: %d, nodes: %lld\n", 2 + ply, ply, i, b2,
			m->qorder, m->real_score, m->ord, m->phase, m->state, m->nodes);
		m++;
		i++;
	}
	LOGGER_0("MOV_DUMP: ** END **\n");
}

int compareBoardSilent(board *source, board *dest)
{
	int i, ret;

	ret = 0;
	if (dest->key != source->key) {
		ret = 1;
		goto konec;
	}
	if (dest->norm != source->norm) {
		ret = 2;
		goto konec;
	}
	if (dest->r45L != source->r45L) {
		ret = 1;
		goto konec;
	}
	if (dest->r45R != source->r45R) {
		ret = 4;
		goto konec;
	}
	if (dest->r90R != source->r90R) {
		ret = 5;
		goto konec;
	}
	if (dest->rule50move != source->rule50move) {
		ret = 6;
		goto konec;
	}
	if (dest->side != source->side) {
		ret = 7;
		goto konec;
	}
	if (dest->ep != source->ep) {
		ret = 8;
		goto konec;
	}
	for (i = WHITE; i < ER_SIDE; i++)
		if (dest->castle[i] != source->castle[i]) {
			ret = 9;
			goto konec;
		}
	for (i = 0; i < 6; i++)
		if (dest->maps[i] != source->maps[i]) {
			ret = 10;
			goto konec;
		}
	for (i = 0; i < 2; i++)
		if (dest->colormaps[i] != source->colormaps[i]) {
			ret = 11;
			goto konec;
		}
	for (i = 0; i < 64; i++)
		if (dest->pieces[i] != source->pieces[i]) {
			ret = 14;
			goto konec;
		}
	if (dest->mindex != source->mindex) {
		ret = 15;
		goto konec;
	}

	konec: if (ret != 0) {
		printf("XXX");
	}
	return ret;
}

int copyStats(struct _statistics *source, struct _statistics *dest)
{
	*dest = *source;
	return 0;
}

int copyBoard(board *source, board *dest)
{
	memcpy(dest, source, sizeof(board));
	copyStats(source->stats, dest->stats);
	dest->pers = source->pers;
	return 0;
}

int triggerBoard()
{
	printf("trigger!\n");
	return 0;
}

#ifndef TUNING
int compareBoard(board *source, board *dest)
{
	int i;
	char a1[100];
	char a2[100];
	char a3[100];
	int r;

	if (dest->ep != source->ep) {
		printf("EP!\n");
		triggerBoard();
	}
	if (dest->gamestage != source->gamestage) {
		printf("GAMESTAGE!\n");
		triggerBoard();
	}
	if (dest->key != source->key) {
		printf("KEY!\n");
		triggerBoard();
	}
	if (dest->move != source->move) {
		printf("MOVE!\n");
		triggerBoard();
	}
	if (dest->norm != source->norm) {
		printf("NORM!\n");
		triggerBoard();
	}
	if (dest->r45L != source->r45L) {
		printf("r45L!\n");
		triggerBoard();
	}
	if (dest->r45R != source->r45R) {
		printf("r45R!\n");
		triggerBoard();
	}
	if (dest->r90R != source->r90R) {
		printf("r90R!\n");
		triggerBoard();
	}
	if (dest->rule50move != source->rule50move) {
		printf("rule50move!\n");
		triggerBoard();
	}
	if (dest->side != source->side) {
		printf("side!\n");
		triggerBoard();
	}

	for (i = WHITE; i < ER_SIDE; i++)
		if (dest->castle[i] != source->castle[i]) {
			printf("CASTLE %d!\n", i);
			triggerBoard();
		}
	for (i = 0; i < 2; i++)
		if (dest->colormaps[i] != source->colormaps[i]) {
			printf("COLORMAPS %d!\n", i);
			triggerBoard();
		}
	for (i = 0; i < 2; i++)
		if (dest->king[i] != source->king[i]) {
			printf("KING %d!\n", i);
			triggerBoard();
		}
	for (i = 0; i < 6; i++)
		if (dest->maps[i] != source->maps[i]) {
			printf("MAPS %d!\n", i);
			triggerBoard();
		}
	for (i = 0; i < 64; i++)
		if (dest->pieces[i] != source->pieces[i]) {
			printf("PIECES %d!\n", i);
			triggerBoard();
		}
	for (i = 0; i < MAXPLY; i++)
		if (dest->positions[i] != source->positions[i]) {
			printf("POSITIONS %d!\n", i);
			triggerBoard();
		}
	for (i = 0; i < MAXPLY; i++)
		if (dest->posnorm[i] != source->posnorm[i]) {
			printf("POSNORM %d!\n", i);
			triggerBoard();
		}
	if (dest->mindex != source->mindex) {
		printf("MIndex!\n");
		r = source->mindex - dest->mindex;
		outbinary((BITVAR) source->mindex, a1);
		outbinary((BITVAR) dest->mindex, a2);
		outbinary((BITVAR) r, a3);
		printf("s: %s\nd: %s\nr: %s\n", a1, a2, a3);
		triggerBoard();
	}
	return 0;
}
#else
int compareBoard(board *source, board *dest) { return 0; }
#endif

void printboard(board *b)
{
	printmask(b->norm, "Normal");
	printmask(b->maps[KING], "Kings");
	printmask(b->maps[QUEEN], "Queens");
	printmask(b->maps[BISHOP], "Bishops");
	printmask(b->maps[KNIGHT], "Knights");
	printmask(b->maps[ROOK], "Rooks");
	printmask(b->maps[PAWN], "Pawns");
	printmask(b->colormaps[WHITE], "White");
	printmask(b->colormaps[BLACK], "Black");
}

void printBoardNice(board const *b)
{
	int f, n;
	int pw, pb, nw, nb, bwl, bwd, bbl, bbd, rw, rb, qw, qb;
	char buff[1024];
	char x, ep[3];
	char row[8];
	if (b->ep != 0) {
		sprintf(ep, "%c%c", b->ep % 8 + 'A', b->ep / 8 + '1');
	} else
		ep[0] = '\0';
	LOGGER_0(
		"Move %d, Side to Move %s, e.p. %s, CastleW:%i B:%i, HashKey 0x%016llX, MIdx:%d\n",
		b->move / 2, (b->side == 0) ? "White" : "Black", ep,
		b->castle[WHITE], b->castle[BLACK],
		(unsigned long long ) b->key, b->mindex);
	x = ' ';
	for (f = 7; f >= 0; f--) {
		for (n = 0; n < 8; n++) {
			switch (b->pieces[f * 8 + n]) {
			case ER_PIECE:
				x = ' ';
				break;
			case BISHOP:
				x = 'B';
				break;
			case KNIGHT:
				x = 'N';
				break;
			case PAWN:
				x = 'P';
				break;
			case QUEEN:
				x = 'Q';
				break;
			case KING:
				x = 'K';
				break;
			case ROOK:
				x = 'R';
				break;

			case BISHOP | BLACKPIECE:
				x = 'b';
				break;
			case KNIGHT | BLACKPIECE:
				x = 'n';
				break;
			case PAWN | BLACKPIECE:
				x = 'p';
				break;
			case QUEEN | BLACKPIECE:
				x = 'q';
				break;
			case KING | BLACKPIECE:
				x = 'k';
				break;
			case ROOK | BLACKPIECE:
				x = 'r';
				break;
			}
			row[n] = x;
		}
		LOGGER_0("  +---+---+---+---+---+---+---+---+\n");
		LOGGER_0("%c | %c | %c | %c | %c | %c | %c | %c | %c |\n",
			f + '1', row[0], row[1], row[2], row[3], row[4], row[5],
			row[6], row[7]);
	}
	LOGGER_0("  +---+---+---+---+---+---+---+---+\n");
	LOGGER_0("    A   B   C   D   E   F   G   H  \n");
	writeEPD_FEN(b, buff, 0, "");
	LOGGER_0("%s\n", buff);

	pw = (b->mindex % PB_MI) / PW_MI;
	pb = (b->mindex % XX_MI) / PB_MI;
	
	nw = (b->mindex % NB_MI) / NW_MI;
	nb = (b->mindex % BWL_MI) / NB_MI;
	bwl = (b->mindex % BWD_MI) / BWL_MI;
	bwd = (b->mindex % BBL_MI) / BWD_MI;
	bbl = (b->mindex % BBD_MI) / BBL_MI;
	bbd = (b->mindex % RW_MI) / BBD_MI;
	rw = (b->mindex % RB_MI) / RW_MI;
	rb = (b->mindex % QW_MI) / RB_MI;
	qw = (b->mindex % QB_MI) / QW_MI;
	qb = (b->mindex % PW_MI) / QB_MI;
	LOGGER_0("%d, %d, %d, %d, %d, %d\n", pw, nw, bwl, bwd, rw, qw);
	LOGGER_0("%d, %d, %d, %d, %d, %d\n", pb, nb, bbl, bbd, rb, qb);
}

int kingCheck(board *b)
{
	BITVAR x;
	int from, c1, c2;
	
	x = (b->maps[KING]) & (b->colormaps[WHITE]);
	c1 = BitCount(x);
	if (!c1) {
		LOGGER_0("Missing kings\n");
		return 0;
	}
	from = LastOne(x);
	if ((x == 0ULL) || (from != b->king[WHITE]) || (c1 != 1)) {
		LOGGER_0("%lld, %d=%lld %o, count %d WHITE\n",
			(unsigned long long ) x, from, 1ULL << from,
			b->king[WHITE], c1);
		return 0;
	}
	x = (b->maps[KING]) & (b->colormaps[BLACK]);
	c2 = BitCount(x);
	from = LastOne(x);
	if ((x == 0ULL) || (from != b->king[BLACK]) || (c2 != 1)) {
		LOGGER_0("%lld, %o=%lld, %o, count %d BLACK\n",
			(unsigned long long ) x, from, 1ULL << from,
			b->king[BLACK], c2);
		return 0;
	}
	x = attack.maps[KING][b->king[WHITE]];
	if (x & normmark[b->king[BLACK]]) {
		printmask(x, "x");
		printmask(attack.maps[KING][b->king[BLACK]], "BLACK");
		return 0;
	}
	return 1;
}

int boardCheck(board *b, char *name)
{

	int ret;
	BITVAR key;
	int matidx;

	if (kingCheck(b) == 0) {
		printBoardNice(b);
		LOGGER_0("king error!\n");
	}
	ret = 1;
	if (b->colormaps[WHITE] & b->colormaps[BLACK]) {
		ret = 0;
		LOGGER_1("ERR: %s, Black and white piece on the same square\n", name);
		printmask(b->colormaps[WHITE], "WHITE");
		printmask(b->colormaps[BLACK], "BLACK");
	}
	key = getKey(b);
	if (b->key != key) {
		ret = 0;
		LOGGER_1("ERR: %s, Keys dont match, board key %llX, computed key %llX\n",name, (unsigned long long) b->key, (unsigned long long) key);
	}
	matidx = computeMATIdx(b);
	if (b->mindex != matidx) {
		ret = 0;
		LOGGER_1("ERR: %s, Material indexes dont match, board mindex %X, computed mindex %X\n",name, b->mindex, matidx);
	}

#if 0		
		for(f=0; f<64; f++) {
			wh=b->colormaps[WHITE] & normmark[f];
			bl=b->colormaps[BLACK] & normmark[f];
			no=b->norm & normmark[f];
			pa=b->maps[PAWN] & normmark[f];
			bi=b->maps[BISHOP] & normmark[f];
			kn=b->maps[KNIGHT] & normmark[f];
			ki=b->maps[KING] & normmark[f];
			qu=b->maps[QUEEN] & normmark[f];
			ro=b->maps[ROOK] & normmark[f];
			pp=b->pieces[f];

			blb=BitCount(bl);
			whb=BitCount(wh);
//			nob=BitCount(no);
			pab=BitCount(pa);
			bib=BitCount(bi);
			knb=BitCount(kn);
			kib=BitCount(ki);
			qub=BitCount(qu);
			rob=BitCount(ro);

			sprintf(bf,"Square %d: ",f);

// empty place
			if((pp==ER_PIECE ) || (no==0)) {
				if(pp!=ER_PIECE) {
					ret=0;
					strcat(bf,"emp: Piece NOT_E ");
				}
				if(no!=0) {
					ret=0;
					strcat(bf,"emp: NORM NOT_E ");
				}
				if((bl|wh)!=0) {
					ret=0;
					if(bl!=0) strcat(bf,"emp: Black NOT_E ");
					if(wh!=0) strcat(bf,"emp: White NOT_E ");
				}
				if ((pa|kn|bi|ro|qu|ki)!=0) {
					ret=0;
					if(pa!=0) strcat(bf,"emp: PAWN NOT_E ");
					if(kn!=0) strcat(bf,"emp: KNIGHT NOT_E ");
					if(bi!=0) strcat(bf,"emp: BISHOP NOT_E ");
					if(ro!=0) strcat(bf,"emp: ROOK NOT_E ");
					if(qu!=0) strcat(bf,"emp: QUEEN NOT_E ");
					if(ki!=0) strcat(bf,"emp: KING NOT_E ");
				}
			}
// non empty
			else {

				if(pp==ER_PIECE) {
					ret=0;
					strcat(bf,"nmp: Piece IS_E ");
				}
				if(no==0) {
					ret=0;
					strcat(bf,"nmp: NORM IS_E ");
				}
				if((bl|wh)==0) {
					ret=0;
					strcat(bf,"nmp: COLORs IS_E ");
				}
				if((blb+whb)>1) {
					ret=0;
					strcat(bf,"nmp: BOTH colors SET_UP ");
				}
				if(pp!=ER_PIECE) {
					if(pp&BLACKPIECE) {
						if(whb!=0){
							ret=0;
							strcat(bf,"nmp: BLACK piece WHITE map setup ");
						}
						if(blb==0){
							ret=0;
							strcat(bf,"nmp: BLACK piece BLACK map IS_E ");
						}
					} else {
						if(blb!=0){
							ret=0;
							strcat(bf,"nmp: WHITE piece BLACK map setup ");
						}
						if(whb==0){
							ret=0;
							strcat(bf,"nmp: WHITE piece WHITE map IS_E ");
						}
					}
					ppp=pp&PIECEMASK;
					switch(ppp) {
					case 0:
							ppp=pab;
							break;
					case 1:
							ppp=knb;
							break;
					case 2:
							ppp=bib;
							break;
					case 3:
							ppp=rob;
							break;
					case 4:
							ppp=qub;
							break;
					case 5:
							ppp=kib;
							break;
					default:
							ret=0;
							break;
					}
					if(ppp==0) {
						ret=0;
						sprintf(b2, "nmp: PIECE %d map not SETUP ", ppp);
						strcat(bf, b2);
						if((pab+knb+bib+rob+qub+kib)>0) {
							ret=0;
							sprintf(b2, "nmp: OTHER MAPs setup P,N,B,R,Q,K %d,%d,%d,%d,%d,%d ", pab, knb, bib, rob, qub, kib);
							strcat(bf, b2);
						}
					}
					if((pab+knb+bib+rob+qub+kib)>1) {
						ret=0;
						sprintf(b2, "nmp: MORE MAPs setup P,N,B,R,Q,K %d,%d,%d,%d,%d,%d ", pab, knb, bib, rob, qub, kib);
						strcat(bf, b2);
					}

				} else {
					if((pab+knb+bib+rob+qub+kib)>0) {
						ret=0;
						sprintf(b2, "nmp: Some MAPS setup P,N,B,R,Q,K %d,%d,%d,%d,%d,%d ", pab, knb, bib, rob, qub, kib);
						strcat(bf, b2);
					}
				}
			}

			if(pp>(ER_PIECE|BLACKPIECE)||(pp<0)) {
				ret=0;
				sprintf(b2, "Piece value error: %X ", pp);
				strcat(bf, b2);
			}

			if(ret==0) {
				LOGGER_1("ERR:%s, %s\n",name, bf);
				printBoardNice(b);
				printboard(b);
//				return 0;
//				abort();
			}
		}
#endif

// material checks
#if 0
		pp=0;
		bwd=b->material[WHITE][DBISHOP];
		bbd=b->material[BLACK][DBISHOP];
		bwl=b->material[WHITE][BISHOP]-bwd;
		bbl=b->material[BLACK][BISHOP]-bbd;
		pw=BitCount(b->maps[PAWN]&b->colormaps[WHITE]);
		pb=BitCount(b->maps[PAWN]&b->colormaps[BLACK]);
		nw=BitCount(b->maps[KNIGHT]&b->colormaps[WHITE]);
		nb=BitCount(b->maps[KNIGHT]&b->colormaps[BLACK]);

		bwl2=BitCount(b->maps[BISHOP]&b->colormaps[WHITE]&WHITEBITMAP);
		bbl2=BitCount(b->maps[BISHOP]&b->colormaps[BLACK]&WHITEBITMAP);
		bwd2=BitCount(b->maps[BISHOP]&b->colormaps[WHITE])-bwl2;
		bbd2=BitCount(b->maps[BISHOP]&b->colormaps[BLACK])-bbl2;

		rw=BitCount(b->maps[ROOK]&b->colormaps[WHITE]);
		rb=BitCount(b->maps[ROOK]&b->colormaps[BLACK]);
		qw=BitCount(b->maps[QUEEN]&b->colormaps[WHITE]);
		qb=BitCount(b->maps[QUEEN]&b->colormaps[BLACK]);

		if(pw!=b->material[WHITE][PAWN]) pp++;
		if(pb!=b->material[BLACK][PAWN]) pp++;
		if(nw!=b->material[WHITE][KNIGHT]) pp++;
		if(nb!=b->material[BLACK][KNIGHT]) pp++;

		if(bwd!=bwd2) pp++;
		if(bbd!=bbd2) pp++;
		if(bwl!=bwl2) pp++;
		if(bbl!=bbl2) pp++;

		if(rw!=b->material[WHITE][ROOK]) pp++;
		if(rb!=b->material[BLACK][ROOK]) pp++;
		if(qw!=b->material[WHITE][QUEEN]) pp++;
		if(qb!=b->material[BLACK][QUEEN]) pp++;

		if(pp>0) {
			printBoardNice(b);
			printboard(b);
			if(pw!=b->material[WHITE][PAWN]) LOGGER_0("boardcheck WP problem mat %d: board %d\n", b->material[WHITE][PAWN], pw);
			if(pb!=b->material[BLACK][PAWN]) LOGGER_0("boardcheck BP problem mat %d: board %d\n", b->material[BLACK][PAWN], pb);
			if(nw!=b->material[WHITE][KNIGHT]) LOGGER_0("boardcheck WN problem mat %d: board %d\n", b->material[WHITE][KNIGHT], nw);
			if(nb!=b->material[BLACK][KNIGHT]) LOGGER_0("boardcheck BN problem mat %d: board %d\n", b->material[BLACK][KNIGHT], nb);

			if(bwd!=bwd2) LOGGER_0("boardcheck WB problem mat %d: board %d\n", bwd2, bwd);
			if(bbd!=bbd2) LOGGER_0("boardcheck BB problem mat %d: board %d\n", bbd2, bbd);
			if(bwl!=bwl2) LOGGER_0("boardcheck WBL problem mat %d: board %d\n", bwl2, bwl);
			if(bbl!=bbl2) LOGGER_0("boardcheck BBL problem mat %d: board %d\n", bbl2, bbl);

			if(rw!=b->material[WHITE][ROOK]) LOGGER_0("boardcheck WR problem mat %d: board %d\n", b->material[WHITE][ROOK], rw);
			if(rb!=b->material[BLACK][ROOK]) LOGGER_0("boardcheck BR problem mat %d: board %d\n", b->material[BLACK][ROOK], rb);
			if(qw!=b->material[WHITE][QUEEN]) LOGGER_0("boardcheck WQ problem mat %d: board %d\n", b->material[WHITE][QUEEN], qw);
			if(qb!=b->material[BLACK][QUEEN]) LOGGER_0("boardcheck BQ problem mat %d: board %d\n", b->material[BLACK][QUEEN], qb);
			return 0;
		}
#endif
	return ret;
}

void eval_dump(board const *b, attack_model *a, personality const *p){
int si, pi, sq, fr;
int mab[3], mae[3], mob[3], moe[3];
int sqb[3], sqe[3], spb[3], spe[3];

int mabx[3], maex[3], mobx[3], moex[3];
int sqbx[3], sqex[3], spbx[3], spex[3];
BITVAR x;

	mab[0]=mae[0]=mob[0]=moe[0]=sqb[0]=sqe[0]=spb[0]=spe[0]=0;
	mab[1]=mae[1]=mob[1]=moe[1]=sqb[1]=sqe[1]=spb[1]=spe[1]=0;
	L0("\nBasic Info\n");
	LOGGER_0("M:mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material,
		a->sc.side[0].mobi_b, a->sc.side[1].mobi_b,
		a->sc.side[0].sqr_b, a->sc.side[1].sqr_b, a->sc.side[0].specs_b, a->sc.side[1].specs_b);
	LOGGER_0("E:mat %d, mob %d, mob %d, sqr %d, sqr %d, spc %d, spc %d\n", a->sc.material_e ,
		a->sc.side[0].mobi_e,a->sc.side[1].mobi_e,
		a->sc.side[0].sqr_e, a->sc.side[1].sqr_e, a->sc.side[0].specs_e, a->sc.side[1].specs_e);
	LOGGER_0("T:score %d, phase %d, score_b %d, score_e %d\n", a->sc.complete, a->phase, a->sc.score_b, a->sc.score_e);

	L0("\nPiece/Square specific\n");
	for(si=WHITE; si<=BLACK; si++){
	  for(pi=PAWN; pi<ER_PIECE; pi++){
		x=b->maps[pi]&b->colormaps[si];
		while(x) {
			fr=LastOne(x);
			if(pi!=PAWN) {
			  if(pi==KING) {
				L0("%d:%d =>sqb %d:%d, sqe %d:%d, sqq %d:%d\n", pi, si,
					a->sq[fr].sqr_b, p->piecetosquare[MG][si][pi][fr],
					a->sq[fr].sqr_e, p->piecetosquare[EG][si][pi][fr],
					(a->sq[fr].sqr_b*a->phase + a->sq[fr].sqr_e*(255-a->phase))/255,
					(p->piecetosquare[MG][si][pi][fr]*a->phase + p->piecetosquare[EG][si][pi][fr]*(255-a->phase))/255
					);
			  } else {
				L0("%d:%d =>mab %d, mae %d, mmm %d, sqb %d:%d, sqe %d:%d, sqq %d:%d\n", pi, si,
					p->Values[0][pi], p->Values[1][pi], (p->Values[0][pi]*a->phase + p->Values[1][pi]*(255-a->phase))/255,
					a->sq[fr].sqr_b, p->piecetosquare[MG][si][pi][fr],
					a->sq[fr].sqr_e, p->piecetosquare[EG][si][pi][fr],
					(a->sq[fr].sqr_b*a->phase + a->sq[fr].sqr_e*(255-a->phase))/255,
					(p->piecetosquare[MG][si][pi][fr]*a->phase + p->piecetosquare[EG][si][pi][fr]*(255-a->phase))/255
					);
				mab[si]+=p->Values[0][pi];
				mae[si]+=p->Values[1][pi];
			  } 
				sqb[si]+=a->sq[fr].sqr_b;
				sqe[si]+=a->sq[fr].sqr_e;
			}else {
				L0("%d:%d =>mab %d, mae %d, mmm %d\n", pi, si,
					p->Values[0][pi], p->Values[1][pi], (p->Values[0][pi]*a->phase + p->Values[1][pi]*(255-a->phase))/255
					);
				mab[si]+=p->Values[0][pi];
				mae[si]+=p->Values[1][pi];
			}

			if(pi!=PAWN) {
				L0("%d:%d =>mob %d, moe %d, mom %d, spb %d, spe %d, spq %d\n", pi, si,
					a->me[fr].pos_mob_tot_b, a->me[fr].pos_mob_tot_e, (a->me[fr].pos_mob_tot_b*a->phase + a->me[fr].pos_mob_tot_e*(255-a->phase))/255,
					a->scc[fr].sqr_b, a->scc[fr].sqr_e, (a->scc[fr].sqr_b*a->phase + a->scc[fr].sqr_e*(255-a->phase))
				);
				spb[si]+=a->scc[fr].sqr_b;
				spe[si]+=a->scc[fr].sqr_e;
				mob[si]+=a->me[fr].pos_mob_tot_b;
				moe[si]+=a->me[fr].pos_mob_tot_e;
			}
			ClrLO(x);
		}
	  }
	}
	mae[2]=mae[0]-mae[1];
	mab[2]=mab[0]-mab[1];
	moe[2]=moe[0]-moe[1];
	mob[2]=mob[0]-mob[1];

	sqb[2]=sqb[0]-sqb[1];
	sqe[2]=sqe[0]-sqe[1];
	spb[2]=spb[0]-spb[1];
	spe[2]=spe[0]-spe[1];

	L0("\nSide summaries\n");
	L0("recalc\n");
	L0("MAT MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", mab[0], mab[1], mab[2], mae[0], mae[1], mae[2]);
	L0("MOB MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", mob[0], mob[1], mob[2], moe[0], moe[1], moe[2]);
	L0("SQR MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", sqb[0], sqb[1], sqb[2], sqe[0], sqe[1], sqe[2]);
	L0("SPC MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", spb[0], spb[1], spb[2], spe[0], spe[1], spe[2]);

	mabx[2]=a->sc.material;
	mabx[0]=a->sc.material_b_w;
	mabx[1]=mabx[0]-mabx[2];

	maex[2]=a->sc.material_e;
	maex[0]=a->sc.material_e_w;
	maex[1]=maex[0]-maex[2];

	mobx[0]=a->sc.side[0].mobi_b;
	mobx[1]=a->sc.side[1].mobi_b;
	mobx[2]=mobx[0]-mobx[1];

	moex[0]=a->sc.side[0].mobi_e;
	moex[1]=a->sc.side[1].mobi_e;
	moex[2]=moex[0]-moex[1];

	sqbx[0]=a->sc.side[0].sqr_b;
	sqbx[1]=a->sc.side[1].sqr_b;
	sqbx[2]=sqbx[0]-sqbx[1];

	sqex[0]=a->sc.side[0].sqr_e;
	sqex[1]=a->sc.side[1].sqr_e;
	sqex[2]=sqex[0]-sqex[1];

	spbx[0]=a->sc.side[0].specs_b;
	spbx[1]=a->sc.side[1].specs_b;
	spbx[2]=spbx[0]-spbx[1];

	spex[0]=a->sc.side[0].specs_e;
	spex[1]=a->sc.side[1].specs_e;
	spex[2]=spex[0]-spex[1];

	L0("original\n");
	L0("MAT MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", mabx[0], mabx[1], mabx[2], maex[0], maex[1], maex[2]);
	L0("MOB MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", mobx[0], mobx[1], mobx[2], moex[0], moex[1], moex[2]);
	L0("SQR MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", sqbx[0], sqbx[1], sqbx[2], sqex[0], sqex[1], sqex[2]);
	L0("SPC MG %6d:%6d Df %6d, EG %6d:%6d Df %6d\n", spbx[0], spbx[1], spbx[2], spex[0], spex[1], spex[2]);

	L0("\nTotals\n");
	L0("recalc\n");
	L0("TOTb %6d:%6d Df %6d\n", (mab[0]+mob[0]+sqb[0]+spb[0]), (mab[1]+mob[1]+sqb[1]+spb[1]), (mab[2]+mob[2]+sqb[2]+spb[2]));
	L0("TOTe %6d:%6d Df %6d\n", (mae[0]+moe[0]+sqe[0]+spe[0]), (mae[1]+moe[1]+sqe[1]+spe[1]), (mae[2]+moe[2]+sqe[2]+spe[2]));
	L0("TOT Df %6d\n",((mab[2]+mob[2]+sqb[2]+spb[2])*a->phase+(mae[2]+moe[2]+sqe[2]+spe[2])*(255-a->phase))/255);

	L0("original\n");
	L0("TOTb %6d:%6d Df %6d\n", (mabx[0]+mobx[0]+sqbx[0]+spbx[0]), (mabx[1]+mobx[1]+sqbx[1]+spbx[1]), (mabx[2]+mobx[2]+sqbx[2]+spbx[2]));
	L0("TOTe %6d:%6d Df %6d\n", (maex[0]+moex[0]+sqex[0]+spex[0]), (maex[1]+moex[1]+sqex[1]+spex[1]), (maex[2]+moex[2]+sqex[2]+spex[2]));
	L0("TOT Df %6d\n",((mabx[2]+mobx[2]+sqbx[2]+spbx[2])*a->phase+(maex[2]+moex[2]+sqex[2]+spex[2])*(255-a->phase))/255);

}
