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

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "bitmap.h"
#include "hash.h"

extern char eVERS[];
extern char eREL[];
extern char eFEATS[];
extern char eNAME[];

extern char *SQUARES_ASC[];
extern char PIECES_ASC[];

extern const int Piece_Map[];
extern const int Square_Swap[64];

extern unsigned char ind45R[];
extern unsigned char ind45L[];
extern unsigned char ind90[];
extern unsigned char indnorm[];

extern BITVAR nmark90[64];
extern BITVAR nmark45R[64];
extern BITVAR nmark45L[64];

extern att_mov attack;

/*
 material table index
 */

extern int MATIdxIncW[ER_PIECE * 2];
extern int MATIdxIncB[ER_PIECE * 2];
extern int64_t MATincW2[ER_PIECE * 2];
extern int64_t MATincB2[ER_PIECE * 2];

extern BITVAR randomTable[ER_SIDE][ER_SQUARE][ER_PIECE];
extern BITVAR sideKey;
extern BITVAR epKey[ER_SQUARE + 1];
extern BITVAR castleKey[ER_SIDE][ER_CASTLE];
extern hashEntry_e *hashTable;
extern int hashValidId;

extern kmoves *killer_moves;

extern int engine_stop;
extern struct _statistics STATS[MAXPLY + 2];

#endif /* GLOBALS_H_ */
