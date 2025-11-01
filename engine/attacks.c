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
#include "inlines.h"

// generate bitmap containing all pieces attacking this square
BITVAR AttackedTo(board *b, int pos)
{
BITVAR ret;

	ret  = (RookAttacks(b, pos) & (b->maps[ROOK] | b->maps[QUEEN]));
	ret |= (BishopAttacks(b, pos) & (b->maps[BISHOP] | b->maps[QUEEN]));
	ret |= (attack.maps[KNIGHT][pos] & b->maps[KNIGHT]);
	ret |= (attack.maps[KING][pos] & b->maps[KING]);
	ret |= (attack.pawn_att[WHITE][pos] & b->maps[PAWN]
		& (b->colormaps[BLACK]));
	ret |= (attack.pawn_att[BLACK][pos] & b->maps[PAWN]
		& (b->colormaps[WHITE]));

	return ret;
}

BITVAR DiagAttacks_2(board *b, int pos)
{
	BITVAR t11, t12, t21, t22;
	get45Rvector2(b->r45R, pos, &t11, &t21);
	get45Lvector2(b->r45L, pos, &t12, &t22);
	return (t21 | t22);
}

BITVAR NormAttacks_2(board *b, int pos)
{
	BITVAR t11, t12, t21, t22;
	getnormvector2(b->norm, pos, &t11, &t21);
	get90Rvector2(b->r90R, pos, &t12, &t22);
	return (t21 | t22);
}

// get LVA attacker to square to from side
int GetLVA_to(board *b, int to, int side, BITVAR ignore)
{
	BITVAR cr, di, kn_a, pn_a, ki_a, norm, ns;
	int s, ff;

	s = Flip(side);

	norm = b->norm & ignore;
	ns = norm & b->colormaps[side];
	pn_a = (attack.pawn_att[s][to] & b->maps[PAWN] & ns);
	if (pn_a)
		return LastOne(pn_a);
	kn_a = (attack.maps[KNIGHT][to] & b->maps[KNIGHT] & ns);
	if (kn_a)
		return LastOne(kn_a);

	di = attack.maps[BISHOP][to] & b->maps[BISHOP] & ns;
	while (di) {
		ff = LastOne(di);
		if (!(attack.rays_int[to][ff] & norm))
			return ff;
		ClrLO(di);
	}
	
	cr = attack.maps[ROOK][to] & b->maps[ROOK] & ns;
	while (cr) {
		ff = LastOne(cr);
		if (!(attack.rays_int[to][ff] & norm))
			return ff;
		ClrLO(cr);
	}

	di = ((attack.maps[BISHOP][to] & b->maps[QUEEN])
		| (attack.maps[ROOK][to] & b->maps[QUEEN])) & ns;
	while (di) {
		ff = LastOne(di);
		if (!(attack.rays_int[to][ff] & norm))
			return ff;
		ClrLO(di);
	}

	ki_a = attack.maps[KING][to] & b->maps[KING] & ns;
	if (ki_a)
		return LastOne(ki_a);
	else
		return -1;
}

// propagate pieces north, along empty squares - ie iboard is occupancy inversed, 1 means empty square
// result has squares in between initial position and stop set, not including initial position, includes final(blocked) squares
BITVAR FillNorth(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);
	flood |= pieces = ((pieces << 8) & iboard);

	return flood << 8;
}

BITVAR FillSouth(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;
	flood |= pieces = (pieces >> 8) & iboard;

	return flood >> 8;
}

BITVAR FillWest(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &= N;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;
	flood |= pieces = (pieces >> 1) & iboard;

	return (flood >> 1) & N;
}

BITVAR FillEast(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0xfefefefefefefefe;
	iboard &= N;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;
	flood |= pieces = (pieces << 1) & iboard;

	return (flood << 1) & N;
}

BITVAR FillNorthEast(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0xfefefefefefefefe;
	iboard &= N;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;
	flood |= pieces = (pieces << 9) & iboard;

	return (flood << 9) & N;
}

BITVAR FillNorthWest(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &= N;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;
	flood |= pieces = (pieces << 7) & iboard;

	return (flood << 7) & N;
}

BITVAR FillSouthEast(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0xfefefefefefefefe;
	iboard &= N;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;
	flood |= pieces = (pieces >> 7) & iboard;

	return (flood >> 7) & N;
}

BITVAR FillSouthWest(BITVAR pieces, BITVAR iboard, BITVAR init)
{
	BITVAR flood = init;
	const BITVAR N = 0x7f7f7f7f7f7f7f7f;
	iboard &= N;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;
	flood |= pieces = (pieces >> 9) & iboard;

	return (flood >> 9) & N;
}

// it generates squares OPSIDE king cannot step on, it ignores PINS
// it builds all squares attacked by side 

BITVAR KingAvoidSQ(board const *b, attack_model *a, int side)
{
	BITVAR ret, empty, set1, set2, set3, set4;
	int from, opside;
	
	opside = Flip(side);
	empty = ~b->norm;
// remove opside king to allow attack propagation beyond it
	empty |= normmark[b->king[opside]];

	set1 = b->colormaps[side] & (b->maps[QUEEN] | b->maps[ROOK]);
	set2 = b->colormaps[side] & (b->maps[QUEEN] | b->maps[BISHOP]);
	ret = FillNorth(set1, empty, set1)
		| FillSouth(set1, empty, set1)
		| FillEast(set1, empty, set1)
		| FillWest(set1, empty, set1)
		| FillNorthEast(set2, empty, set2)
		| FillSouthWest(set2, empty, set2)
		| FillNorthWest(set2, empty, set2)
		| FillSouthEast(set2, empty, set2);
	set3 = b->colormaps[side] & b->maps[PAWN];
	ret |= (side == WHITE) ? (((set3 << 9) & 0xfefefefefefefefe) | ((set3 << 7) & 0x7f7f7f7f7f7f7f7f)) :
							 (((set3 >> 7) & 0xfefefefefefefefe) | ((set3 >> 9) & 0x7f7f7f7f7f7f7f7f));
	set4 = (b->maps[KNIGHT] & b->colormaps[side]);
	while (set4) {
		from = LastOne(set4);
		ret |= (attack.maps[KNIGHT][from]);
		ClrLO(set4);
	}
// fold in my king as purpose is to cover all squares opside king cannot step on
	ret |= (attack.maps[KING][b->king[side]]);
	return ret;
}

inline static int getOneSquare(board const *b, int x, int y, int side, BITVAR *ca, BITVAR *da) {
	*ca = *da = 0;
	BITVAR cr, di;
	if(x<0 || x>7 || y<0 || y>7) return 0;
	di = BishopAttacks(b, getPos(x,y));
	cr = RookAttacks(b, getPos(x,y));
	*da = di & b->colormaps[side] & (b->maps[BISHOP] | b->maps[QUEEN]);
	*ca = cr & b->colormaps[side] & (b->maps[ROOK] | b->maps[QUEEN]);
//	printmask(*da ,"di");
//	printmask(*ca ,"cr");
return (((*ca) | (*da)) != 0 ? 1:0);
}


// alternate version, in fact only squares around king are important, we can optimize
BITVAR KingAvoidSQAlt(board const *b, attack_model *a, int side)
{
	BITVAR ret=0, ca, da, set4, set3, set2, set1, empty;
	int kx, ky, from;
	int opside = Flip(side);

	ky = getRank(b->king[opside]);
	kx = getFile(b->king[opside]);
	ClearAll(b->king[opside], opside, KING, b);
	for(int f=-1;f<=1;f++) {
		for(int n=-1;n<=1;n++) {
			if(getOneSquare(b, kx+n, ky+f, side, &ca, &da)) {
				ret |= NORMM(getPos(kx+n, ky+f));
			}
		}
	}

// optionally deal with castling
	if(b->castle[opside]!=0) {
		empty = ~b->norm;
		if(opside==WHITE) {
			set1 = b->colormaps[BLACK] & (b->maps[QUEEN] | b->maps[ROOK]);
			set2 = b->colormaps[BLACK] & (b->maps[QUEEN] | b->maps[BISHOP]);
			ret |= FillSouth(set1, empty, set1)
				| FillSouthWest(set2, empty, set2)
				| FillSouthEast(set2, empty, set2);
		} else {
			set1 = b->colormaps[WHITE] & (b->maps[QUEEN] | b->maps[ROOK]);
			set2 = b->colormaps[WHITE] & (b->maps[QUEEN] | b->maps[BISHOP]);
			ret |= FillNorth(set1, empty, set1)
				| FillNorthWest(set2, empty, set2)
				| FillNorthEast(set2, empty, set2);
		}
	}
	set3 = b->colormaps[side] & b->maps[PAWN];
	ret |= (side == WHITE) ? (((set3 << 9) & 0xfefefefefefefefe) | ((set3 << 7) & 0x7f7f7f7f7f7f7f7f)) :
							 (((set3 >> 7) & 0xfefefefefefefefe) | ((set3 >> 9) & 0x7f7f7f7f7f7f7f7f));
	set4 = (b->maps[KNIGHT] & b->colormaps[side]);
	while (set4) {
		from = LastOne(set4);
		ret |= (attack.maps[KNIGHT][from]);
		ClrLO(set4);
	}
// fold in my king as purpose is to cover all squares opside king cannot step on
	ret |= (attack.maps[KING][b->king[side]]);
	SetAll(b->king[opside], opside, KING, b);
	return ret;
}

/*
typedef struct _att_incr {
// number of pieces of type|side
	int pos_c[(ER_PIECE | BLACKPIECE) + 1];
// position at board of [piece of type|side][0..pos_c[piece of type|side]]
	int pos_m[(ER_PIECE | BLACKPIECE) + 1][10];
// bitmap of attacks of piece type|side
	BITVAR bit_pc[(ER_PIECE | BLACKPIECE) + 1];
// bitmap of attacks of side
	BITVAR bit_sd[2];
// bitmaps of individual piece attacks are in attack_model mvs

} attack_incremental;
*/

#if 0
int setup_attack_index(const board *const b, attack_model *a)
{

int i, FIG[]={ PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
BITVAR x;
	for(int side=0;side<=1;side++) {
		for(i=0;i<6;i++){
			int pc=FIG[i];
			a->pos_c[pc+side*BLACKPIECE]=-1;
			x = b->maps[pc]&b->colormaps[side];
			while (x) {
				int from=LastOne(x);
				a->pos_m[pc+side*BLACKPIECE][++(a->pos_c[pc+side*BLACKPIECE])]=from;
				ClrLO(x);
			}
		}
	}
	return 0;
}

int attackMoveFromTo(const board *b, attack_model *a, int from, int to, int side, int piece)
{
// get proper piece
int i;
	for(i=a->pos_c[piece+side*BLACKPIECE];i<=0;i--){
		if(a->pos_m[piece+side*BLACKPIECE][i]==from) break;
	}
	assert(i>=0);
	a->pos_m[piece+side*BLACKPIECE][i]=to;
return 1;
}

int attackRemovePiece(const board *b, attack_model *a, int from, int side, int piece)
{
int i;
	for(i=a->pos_c[piece+side*BLACKPIECE];i<=0;i--){
		if(a->pos_m[piece+side*BLACKPIECE][i]==from) break;
	}
	assert(i>=0);
	if(i<a->pos_c[piece+side*BLACKPIECE]) a->pos_m[piece+side*BLACKPIECE][i]=a->pos_m[piece+side*BLACKPIECE][a->pos_c[piece+side*BLACKPIECE]];
	a->pos_c[piece+side*BLACKPIECE]--;
return 1;
}

int attackAddPiece(const board *b, attack_model *a, int from, int side, int piece)
{
    a->pos_m[piece+side*BLACKPIECE][++(a->pos_c[piece+side*BLACKPIECE])]=from;
return 1;
}
#endif