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

#include "globals.h"

char eVERS[] = "0.46.10";
char eREL[] = "devel";
char eNAME[] = "Carrot";
char eFEATS[] = "HH tune NMP+LMR+HH+RFP";

const int Piece_Map[] = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
	DBISHOP, LBISHOP, LIGHT, HEAVY, PIECES, TPIECES, ER_PIECE_EX
};

const int Square_Swap[] = { 56, 57, 58, 59, 60, 61, 62, 63, 48, 49, 50, 51, 52,
	53, 54, 55, 40, 41, 42, 43, 44, 45, 46, 47, 32, 33, 34, 35, 36, 37, 38,
	39, 24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23, 8,
	9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 };

BITVAR nnormmark[64];
BITVAR nmark90[64];
BITVAR nmark45R[64];
BITVAR nmark45L[64];

char *SQUARES_ASC[] = { "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2",
	"b2", "c2", "d2", "e2", "f2", "g2", "h2", "a3", "b3", "c3", "d3", "e3",
	"f3", "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a5",
	"b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6", "e6",
	"f6", "g6", "h6", "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "a8",
	"b8", "c8", "d8", "e8", "f8", "g8", "h8" };

char PIECES_ASC[] = { 'P', 'N', 'B', 'R', 'Q', 'K', 'x' };

att_mov attack;

/*
 rows (from lowest bit up)
 A1 B1 C1 ...... G8 H8
 prevod z cisla pole na BITmapu
 */

BITVAR normmark[] = { 1ULL << 0, 1ULL << 1, 1ULL << 2, 1ULL << 3, 1ULL << 4,
	1ULL << 5, 1ULL << 6, 1ULL << 7, 1ULL << 8, 1ULL << 9, 1ULL << 10, 1ULL
		<< 11, 1ULL << 12, 1ULL << 13, 1ULL << 14, 1ULL << 15, 1ULL
		<< 16, 1ULL << 17, 1ULL << 18, 1ULL << 19, 1ULL << 20, 1ULL
		<< 21, 1ULL << 22, 1ULL << 23, 1ULL << 24, 1ULL << 25, 1ULL
		<< 26, 1ULL << 27, 1ULL << 28, 1ULL << 29, 1ULL << 30, 1ULL
		<< 31, 1ULL << 32, 1ULL << 33, 1ULL << 34, 1ULL << 35, 1ULL
		<< 36, 1ULL << 37, 1ULL << 38, 1ULL << 39, 1ULL << 40, 1ULL
		<< 41, 1ULL << 42, 1ULL << 43, 1ULL << 44, 1ULL << 45, 1ULL
		<< 46, 1ULL << 47, 1ULL << 48, 1ULL << 49, 1ULL << 50, 1ULL
		<< 51, 1ULL << 52, 1ULL << 53, 1ULL << 54, 1ULL << 55, 1ULL
		<< 56, 1ULL << 57, 1ULL << 58, 1ULL << 59, 1ULL << 60, 1ULL
		<< 61, 1ULL << 62, 1ULL << 63 };

/*
 sloupec z cisla pole
 */
unsigned char indnorm[] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0,
	1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0,
	1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };

/*
 rada z cisla pole
 */
unsigned char ind90[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2,
	2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5,
	5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7 };
/*
 to get position within diagonal
 */
unsigned char ind45R[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
	2, 2, 2, 2, 2, 2, 0, 1, 2, 3, 3, 3, 3, 3, 0, 1, 2, 3, 4, 4, 4, 4, 0, 1,
	2, 3, 4, 5, 5, 5, 0, 1, 2, 3, 4, 5, 6, 6, 0, 1, 2, 3, 4, 5, 6, 7 };
/*
 to get position within diagonal
 */
unsigned char ind45L[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 2, 2,
	2, 2, 2, 2, 1, 0, 3, 3, 3, 3, 3, 2, 1, 0, 4, 4, 4, 4, 3, 2, 1, 0, 5, 5,
	5, 4, 3, 2, 1, 0, 6, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 0 };

int MATIdxIncW[ER_PIECE * 2];
int MATIdxIncB[ER_PIECE * 2];

int64_t MATincW2[ER_PIECE * 2];
int64_t MATincB2[ER_PIECE * 2];

BITVAR randomTable[ER_SIDE][ER_SQUARE][ER_PIECE];
BITVAR sideKey;
BITVAR epKey[ER_SQUARE + 1];
BITVAR castleKey[ER_SIDE][ER_CASTLE];
hashEntry_e *hashTable;
int hashValidId;

kmoves *killer_moves;
int engine_stop;

struct _statistics STATS[MAXPLY + 2];
