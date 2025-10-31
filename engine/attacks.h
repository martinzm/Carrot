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

#ifndef ATTACKS_H
#define ATTACKS_H

#include "inlines.h"
#include "bitmap.h"
#include "globals.h"
static inline BITVAR RookAttacks(board const *b, int pos)
{
       return getnormvector(b->norm, pos) | get90Rvector(b->r90R, pos);
}
static inline BITVAR BishopAttacks(board const *b, int pos)
{
       return get45Rvector(b->r45R, pos) | get45Lvector(b->r45L, pos);
}
static inline BITVAR QueenAttacks(board const *b, int pos)
{
       return getnormvector(b->norm, pos) | get90Rvector(b->r90R, pos)
                       | get45Rvector(b->r45R, pos) | get45Lvector(b->r45L, pos);
}

static inline BITVAR KnightAttacks(board const *b, int pos) {
       return (attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
}


BITVAR DiagAttacks_2(board *b, int pos);
BITVAR NormAttacks_2(board *b, int pos);

BITVAR AttackedTo(board *b, int pos);
int GetLVA_to(board *b, int to, int side, BITVAR ignore);

BITVAR FillNorth(BITVAR, BITVAR, BITVAR);
BITVAR FillSouth(BITVAR, BITVAR, BITVAR);
BITVAR FillEast(BITVAR, BITVAR, BITVAR);
BITVAR FillWest(BITVAR, BITVAR, BITVAR);
BITVAR FillNorthEast(BITVAR, BITVAR, BITVAR);
BITVAR FillNorthWest(BITVAR, BITVAR, BITVAR);
BITVAR FillSouthEast(BITVAR, BITVAR, BITVAR);
BITVAR FillSouthWest(BITVAR, BITVAR, BITVAR);

BITVAR KingAvoidSQ(board const*, attack_model*, int);
BITVAR KingAvoidSQAlt(board const*, attack_model*, int);

int setup_attack_index(const board *const, attack_model *);
int attackMoveFromTo(const board *, attack_model *, int, int, int, int);
int attackRemovePiece(const board *, attack_model *, int, int, int);
int attackAddPiece(const board *, attack_model *, int, int, int);

#endif
