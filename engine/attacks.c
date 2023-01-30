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

#include "attacks.h"
#include "bitmap.h"
#include "generate.h"
#include "globals.h"
#include "movgen.h"
#include "utils.h"
#include <assert.h>
#include "evaluate.h"

#if 0
BITVAR RookAttacks(board *b, int pos)
{
	return getnormvector(b->norm,pos) | get90Rvector(b->r90R,pos);
}

BITVAR BishopAttacks(board *b, int pos)
{
		return get45Rvector(b->r45R,pos) | get45Lvector(b->r45L,pos);
}

BITVAR QueenAttacks(board *b, int pos)
{
		return RookAttacks(b, pos) | BishopAttacks(b, pos);
}
#endif

BITVAR KnightAttacks(board const *b, int pos)
{
		return (attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
}

// generate bitmap containing all pieces attacking this square
BITVAR AttackedTo(board *b, int pos)
{
	BITVAR ret;
	ret= (RookAttacks(b, pos) & (b->maps[ROOK]|b->maps[QUEEN]));
	ret|=(BishopAttacks(b, pos) & (b->maps[BISHOP]|b->maps[QUEEN]));
	ret|=(attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
	ret|=(attack.maps[KING][pos] & b->maps[KING]);
	ret|=(attack.pawn_att[WHITE][pos] & b->maps[PAWN] & (b->colormaps[BLACK]));
	ret|=(attack.pawn_att[BLACK][pos] & b->maps[PAWN] & (b->colormaps[WHITE]));

//	printmask(ret,"att");
	return ret;
}

BITVAR DiagAttacks_2(board *b, int pos)
{
BITVAR t11,t12, t21, t22;
	get45Rvector2(b->r45R,pos, &t11, &t21);
	get45Lvector2(b->r45L,pos, &t12, &t22);
	return (t21|t22);
}

BITVAR NormAttacks_2(board *b, int pos)
{
BITVAR t11,t12, t21, t22;
	getnormvector2(b->norm,pos, &t11, &t21);
	get90Rvector2(b->r90R,pos, &t12, &t22);
	return (t21|t22);
}

// utoky kralem jsou take pocitany, nebot uvazujeme naprosto nechranene pole.
// odpovida na otazku, kdo utoci na pole TO obsazene stranou SIDE (tj utocnik je z druhe strany nez SIDE)
BITVAR AttackedTo_A_OLD(board *b, int to, int side)
{
	BITVAR cr, di, cr2, di2, cr_a, di_a, kn_a, pn_a, ki_a, ret, nnorm;
	int s, ff;

	s=side^1;
	nnorm=~normmark[to];

	cr=(attack.maps[ROOK][to]) & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s]);
	di_a=cr_a = 0;
	while(cr) {
		ff = LastOne(cr);
		cr2=attack.rays[to][ff] & (~normmark[ff]) & nnorm;
		if(!(cr2 & b->norm)) cr_a |= normmark[ff];
		ClrLO(cr);
	}

	di=(attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]);
	while(di) {
		ff = LastOne(di);
		di2=attack.rays[to][ff] & (~normmark[ff]) & nnorm;
		if(!(di2 & b->norm)) di_a |= normmark[ff];
		ClrLO(di);
	}

	kn_a=attack.maps[KNIGHT][to] & b->maps[KNIGHT];
	pn_a=attack.pawn_att[side][to] & b->maps[PAWN];
	ki_a=(attack.maps[KING][to] & b->maps[KING]);
	ret=(cr_a|di_a|kn_a|pn_a|ki_a) & b->colormaps[s];
	return ret;
}

// get LVA attacker to square to from side
int GetLVA_to(board *b, int to, int side, BITVAR ignore)
{
	BITVAR cr, di, kn_a, pn_a, ki_a, norm, ns;
	int s, ff;

	s=side^1;

	norm=b->norm & ignore;
	ns=norm & b->colormaps[side];
	pn_a=(attack.pawn_att[s][to] & b->maps[PAWN] & ns);
	if(pn_a) return LastOne(pn_a);
	kn_a=(attack.maps[KNIGHT][to] & b->maps[KNIGHT] & ns);
	if(kn_a)return LastOne(kn_a);

	di=attack.maps[BISHOP][to] & b->maps[BISHOP] & ns;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}
	
	cr=attack.maps[ROOK][to] & b->maps[ROOK] & ns;
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(cr);
	}

	di=((attack.maps[BISHOP][to] & b->maps[QUEEN]) | (attack.maps[ROOK][to] & b->maps[QUEEN]))&ns;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}

	ki_a=attack.maps[KING][to] & b->maps[KING] & ns;
	if(ki_a) return LastOne(ki_a); else return -1;
}

// get LVA attacker to square to from side
int GetLVA_to2(board *b, int to, int side, BITVAR ignore)
{
	BITVAR cr, di, kn_a, pn_a, ki_a, norm;
	int s, ff;

	s=side^1;

	norm=b->norm & ignore;
	pn_a=(attack.pawn_att[s][to] & b->maps[PAWN] & norm & b->colormaps[side]);
	if(pn_a) return LastOne(pn_a);
	kn_a=(attack.maps[KNIGHT][to] & b->maps[KNIGHT] & norm & b->colormaps[side]);
	if(kn_a)return LastOne(kn_a);

	di=attack.maps[BISHOP][to] & b->maps[BISHOP] & b->colormaps[side] & norm;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}
	
	cr=attack.maps[ROOK][to] & b->maps[ROOK] & b->colormaps[side] & norm;
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(cr);
	}

	di=((attack.maps[BISHOP][to] & b->maps[QUEEN]) | (attack.maps[ROOK][to] & b->maps[QUEEN]))&(b->colormaps[side]) & norm;
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & norm)) return ff;
		ClrLO(di);
	}

	ki_a=(attack.maps[KING][to] & b->maps[KING] & b->colormaps[side]) & norm;
	if(ki_a) return LastOne(ki_a); else return -1;
}
// create full map of attackers to mentioned square belonging to side
BITVAR AttackedTo_A(board *b, int to, int side)
{
	BITVAR cr, di, cr_a, di_a, kn_a, pn_a, ki_a, ret;
	int s, ff;

	s=side^1;

	cr=(attack.maps[ROOK][to]) & (b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s]);
	di_a=cr_a = 0;
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & b->norm)) cr_a |= normmark[ff];
		ClrLO(cr);
	}

	di=(attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]);
	while(di) {
		ff = LastOne(di);
		if(!(attack.rays_int[to][ff] & b->norm)) di_a |= normmark[ff];
		ClrLO(di);
	}

	kn_a=attack.maps[KNIGHT][to] & b->maps[KNIGHT];
	pn_a=attack.pawn_att[side][to] & b->maps[PAWN];
	ki_a=(attack.maps[KING][to] & b->maps[KING]);
	ret=(cr_a|di_a|kn_a|pn_a|ki_a) & b->colormaps[s];
	return ret;
}

// just answer to question if square belonging to side is under attack from opponent
BITVAR AttackedTo_B(board *b, int to, int side)
{
	BITVAR cr;
	int s, ff;

	s=side^1;

	assert((to>=0)&&(to<64));
	cr=((attack.maps[ROOK][to])&(b->maps[ROOK]|b->maps[QUEEN])&(b->colormaps[s])) | ((attack.maps[BISHOP][to]) & (b->maps[BISHOP]|b->maps[QUEEN])&(b->colormaps[s]));
	while(cr) {
		ff = LastOne(cr);
		if(!(attack.rays_int[to][ff] & b->norm)) return 1;
		ClrLO(cr);
	}

	if(attack.pawn_att[side][to] & b->maps[PAWN] & b->colormaps[s]) return 1;
	if(attack.maps[KNIGHT][to] & b->maps[KNIGHT] & b->colormaps[s]) return 1;
	if(attack.maps[KING][to] & b->maps[KING] & b->colormaps[s]) return 1;
	return 0;
}

//is side in check ?
// must take care of pawn attacks
// pawn attack only in forward direction

int isInCheck(board *b, int side)
{
BITVAR q;
unsigned char z;	
	z=((side == WHITE) ? BLACK : WHITE);
	q=(AttackedTo(b,b->king[side])&(b->colormaps[z]));
//	printboard(b);
	if(q!=0) {
	    return 1;
	}
	return 0;
}

// generate bitmap of white/black pawn attacked squares. EP attack is not included
// generate from current board;
// returns all squares where pawns attack - include even attacks by pinned pawns
// atmap contains map of all squares which can be captured by pawns

// Only for eval computation
// PINS must be evaluated!

BITVAR WhitePawnAttacks(board *b, attack_model *a)
{
BITVAR x,r,r2, pins, mv;
int from;
	pins=((a->ke[WHITE].cr_pins | a->ke[WHITE].di_pins) & b->maps[PAWN] & b->colormaps[WHITE]);
	x=b->maps[PAWN] & b->colormaps[WHITE] & (~pins);
	r2=r=((x & ~(FILEH | RANK8))<<9 | (x & ~(FILEA | RANK8))<<7);
	while(pins!=0UL) {
		from = LastOne(pins);
		mv = (attack.pawn_att[WHITE][from]) & attack.rays[b->king[WHITE]][from];
		r|=mv;
		r2|=(attack.pawn_att[WHITE][from]);
		ClrLO(pins);
	}
//	*atmap=r;
	return r2;
}

BITVAR WhitePawnAttacksN(board *b, attack_model *a)
{
BITVAR x,r,r2, pins, mv;
BITVAR ret, empty, set1, set2, set3,set4,  at;
int from;

//		empty=~b->norm;
		set3 = b->colormaps[WHITE]&b->maps[PAWN];
		ret = (((set3 << 9) &0xfefefefefefefefe ) | ((set3 << 7) &0x7f7f7f7f7f7f7f7f ));
		return ret;
}

void ttt(board *b, attack_model *a, BITVAR *atmap){
	if(b->maps[PAWN] & b->colormaps[BLACK]) NLOGGER_0("x");
return;
}

BITVAR BlackPawnAttacks(board *b, attack_model *a)
{
BITVAR x,r,r2, pins, mv, p2;
int from;

	pins=((a->ke[BLACK].cr_pins | a->ke[BLACK].di_pins) & b->maps[PAWN] & b->colormaps[BLACK]);
	x=b->maps[PAWN] & b->colormaps[BLACK] & (~pins);
	r2=r=((x & ~(FILEA | RANK1))>>9 | (x & ~(FILEH | RANK1))>>7);
//pins
	while(pins!=0) {
		from = LastOne(pins) & 63;
		assert((from>=0)&&(from<64));
		mv = (attack.pawn_att[BLACK][from]) & attack.rays[b->king[BLACK]][from];
		r|=mv;
		r2|=(attack.pawn_att[BLACK][from]);
		ClrLO(pins);
	}
//	*atmap=r;
	return r2;
}

BITVAR BlackPawnAttacksN(board *b, attack_model *a)
{
BITVAR x,r,r2, pins, mv;
BITVAR ret, empty, set1, set2, set3,set4,  at;
int from;

//		empty=~b->norm;
		set3 = b->colormaps[BLACK]&b->maps[PAWN];
		ret =  (((set3 >> 7) &0xfefefefefefefefe ) | ((set3 >> 9) &0x7f7f7f7f7f7f7f7f ));
		return ret;
}

// generate all possible pawn moves for current board 
BITVAR WhitePawnMovesO(board *b, attack_model *a)
{
BITVAR x, pins, r;
int from;
	pins=((a->ke[WHITE].cr_pins | a->ke[WHITE].di_pins) & b->maps[PAWN] & b->colormaps[WHITE]);
	x=b->maps[PAWN] & b->colormaps[WHITE] & (~pins);
	r = ((x << 8) & (~b->norm));
	r|= (((r&RANK3) << 8) & (~b->norm));
//pins
	while(pins!=0) {
		from = LastOne(pins);
		r|= (attack.pawn_move[WHITE][from]) & attack.rays[b->king[WHITE]][from] & (~b->norm);
		ClrLO(pins);
	}
return r;
}

BITVAR WhitePawnMoves(board *b, attack_model *a)
{
BITVAR set3, ret;
	set3 = b->colormaps[WHITE]&b->maps[PAWN];
	ret = (((set3 << 8)));
	return ret;
}

BITVAR BlackPawnMoves(board *b, attack_model *a){
BITVAR x, pins, r;
int from;
	pins=((a->ke[BLACK].cr_pins | a->ke[BLACK].di_pins) & b->maps[PAWN] & b->colormaps[BLACK]);
	x=b->maps[PAWN] & b->colormaps[BLACK] & (~pins);
	r = ((x >> 8) & (~b->norm));
	r|= (((x&RANK6) >> 8) & (~b->norm));
	while(pins!=0) {
		from = LastOne(pins);
		r|= (attack.pawn_move[BLACK][from]) & attack.rays[b->king[BLACK]][from] & (~b->norm);
		ClrLO(pins);
	}
return r;
}

// propagate pieces north, along empty squares - ie iboard is occupancy inversed, 1 means empty square
// result has squares in between initial position and stop set, not including initial position and final(blocked) squares
BITVAR FillNorth(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |=          ((pieces << 8) & iboard);
	return 				flood<<8;
}

BITVAR FillSouth(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |=          (pieces >> 8) & iboard;

	return 				flood>>8;
}

BITVAR FillWest(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &=N;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |=          (pieces >> 1) & iboard;

	return 				(flood>>1)  & N;
}

BITVAR FillEast(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
const BITVAR N = 0xfefefefefefefefe;
	iboard &=N;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |=          (pieces << 1) & iboard;

	return 				(flood<<1)  & N;
}

BITVAR FillNorthEast(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
const BITVAR N = 0xfefefefefefefefe;
	iboard &=N;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |=          (pieces << 9) & iboard;

	return 				(flood<<9)  & N;
}

BITVAR FillNorthWest(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &=N;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |=          (pieces << 7) & iboard;

	return 				(flood<<7)  & N;
}

BITVAR FillSouthEast(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
const BITVAR N = 0xfefefefefefefefe;
	iboard &=N;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |=          (pieces >> 7) & iboard;

	return 				(flood>>7)  & N;
}

BITVAR FillSouthWest(BITVAR pieces, BITVAR iboard, BITVAR init) {
BITVAR flood = init;
const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &=N;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |=          (pieces >> 9) & iboard;

	return 				(flood>>9)  & N;
}

// it generates squares OPSIDE king cannot step on, it ignores PINS
// build all squares attacked by side 

BITVAR KingAvoidSQ(board const *b, attack_model *a, int side)
{
BITVAR ret, empty, set1, set2, set3,set4,  at;
int from, opside;
	
	opside= Flip(side);
	empty=~b->norm;
// remove opside king to allow attack propagation beyond it
	empty|=normmark[b->king[opside]];
	
	set1 =b->colormaps[side]&(b->maps[QUEEN]|b->maps[ROOK]);
	set2 =b->colormaps[side]&(b->maps[QUEEN]|b->maps[BISHOP]);
	ret  = FillNorth(set1, empty, set1) | FillSouth(set1, empty, set1) | FillEast(set1, empty, set1) | FillWest(set1, empty, set1)
		|  FillNorthEast(set2, empty, set2) | FillNorthWest(set2, empty, set2)
		|  FillSouthEast(set2, empty, set2) | FillSouthWest(set2, empty, set2);
	
	set3 = b->colormaps[side]&b->maps[PAWN];
	ret |= (side==WHITE) ? (((set3 << 9) &0xfefefefefefefefe ) | ((set3 << 7) &0x7f7f7f7f7f7f7f7f ))
		: (((set3 >> 7) &0xfefefefefefefefe ) | ((set3 >> 9) &0x7f7f7f7f7f7f7f7f ));

	set1 = (b->maps[KNIGHT]&b->colormaps[side]);
	while (set1) {
		from = LastOne(set1);
		ret |= (attack.maps[KNIGHT][from]);
		ClrLO(set1);
	}
// fold in my king as purpose is to cover all squares opside king cannot step on
	ret |= (attack.maps[KING][b->king[side]]);

return ret;
}
