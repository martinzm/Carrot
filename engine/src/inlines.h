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

#ifndef INLINES_H
#include "bitmap.h"

extern att_mov attack;
extern uint32_t attnorm[];
extern int att90[];
extern int att45R[];
extern int att45L[];
extern int set90[];
extern BITVAR normmark[];
extern BITVAR mark90[];
extern BITVAR mark45L[];
extern BITVAR mark45R[];
extern BITVAR nnormmark[64];
extern BITVAR nmark90[64];
extern BITVAR nmark45L[64];
extern BITVAR nmark45R[64];

static inline BITVAR SetNorm(int pos, BITVAR map)
{
	return (map | normmark[pos]);
}

static inline BITVAR Set90(int pos, BITVAR map)
{
	return (map | mark90[pos]);
}

static inline BITVAR Set45R(int pos, BITVAR map)
{
	return (map | mark45R[pos]);
}

static inline BITVAR Set45L(int pos, BITVAR map)
{
	return (map | mark45L[pos]);
}

static inline BITVAR ClrNorm(int pos, BITVAR map)
{
	return (map & nnormmark[pos]);
}

static inline BITVAR Clr90(int pos, BITVAR map)
{
	return (map & nmark90[pos]);
}

static inline BITVAR Clr45R(int pos, BITVAR map)
{
	return (map & nmark45R[pos]);
}

static inline BITVAR Clr45L(int pos, BITVAR map)
{
	return (map & (nmark45L[pos]));
}

static inline BITVAR get45Rvector(BITVAR board, int pos)
{
	return attack.attack_r45R[pos][(board >> att45R[pos]) & 0xff];
}

static inline BITVAR get45Lvector(BITVAR board, int pos)
{
	return attack.attack_r45L[pos][(board >> att45L[pos]) & 0xff];
}

static inline BITVAR get90Rvector(BITVAR board, int pos){
	return attack.attack_r90R[pos][(board >> att90[pos]) & 0xff];
}
static inline BITVAR getnormvector(BITVAR board, int pos){
    return attack.attack_norm[pos][(board >> attnorm[pos]) & 0xff];
}

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

static inline void SetAll(int pos, int side, int piece, board *b)
{
	b->norm = SetNorm(pos, b->norm);
	b->r45L = Set45L(pos, b->r45L);
	b->r45R = Set45R(pos, b->r45R);
	b->r90R = Set90(pos, b->r90R);
	b->maps[piece] = SetNorm(pos, b->maps[piece]);
	b->colormaps[side] = SetNorm(pos, b->colormaps[side]);
	b->pieces[pos] = (int8_t)(piece + BLACKPIECE * side);
}

static inline void ClearAll(int pos, int side, int piece, board *b)
{
	b->norm = ClrNorm(pos, b->norm);
	b->r45L = Clr45L(pos, b->r45L);
	b->r45R = Clr45R(pos, b->r45R);
	b->r90R = Clr90(pos, b->r90R);
	b->maps[piece] = ClrNorm(pos, b->maps[piece]);
	b->colormaps[side] = ClrNorm(pos, b->colormaps[side]);
	b->pieces[pos] = ER_PIECE;
}

static inline void MoveFromTo(int from, int to, int side, int piece, board *b)
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


static inline int GT_M(board const *b, personality const *p, int s, int pi, int fo)
{
       return fo != 0 ? BitCount(b->maps[pi] & b->colormaps[s]) :
               p->mat_info[b->mindex].m[s][pi];
}

// returns material info about particular type on board - from precomputed table, based on normal num of pieces
static inline int GT_M0(board const *b, personality const *p, int s, int pi)
{
       return p->mat_info[b->mindex].m[s][pi];
}

// counts particular piece on board for side
static inline int GT_M1(board const *b, personality const *p, int s, int pi)
{
       return BitCount(b->maps[pi] & b->colormaps[s]);
}



#define INLINES_H

#endif
