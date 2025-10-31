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

#include "bitmap.h"
#include "utils.h"
#include "globals.h"


/*
 * markXXXfrom] contains mapping of position from to bit in respective
 * rotated board. XXX indicates rotation
 */

/*
 columns (from lowest bit up)
 A1 A2 A3 ....... H6 H7 H8
 */

BITVAR mark90[] = { 1ULL << 0, 1ULL << 8, 1ULL << 16, 1ULL << 24, 1ULL << 32,
	1ULL << 40, 1ULL << 48, 1ULL << 56, 1ULL << 1, 1ULL << 9, 1ULL << 17,
	1ULL << 25, 1ULL << 33, 1ULL << 41, 1ULL << 49, 1ULL << 57, 1ULL << 2,
	1ULL << 10, 1ULL << 18, 1ULL << 26, 1ULL << 34, 1ULL << 42, 1ULL << 50,
	1ULL << 58, 1ULL << 3, 1ULL << 11, 1ULL << 19, 1ULL << 27, 1ULL << 35,
	1ULL << 43, 1ULL << 51, 1ULL << 59, 1ULL << 4, 1ULL << 12, 1ULL << 20,
	1ULL << 28, 1ULL << 36, 1ULL << 44, 1ULL << 52, 1ULL << 60, 1ULL << 5,
	1ULL << 13, 1ULL << 21, 1ULL << 29, 1ULL << 37, 1ULL << 45, 1ULL << 53,
	1ULL << 61, 1ULL << 6, 1ULL << 14, 1ULL << 22, 1ULL << 30, 1ULL << 38,
	1ULL << 46, 1ULL << 54, 1ULL << 62, 1ULL << 7, 1ULL << 15, 1ULL << 23,
	1ULL << 31, 1ULL << 39, 1ULL << 47, 1ULL << 55, 1ULL << 63 };

/*
 Diagonals are stored as follows (from lowest bit up)
 H1
 G1 H2
 F1 G2 H3
 .......
 A7 B8
 A8
 */

BITVAR mark45R[] = { 1ULL << 28, 1ULL << 21, 1ULL << 15, 1ULL << 10, 1ULL << 6,
	1ULL << 3, 1ULL << 1, 1ULL << 0, 1ULL << 36, 1ULL << 29, 1ULL << 22,
	1ULL << 16, 1ULL << 11, 1ULL << 7, 1ULL << 4, 1ULL << 2, 1ULL << 43,
	1ULL << 37, 1ULL << 30, 1ULL << 23, 1ULL << 17, 1ULL << 12, 1ULL << 8,
	1ULL << 5, 1ULL << 49, 1ULL << 44, 1ULL << 38, 1ULL << 31, 1ULL << 24,
	1ULL << 18, 1ULL << 13, 1ULL << 9, 1ULL << 54, 1ULL << 50, 1ULL << 45,
	1ULL << 39, 1ULL << 32, 1ULL << 25, 1ULL << 19, 1ULL << 14, 1ULL << 58,
	1ULL << 55, 1ULL << 51, 1ULL << 46, 1ULL << 40, 1ULL << 33, 1ULL << 26,
	1ULL << 20, 1ULL << 61, 1ULL << 59, 1ULL << 56, 1ULL << 52, 1ULL << 47,
	1ULL << 41, 1ULL << 34, 1ULL << 27, 1ULL << 63, 1ULL << 62, 1ULL << 60,
	1ULL << 57, 1ULL << 53, 1ULL << 48, 1ULL << 42, 1ULL << 35 };

/*
 Diagonals are stored as follows (from lowest bit up)
 A1
 B1 A2
 C1 B2 A3
 ........
 H7 G8
 H8
 */

BITVAR mark45L[] = { 1ULL << 0, 1ULL << 1, 1ULL << 3, 1ULL << 6, 1ULL << 10,
	1ULL << 15, 1ULL << 21, 1ULL << 28, 1ULL << 2, 1ULL << 4, 1ULL << 7,
	1ULL << 11, 1ULL << 16, 1ULL << 22, 1ULL << 29, 1ULL << 36, 1ULL << 5,
	1ULL << 8, 1ULL << 12, 1ULL << 17, 1ULL << 23, 1ULL << 30, 1ULL << 37,
	1ULL << 43, 1ULL << 9, 1ULL << 13, 1ULL << 18, 1ULL << 24, 1ULL << 31,
	1ULL << 38, 1ULL << 44, 1ULL << 49, 1ULL << 14, 1ULL << 19, 1ULL << 25,
	1ULL << 32, 1ULL << 39, 1ULL << 45, 1ULL << 50, 1ULL << 54, 1ULL << 20,
	1ULL << 26, 1ULL << 33, 1ULL << 40, 1ULL << 46, 1ULL << 51, 1ULL << 55,
	1ULL << 58, 1ULL << 27, 1ULL << 34, 1ULL << 41, 1ULL << 47, 1ULL << 52,
	1ULL << 56, 1ULL << 59, 1ULL << 61, 1ULL << 35, 1ULL << 42, 1ULL << 48,
	1ULL << 53, 1ULL << 57, 1ULL << 60, 1ULL << 62, 1ULL << 63 };


/*
 * attXXX[from] contains how much we need to rotate to get line which contains position
 * from in rotated board. XXX indicates rotation
 */

/*
 to get proper row - indexed by position on the board
 */
uint32_t attnorm[] = { 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 16, 16, 16,
	16, 16, 16, 16, 16, 24, 24, 24, 24, 24, 24, 24, 24, 32, 32, 32, 32, 32,
	32, 32, 32, 40, 40, 40, 40, 40, 40, 40, 40, 48, 48, 48, 48, 48, 48, 48,
	48, 56, 56, 56, 56, 56, 56, 56, 56 };
/*
 to get proper column - indexed by position on the board
 */
int att90[] = { 0, 8, 16, 24, 32, 40, 48, 56, 0, 8, 16, 24, 32, 40, 48, 56, 0,
	8, 16, 24, 32, 40, 48, 56, 0, 8, 16, 24, 32, 40, 48, 56, 0, 8, 16, 24,
	32, 40, 48, 56, 0, 8, 16, 24, 32, 40, 48, 56, 0, 8, 16, 24, 32, 40, 48,
	56, 0, 8, 16, 24, 32, 40, 48, 56 };
/*
 to get proper diagonal - indexed by position on the board
 */
int att45R[] = { 28, 21, 15, 10, 6, 3, 1, 0, 36, 28, 21, 15, 10, 6, 3, 1, 43,
	36, 28, 21, 15, 10, 6, 3, 49, 43, 36, 28, 21, 15, 10, 6, 54, 49, 43, 36,
	28, 21, 15, 10, 58, 54, 49, 43, 36, 28, 21, 15, 61, 58, 54, 49, 43, 36,
	28, 21, 63, 61, 58, 54, 49, 43, 36, 28 };
/*
 to get proper diagonal - indexed by position on the board
 */
int att45L[] = { 0, 1, 3, 6, 10, 15, 21, 28, 1, 3, 6, 10, 15, 21, 28, 36, 3, 6,
	10, 15, 21, 28, 36, 43, 6, 10, 15, 21, 28, 36, 43, 49, 10, 15, 21, 28,
	36, 43, 49, 54, 15, 21, 28, 36, 43, 49, 54, 58, 21, 28, 36, 43, 49, 54,
	58, 61, 28, 36, 43, 49, 54, 58, 61, 63 };


/*
 * maskXXX[from] has occupancy mask. We AND this with rotated result
 */
unsigned char masknorm[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

unsigned char mask90[] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

unsigned char mask45R[] =
	{ 255, 127, 63, 31, 15, 7, 3, 1, 127, 255, 127, 63, 31, 15, 7, 3, 63,
		127, 255, 127, 63, 31, 15, 7, 31, 63, 127, 255, 127, 63, 31, 15,
		15, 31, 63, 127, 255, 127, 63, 31, 7, 15, 31, 63, 127, 255, 127,
		63, 3, 7, 15, 31, 63, 127, 255, 127, 1, 3, 7, 15, 31, 63, 127,
		255 };

unsigned char mask45L[] = { 1, 3, 7, 15, 31, 63, 127, 255, 3, 7, 15, 31, 63,
	127, 255, 127, 7, 15, 31, 63, 127, 255, 127, 63, 15, 31, 63, 127, 255,
	127, 63, 31, 31, 63, 127, 255, 127, 63, 31, 15, 63, 127, 255, 127, 63,
	31, 15, 7, 127, 255, 127, 63, 31, 15, 7, 3, 255, 127, 63, 31, 15, 7, 3,
	1 };

void outbinary(BITVAR m, char *o)
{
	int f;
	for (f = 63; f >= 0; f--) {
		*o++ = (m & normmark[f]) == 0 ? '0' : '1';
	}
	*o = '\0';
}

void printmask(BITVAR m, char *s)
{
	int f, n;
	BITVAR q, z;
	char buf[100], b2[20];

	if (s != NULL)
		logger("", s, "\n");
	q = 1ULL << 56;
	for (f = 8; f >= 1; f--) {
		z = q;
		sprintf(buf, "%d ", f);
		for (n = 0; n < 8; n++) {
			if (m & q)
				sprintf(b2, "o");
			else
				sprintf(b2, ".");
			strcat(buf, b2);
			q <<= 1;
		}
		logger("", buf, "\n");
		q = z >> 8;
	}
	logger("", "  ABCDEFGH\n", "");
}

void mask2print(char *b[9])
{
	int f;
	for (f = 0; f < 9; f++) {
		logger2("line %d %s\n", f, b[f]);
	}
}

void mask2init2(char (*b)[256], char *out[9])
{
	int f;
	for (f = 0; f < 9; f++) {
		b[f][0] = '\0';
		out[f] = b[f];
	}
	return;
}

void mask2init(char (*b)[9])
{
	int f;
	for (f = 0; f < 9; f++)
		b[f][0] = '\0';
	return;
}

void mask2add(char *s[9], char (*st)[9])
{
	int f;
	for (f = 0; f < 9; f++) {
		strcat(s[f], *(st + f));
		strcat(s[f], " ");
	}
}

void printmask2(BITVAR m, char (*st)[9], char *title)
{
	size_t f;
	int n;
	BITVAR q, z;

	if (title != NULL) {
		strncpy(*st, title, 9);
		f = strlen(*st);
		for (; f < 8; f++)
			st[0][f] = ' ';
		st[0][8] = '\0';
	}
	q = 1ULL << 56;
	for (f = 8; f >= 1; f--) {
		z = q;
		for (n = 0; n < 8; n++) {
			st[9 - f][n] = (m & q) ? 'o' : '.';
			q <<= 1;
		}
		st[9 - f][8] = '\0';
		q = z >> 8;
	}
}

void printmask90(BITVAR m, char *s)
{
	int f, n;
	BITVAR q;
	char buf[100], b2[20];

	if (s != NULL)
		logger("", s, "\n");
	for (f = 8; f >= 1; f--) {
		sprintf(buf, "%d ", f);
		q = 1ULL << (f - 1);
		for (n = 0; n < 8; n++) {
			if (m & q)
				sprintf(b2, "o");
			else
				sprintf(b2, ".");
			strcat(buf, b2);
			q <<= 8;
		}
		logger("", buf, "\n");
	}
	logger("", "  ABCDEFGH\n", "");
}

void printmask45R(BITVAR m, char *s)
{
	int f, n;
	BITVAR q;
	
	if (s != NULL)
		printf("%s\n", s);
	for (f = 8; f > 0; f--) {
		printf("%d ", f);
		for (n = 1; n <= 8; n++) {
			q = 1ULL
				<< (att45R[(f - 1) * 8 + n - 1]
					+ ind45R[(f - 1) * 8 + n - 1]);
			if (m & q)
				printf("o");
			else
				printf(".");
		}
		printf("\n");
	}
	printf("  ABCDEFGH\n");
}

void printmask45L(BITVAR m, char *s)
{
	int f, n;
	BITVAR q;
	
	if (s != NULL)
		printf("%s\n", s);
	for (f = 8; f > 0; f--) {
		printf("%d ", f);
		for (n = 1; n <= 8; n++) {
			q = 1ULL
				<< (att45L[(f - 1) * 8 + n - 1]
					+ ind45L[(f - 1) * 8 + n - 1]);
			if (m & q)
				printf("o");
			else
				printf(".");
		}
		printf("\n");
	}
	printf("  ABCDEFGH\n");

}

BITVAR SetNorm(int pos, BITVAR map)
{
	return (map | normmark[pos]);
}

BITVAR Set90(int pos, BITVAR map)
{
	return (map | mark90[pos]);
}

BITVAR Set45R(int pos, BITVAR map)
{
	return (map | mark45R[pos]);
}

BITVAR Set45L(int pos, BITVAR map)
{
	return (map | mark45L[pos]);
}

BITVAR ClrNorm(int pos, BITVAR map)
{
	return (map & nnormmark[pos]);
}

BITVAR Clr90(int pos, BITVAR map)
{
	return (map & nmark90[pos]);
}

BITVAR Clr45R(int pos, BITVAR map)
{
	return (map & nmark45R[pos]);
}

BITVAR Clr45L(int pos, BITVAR map)
{
	return (map & (nmark45L[pos]));
}

BITVAR get45Rvector(BITVAR board, int pos)
{
// get vector
//	return attack.attack_r45R[pos][(board >> att45R[pos]) & mask45R[pos]];
	return attack.attack_r45R[pos][(board >> att45R[pos]) & 0xff];
}

BITVAR get45Lvector(BITVAR board, int pos)
{
// get vector
//	return attack.attack_r45L[pos][(board >> att45L[pos]) & mask45L[pos]];
	return attack.attack_r45L[pos][(board >> att45L[pos]) & 0xff];
}

BITVAR get90Rvector(BITVAR board, int pos)
{
// get vector
//	return attack.attack_r90R[pos][(board >> att90[pos]) & mask90[pos]];
	return attack.attack_r90R[pos][(board >> att90[pos]) & 0xff];
}

BITVAR getnormvector(BITVAR board, int pos)
{
//uint32_t pp;
// get vector

//	pp = attnorm[pos];
//	return attck.attack_norm[pos][(board >> attnorm[pos]) & masknorm[pos]];
	return attack.attack_norm[pos][(board >> attnorm[pos]) & 0xff];
}

int get45Rvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2)
{
	BITVAR t = (board >> att45R[pos]) & mask45R[pos];
	*d1 = attack.attack_r45R[pos][t];
	*d2 = attack.attack_r45R_2[pos][t];
	return 0;
}

int get45Lvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2)
{
	BITVAR t = (board >> att45L[pos]) & mask45L[pos];
	*d1 = attack.attack_r45L[pos][t];
	*d2 = attack.attack_r45L_2[pos][t];
	return 0;
}

int get90Rvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2)
{
	BITVAR t = (board >> att90[pos]) & mask90[pos];
	*d1 = attack.attack_r90R[pos][t];
	*d2 = attack.attack_r90R_2[pos][t];
	return 0;
}

int getnormvector2(BITVAR board, int pos, BITVAR *d1, BITVAR *d2)
{
	BITVAR t = (board >> attnorm[pos]) & masknorm[pos];
	*d1 = attack.attack_norm[pos][t];
	*d2 = attack.attack_norm_2[pos][t];
	return 0;
}

int FirstOne(BITVAR board)
{
	return 63 - __builtin_clzll(board);
}

void init_nmarks()
{
	int f;
	for (f = 0; f < 64; f++) {
		nnormmark[f] = ~normmark[f];
		nmark90[f] = ~mark90[f];
		nmark45L[f] = ~mark45L[f];
		nmark45R[f] = ~mark45R[f];
	}
}

int getFileX(int pos)
{
	return indnorm[pos];
}

int getRankX(int pos)
{
	return ind90[pos];
}

void SetAll(int pos, int side, int piece, board *b)
{
	b->norm = SetNorm(pos, b->norm);
	b->r45L = Set45L(pos, b->r45L);
	b->r45R = Set45R(pos, b->r45R);
	b->r90R = Set90(pos, b->r90R);
	b->maps[piece] = SetNorm(pos, b->maps[piece]);
	b->colormaps[side] = SetNorm(pos, b->colormaps[side]);
	b->pieces[pos] = (int8_t)(piece + BLACKPIECE * side);
}

void ClearAll(int pos, int side, int piece, board *b)
{
	b->norm = ClrNorm(pos, b->norm);
	b->r45L = Clr45L(pos, b->r45L);
	b->r45R = Clr45R(pos, b->r45R);
	b->r90R = Clr90(pos, b->r90R);
	b->maps[piece] = ClrNorm(pos, b->maps[piece]);
	b->colormaps[side] = ClrNorm(pos, b->colormaps[side]);
	b->pieces[pos] = ER_PIECE;
}

void MoveFromTo(int from, int to, int side, int piece, board *b)
{
	BITVAR x;
	x = normmark[from] | normmark[to];
	b->norm ^= x;
	b->colormaps[side] ^= x;

	b->maps[piece] ^= x;

	b->r45L ^= (mark45L[from] | mark45L[to]);
	b->r45R ^= (mark45R[from] | mark45R[to]);
	b->r90R ^= (mark90[from] | mark90[to]);
	b->pieces[from] = ER_PIECE;
	b->pieces[to] = (int8_t)(piece + side * BLACKPIECE);
}

