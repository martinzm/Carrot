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

#include "movgen.h"
#include "attacks.h"
#include "evaluate.h"
#include "generate.h"
#include "hash.h"
#include "defines.h"
#include "bitmap.h"
#include "tests.h"
#include "utils.h"
#include "globals.h"
#include "search.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MOVE_TEST_SETUP BITVAR mv2=mv
#define MOVE_TEST(x) if(*(move-1)==x) { printf("Move from:%d to:%d triggered file:%s, line:%d\n", from, to, __FILE__, __LINE__ );printmask(mv2,"rook"); printboard(b);  dumpit(b, from); }

BITVAR isInCheck_Eval(board *b, attack_model *a, int side)
{
	return a->ke[side].attackers;
}

int is_quiet_move(board const * const b, attack_model const * const a, move_entry const * const m)
{
	int to;
	int prom;

	to = UnPackTo(m->move);
	prom = UnPackProm(m->move);
//	if ((b->pieces[to] == ER_PIECE) &&  (prom>QUEEN))
	if ((b->pieces[to] == ER_PIECE))
		return 1;
	return 0;
}

/*
 * Serialize moves from bitmaps, capture type of moves available at board for side
 */
 

#define MVSFROM(BO, SI, OSI, FUNC, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = FUNC(BO, FR);\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP) & BO->colormaps[OSI];

#define MVSFROMA(BO, SI, OSI, PIE, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = attack.maps[PIE][FR];\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP) & BO->colormaps[OSI];

#define MVSFROMP(BO, SI, OSI, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = attack.pawn_move[SI][FR];\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP);

#define MVSFROMPA(BO, SI, OSI, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = attack.pawn_att[SI][FR];\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP) & BO->colormaps[OSI];

void mvsfroma2(const board * const b, attack_model *a, int piece, int side, bmv **ii, BITVAR mask, BITVAR lim) {
BITVAR v;
	v = b->maps[piece] & (lim);
	while (v) {
		(*ii)->fr = LastOne(v); 
		(*ii)->pi = piece;
		(*ii)->mr = attack.rays_dir[b->king[side]][(*ii)->fr];
		(*ii)->mm = attack.maps[piece][(*ii)->fr] & mask;
		(*ii)++;
		ClrLO(v);
	}
}

void mvsfromp2(const board *const b, attack_model *a, int side, bmv **ii, BITVAR mask, BITVAR lim) {
BITVAR v;
	v = b->maps[PAWN]&(lim);
	while (v) {
		(*ii)->fr = LastOne(v);
		(*ii)->pi = PAWN;
		(*ii)->mm = attack.pawn_move[side][(*ii)->fr] & mask;
		(*ii)->mr = attack.rays_dir[b->king[side]][(*ii)->fr];
		(*ii)++;
		ClrLO(v);
	}
}

inline void mvsfromp21(const board *const b, attack_model *a, int side, bmv **ii, BITVAR mask, BITVAR lim, BITVAR pins) {
BITVAR v;
	v = b->maps[PAWN]&(lim);
	while (v) {
		(*ii)->fr = LastOne(v);
		(*ii)->pi = PAWN;
//		(*ii)->mm = attack.pawn_move[side][(*ii)->fr] & mask;
		(*ii)->mr = attack.rays_dir[b->king[side]][(*ii)->fr];
		a->mvs[(*ii)->fr] |= (*ii)->mm = ((((pins >> (*ii)->fr)&1)-1)|(*ii)->mr) & attack.pawn_move[side][(*ii)->fr] & mask;
		(*ii)++;
		ClrLO(v);
	}
}

inline void mvsfromk21(const board *const b, attack_model *a, int side, bmv **ii) {
BITVAR v;

	int from = b->king[side];
	v = (attack.maps[KING][from])
		& (~attack.maps[KING][b->king[Flip(side)]])
		& (~a->att_by_side[Flip(side)]);

		(*ii)->fr = from;
		(*ii)->pi = KING;
//		(*ii)->mr = ;
		a->mvs[from] = (*ii)->mm =  v;
		(*ii)++;
}

#define mvsfrompa2(B, A, S, I, M, L) \
 BITVAR v; v=B->maps[PAWN]&L;\
  while(v) { I->fr=LastOne(v);\
		I->pi = PAWN;\
		A->mvs[I->fr] = I->mm = attack.pawn_att[S][I->fr] & M;\
		I->mr = attack.rays_dir[B->king[S]][I->fr];\
		I++;\
		ClrLO(v);\
	}\
};

#define mvsfrompa21(B, A, S, I, M, L, PP) \
{ BITVAR v; v=B->maps[PAWN]&L;\
  while(v) { I->fr=LastOne(v);\
		I->pi = PAWN;\
		I->mr = attack.rays_dir[B->king[S]][I->fr];\
		A->mvs[I->fr] = I->mm = ((((PP >> I->fr)&1)-1)|I->mr) & attack.pawn_att[S][I->fr] & M;\
		I++;\
		ClrLO(v);\
	}\
};

/**** CURRENT bitmap generators ****/
void mvsfroma21(const board * const b, attack_model *a, int piece, int side, bmv **ii, BITVAR mask, BITVAR lim, BITVAR pins) {
BITVAR v;
	v = b->maps[piece] & (lim);
	while (v) {
		(*ii)->fr = LastOne(v); 
		(*ii)->pi = piece;
		a->mvs[(*ii)->fr] = (*ii)->mm = attack.maps[piece][(*ii)->fr] & mask & (((pins >> ((*ii)->fr))&1)-1);
		(*ii)->mr = attack.rays_dir[b->king[side]][(*ii)->fr];
		(*ii)++;
		ClrLO(v);
	}
}

void mvsfroma21N(const board * const b, attack_model *a, int piece, int side, BITVAR mask, BITVAR lim, BITVAR pins) {
BITVAR v;
	v = b->maps[piece] & (lim);
	while (v) {
		int fr = LastOne(v); 
		BITVAR mr = attack.rays_dir[b->king[side]][fr];
		BITVAR mk = attack.maps[piece][fr] & mask;
		a->mvs[fr] = ((((pins >> fr) &1)-1)|mr) & mk;
		a->mvk[fr] = mk;

#if 0
		if(piece==KNIGHT){
		L0("Knight at %d\n", fr);
		printmask(v, "pcs");
		printmask(pins, "pin");
		printmask(mr, "ray");
		printmask(mk, "umsk");
		printmask(a->mvs[fr], "mvs");
		}
#endif
		ClrLO(v);
	}
}

void mvsfromk22(const board *const b, attack_model *a, int side ) {
BITVAR v;

// !!!! att_by_side MUSI byt aktualni !!!!
	int from = b->king[side];
	v = (attack.maps[KING][from])
		& (~attack.maps[KING][b->king[Flip(side)]])
		& (~a->att_by_side[Flip(side)]);

		if (b->castle[side]) {
			int orank = side == WHITE ? 0:7;
			if (b->castle[side] & QUEENSIDE) {
				if ((attack.rays[getPos(2,orank)][getPos(4,orank)]
					& ((a->att_by_side[Flip(side)]
						| attack.maps[KING][b->king[Flip(side)]]))) == 0
					&& ((attack.rays[getPos(1, orank)][getPos(3,orank)] & b->norm) == 0)){
						v |= NORMM(getPos(2, orank));
				}
			}
			if (b->castle[side] & KINGSIDE) {
				if ((attack.rays[getPos(4, orank)][getPos(6,orank)]
					& (a->att_by_side[Flip(side)]
						| attack.maps[KING][b->king[Flip(side)]])) == 0
					&& ((attack.rays[getPos(5,orank)][getPos(6,orank)] & b->norm) == 0)) {
						v |= NORMM(getPos(6, orank));
				}
			}
		}
		a->mvs[from] = v;
}

// << 9 - moves up right
// << 7 - up left
// >> 7 - down right
// >> 9 - down left
// 0 utok vlevo, 1 utok vpravo, 2 posun vpred, 3 doublepush, 4 ep, 5 pot utok vlevo, 6 pot utok vpravo
// rozdelit na pin a nepin
int pawn_set_white(board const *b, king_eval const *ke, BITVAR pins, BITVAR *pset){
BITVAR epbmp, dir, tmp;

// non pins
	BITVAR pwi = b->maps[PAWN] & b->colormaps[WHITE];
	BITVAR pww = pwi & (~pins);
	BITVAR pwb = pwi & pins;
	BITVAR nbn = b->norm & b->colormaps[BLACK];

	pset[6]= (pwi << 9) & 0xfefefefefefefefe;
	pset[5]= (pwi << 7) & 0x7f7f7f7f7f7f7f7f;
	pset[0]= (nbn >> 7) & pww & 0xfefefefefefefefe;
	pset[1]= (nbn >> 9) & pww & 0x7f7f7f7f7f7f7f7f;
	pset[2]= ((~b->norm) >> 8) & pww;
	pset[3]= ((~b->norm) >> 16) & pset[2] & RANK2;
	pset[4]= 0;
	if(b->ep > 0) {
		epbmp =	(b->ep >0 && (ke->ep_block == 0)) ? attack.ep_mask[b->ep]
				&  pww : 0;
		pset[4] = pww & epbmp;

		tmp = NORMM(b->ep+8);
		dir=attack.dirs[b->king[WHITE]][3] | attack.dirs[b->king[WHITE]][7];
		pset[4] |= ((tmp & dir) >> 7) & pwb & 0xfefefefefefefefe  & dir;
		dir=attack.dirs[b->king[WHITE]][5] | attack.dirs[b->king[WHITE]][1];
		pset[4] |= ((tmp & dir) >> 9) & pwb & 0x7f7f7f7f7f7f7f7f & dir;
	}
// pins
	dir=attack.dirs[b->king[WHITE]][3] | attack.dirs[b->king[WHITE]][7];
	pset[0] |= ((nbn & dir) >> 7) & pwb & 0xfefefefefefefefe  & dir;
	dir=attack.dirs[b->king[WHITE]][5] | attack.dirs[b->king[WHITE]][1];
	pset[1] |= ((nbn & dir) >> 9) & pwb & 0x7f7f7f7f7f7f7f7f & dir;
	dir=attack.dirs[b->king[WHITE]][4] | attack.dirs[b->king[WHITE]][0];
	pset[2] |= (((~b->norm) & dir) >> 8) & pwb  & dir;
	pset[3] |= (((~b->norm) & dir) >> 16) & pset[2] & RANK2 & dir;
	return 0;
}

int pawn_set_black(board const *b, king_eval const *ke, BITVAR pins, BITVAR *pset){
BITVAR epbmp, dir, tmp;

	BITVAR pbi = b->maps[PAWN] & b->colormaps[BLACK];
	BITVAR pbb = pbi & (~pins);
	BITVAR pbw = pbi & pins;
	BITVAR nwn = b->norm & b->colormaps[WHITE];

//	BITVAR pi = b->maps[PAWN] & b->colormaps[BLACK];
	pset[6]= (pbi >> 7) & 0xfefefefefefefefe;
	pset[5]= (pbi >> 9) & 0x7f7f7f7f7f7f7f7f;
	pset[0]= (nwn << 9) & pbb & 0xfefefefefefefefe;
	pset[1]= (nwn << 7) & pbb & 0x7f7f7f7f7f7f7f7f;
	pset[2]= ((~b->norm) << 8) & pbb;
	pset[3]= ((~b->norm) << 16) & pset[2] & RANK7;
	pset[4]= 0;
	if(b->ep > 0) {
		epbmp =	(b->ep >0 && (ke->ep_block == 0)) ? attack.ep_mask[b->ep]
				& pbb : 0;
		pset[4] = pbb & epbmp;
		tmp = NORMM(b->ep-8);
		dir=attack.dirs[b->king[BLACK]][1] | attack.dirs[b->king[BLACK]][5];
		pset[4] |= ((tmp & dir) << 9) & pbw & 0xfefefefefefefefe & dir;
		dir=attack.dirs[b->king[BLACK]][3] | attack.dirs[b->king[BLACK]][7];
		pset[4] |= ((tmp & dir) << 7) & pbw & 0x7f7f7f7f7f7f7f7f & dir;
	}
	dir=attack.dirs[b->king[BLACK]][1] | attack.dirs[b->king[BLACK]][5];
	pset[0] |= ((nwn & dir) << 9) & pbw & 0xfefefefefefefefe & dir;
	dir=attack.dirs[b->king[BLACK]][3] | attack.dirs[b->king[BLACK]][7];
	pset[1] |= ((nwn & dir) << 7) & pbw & 0x7f7f7f7f7f7f7f7f & dir;
	dir=attack.dirs[b->king[BLACK]][4] | attack.dirs[b->king[BLACK]][0];
	pset[2] |= (((~b->norm)&dir) << 8) & pbw & dir;
	pset[3] |= (((~b->norm)&dir) << 16) & pset[2] & RANK7 & dir;

	return 0;
}

BITVAR regenerateSQAttacked(const board *const b, attack_model *a, int side){
int opside;
BITVAR att=0, pmap;
	int ptype[] = { QUEEN, ROOK, BISHOP, KNIGHT, PAWN };

	for(int f=0; f<4; f++) {
		int piece = ptype[f];
		pmap = b->maps[piece]& b->colormaps[side];
		while(pmap) {
			int ppos = LastOne(pmap);
			att|=a->mvk[ppos];
			ClrLO(pmap);
		}
	}
#if 0
	if(side==WHITE) {
		BITVAR pi = b->maps[PAWN] & b->colormaps[WHITE];
		att|=(pi << 9) & 0xfefefefefefefefe;
		att|=(pi << 7) & 0x7f7f7f7f7f7f7f7f;
	} else {
		BITVAR pi = b->maps[PAWN] & b->colormaps[BLACK];
		att|=(pi >> 7) & 0xfefefefefefefefe;
		att|=(pi >> 9) & 0x7f7f7f7f7f7f7f7f;
	}
return att|a->ke[Flip(side)].att_vec;
#else 
return att|a->ke[Flip(side)].att_vec| a->pset[side][5]|a->pset[side][6];
#endif

}

int generateBitmaps(const board *const b, attack_model *a, BITVAR upd, int side)
{
BITVAR *pset, pins, t;
unsigned char opside;

	if (side == WHITE) {
		opside = BLACK;
		pset = (a->pset[WHITE]);
	} else {
		opside = WHITE;
		pset = (a->pset[BLACK]);
	}

	pins = ((a->ke[side].cr_pins | a->ke[side].di_pins));
	t = b->colormaps[side] & upd;

// generate bitmaps for all moves 
	MVSFROM21(b, a, QUEEN, side, QueenAttacks, 0, FULLBITMAP, t, pins) ;
	MVSFROM21(b, a, ROOK, side, RookAttacks, 0, FULLBITMAP, t, pins) ;
	MVSFROM21(b, a, BISHOP, side, BishopAttacks, 0, FULLBITMAP, t, pins) ;
//	mvsfroma21N(b, a, KNIGHT, side, FULLBITMAP, t, pins) ;
	mvsfroma21N(b, a, KNIGHT, side, FULLBITMAP, b->colormaps[side], pins) ;
// generate pawn info

//	if(t & b->maps[PAWN]) {
		if(side==WHITE) {
			pawn_set_white(b, &(a->ke[WHITE]), pins, pset);
		} else {
			pawn_set_black(b, &(a->ke[BLACK]), pins, pset);
		}
//	}
return 0;
}

// serialize captures
void generateCapturesN3(const board *const b, attack_model *a, move_entry **m, int gen_u)
{
	int from, to, epn, get_rank;
	int ptype[] = { QUEEN, ROOK, BISHOP, KNIGHT, PAWN };
	BITVAR mv, rank, piece, epbmp, pins, tp, tq, kpin, nmf, tt, pmap;
	bmv mm[64];
	bmv *ipp,*ib,*in,*ir,*iq,*ik,*ii, *ix, *ipc, *ipa;
	
	BITVAR *pset;

	move_entry *move;
	int ep_add;
	unsigned char side, opside;

	move = *m;
	if (b->side == WHITE) {
		rank = RANK7;
		side = WHITE;
		opside = BLACK;
		ep_add = 8;
		get_rank = 1;
		pset = (a->pset[WHITE]);
	} else {
		rank = RANK2;
		opside = WHITE;
		side = BLACK;
		ep_add = -8;
		get_rank = -1;
		pset = (a->pset[BLACK]);
	}
// !!!! bitmap generations should be moved outside
//	generateBitmaps(b, a, b->colormaps[side], b->side);

// generate piece captures

	for(int f=0; f<4; f++) {
		int piece = ptype[f];
		pmap = b->maps[piece]& b->colormaps[side];
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & b->colormaps[opside];
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score =
					b->pers->LVAcap[piece][b->pieces[to] & PIECEMASK];
				move++;
				ClrLO(mv);
			}
		ClrLO(pmap);
		}
	}

// pawn attacks non promoting
		pmap = (pset[0]) & (~rank);
		while(pmap) {
			int ppos = LastOne(pmap);
			to = getPos(getFile(ppos)-1, getRank(ppos)+get_rank);
			move->move = PackMove(ppos, to, ER_PIECE, 0);
			move->qorder = move->real_score =
					b->pers->LVAcap[PAWN][b->pieces[to] & PIECEMASK];
			move++;
			ClrLO(pmap);
		}
		pmap = (pset[1]) & (~rank);
		while(pmap) {
			int ppos = LastOne(pmap);
			to = getPos(getFile(ppos)+1, getRank(ppos)+get_rank);
			move->move = PackMove(ppos, to, ER_PIECE, 0);
			move->qorder = move->real_score =
					b->pers->LVAcap[PAWN][b->pieces[to] & PIECEMASK];
			move++;
			ClrLO(pmap);
		}

// pawn attacks promoting
		if (gen_u != 0) {
			pmap = (pset[0]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)-1, getRank(ppos)+get_rank);
				move->move = PackMove(ppos, to, QUEEN, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
				move++;
				move->move = PackMove(ppos, to, KNIGHT, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
				move++;
//underpromotion
				move->move = PackMove(ppos, to, BISHOP, 0);
				move->qorder = move->real_score = A_OR2;
				move++;
				move->move = PackMove(ppos, to, ROOK, 0);
				move->qorder = move->real_score = A_OR2;
				move++;
				ClrLO(pmap);
			}
			pmap = (pset[1]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)+1, getRank(ppos)+get_rank);
				move->move = PackMove(ppos, to, QUEEN, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
				move++;
				move->move = PackMove(ppos, to, KNIGHT, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
				move++;
//underpromotion
				move->move = PackMove(ppos, to, BISHOP, 0);
				move->qorder = move->real_score = A_OR2;
				move++;
				move->move = PackMove(ppos, to, ROOK, 0);
				move->qorder = move->real_score = A_OR2;
				move++;
				ClrLO(pmap);
			}
// pawn non attack promoting
			pmap = (pset[2]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos), getRank(ppos)+get_rank);
				move->move = PackMove(ppos, to, QUEEN, 0);
				move->qorder = move->real_score = A_QUEEN_PROM;
				move++;
				move->move = PackMove(ppos, to, KNIGHT, 0);
				move->qorder = move->real_score = A_KNIGHT_PROM;
				move++;
// underpromotion
				move->move = PackMove(ppos, to, BISHOP, 0);
				move->qorder = move->real_score = A_MINOR_PROM + B_OR;
				move++;
				move->move = PackMove(ppos, to, ROOK, 0);
				move->qorder = move->real_score = A_MINOR_PROM + R_OR;
				move++;
				ClrLO(pmap);
			}
		} else {
			pmap = (pset[0]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)-1, getRank(ppos)+get_rank);
				move->move = PackMove(ppos, to, QUEEN, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
				move++;
				move->move = PackMove(ppos, to, KNIGHT, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
				move++;
				ClrLO(pmap);
			}
			pmap = (pset[1]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)+1, getRank(ppos)+get_rank);
				move->move = PackMove(ppos, to, QUEEN, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
				move++;
				move->move = PackMove(ppos, to, KNIGHT, 0);
				move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
				move++;
				ClrLO(pmap);
			}
// pawn non attack promoting
			pmap = (pset[2]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos), getRank(ppos)+get_rank);
				move->move = PackMove(ppos, to, QUEEN, 0);
				move->qorder = move->real_score = A_QUEEN_PROM;
				move++;
				move->move = PackMove(ppos, to, KNIGHT, 0);
				move->qorder = move->real_score = A_KNIGHT_PROM;
				move++;
				ClrLO(pmap);
			}
		}

// ep capture
	pmap = (pset[4]);
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(b->ep), getRank(b->ep) + get_rank);
			move->move = PackMove(ppos, to, PAWN,0);
			move->qorder = move->real_score = b->pers->LVAcap[PAWN][PAWN];
		move++;
		ClrLO(pmap);
	}

// !!!! king should be moved into separate function
	from = b->king[side];
	mv = a->mvs[from] & (b->colormaps[opside]);
	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to, ER_PIECE, 0);
		move->qorder = move->real_score =
			b->pers->LVAcap[KING][b->pieces[to] & PIECEMASK];
		move++;
		ClrLO(mv);
	}
	*m = move;
}

// serialize captures
void generateCapturesN2(const board *const b, attack_model *a, move_entry **m, int gen_u)
{
	generateBitmaps(b, a, b->colormaps[b->side], b->side);
//	a->att_by_side[WHITE] = KingAvoidSQAlt(b, a, WHITE);
//	a->att_by_side[BLACK] = KingAvoidSQAlt(b, a, BLACK);
	mvsfromk22(b, a, b->side);
	generateCapturesN3(b, a, m, gen_u);
}

#if 1

#define GETMVSC(BO, FR, PIECE, SI, FUNC, AT, PIN, ALLOW, V, TP, TQ) V=BO->maps[PIECE]&BO->colormaps[SI];\
while (V){ FR=LastOne(V); TQ=NORMM(FR); TP = FUNC(BO, FR);\
AT->mvs[FR] = (PIN & TQ) ? 0 : TP&ALLOW ; ClrLO(V); }

#define GETMVKC(BO, FR, PIECE, SI, AT, PIN, ALLOW, V, TP, TQ) V=BO->maps[PIECE]&BO->colormaps[SI];\
while (V){ FR=LastOne(V); TQ=NORMM(FR); TP = attack.maps[PIECE][FR];\
AT->mvs[FR] = (PIN & TQ) ? 0 : TP&ALLOW ; ClrLO(V); }

#endif

typedef struct _run_in {
	int st;
	int en;
	int add;
	int opside;
	int orank;
} run_in;

run_in RR[] = { { ER_PIECE, PAWN, 0, BLACK, 0 }, { ER_PIECE | BLACKPIECE, PAWN
	| BLACKPIECE, BLACKPIECE, WHITE, 56 } };


/*
 * Serialize moves from bitmaps for all types of moves available at board for side
 */

#define MVSFROMn(BO, SI, OSI, FUNC, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = FUNC(BO, FR) & (~BO->norm);\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP);

#define MVSFROMAn(BO, SI, OSI, PIE, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = attack.maps[PIE][FR] & (~BO->norm);\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP);

#define MVSFROMPn(BO, SI, OSI, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = attack.pawn_move[SI][FR] & (~BO->norm);\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP);

#define MVSFROMPAn(BO, SI, OSI, PIN, RES, FR, V, TP, TQ) \
		FR=LastOne(V); \
		TQ=NORMM(FR); \
		TP = attack.pawn_att[SI][FR] & (~BO->norm);\
		RES = ((PIN & TQ) ? TP&attack.rays_dir[BO->king[SI]][FR] : TP);

// serialize all non captures, moves bitmaps already generated
void generateMovesN2(const board *const b, attack_model *a, move_entry **m)
{
	int from, to, get_rank;
	int ptype[] = { QUEEN, ROOK, BISHOP, KNIGHT, PAWN };
	BITVAR mv, rank, brank, piece, bran2;
	BITVAR epbmp, pins, tp, tq, kpin, nmf, tmp, tmp2, tx, tx2, dir;
	BITVAR *pset, pmap;
	bmv mm[64];
	bmv *ip,*ib,*in,*ir,*iq,*ik,*ii, *ix;

	move_entry *move;
	int orank, ff;
	unsigned char side, opside;

	move = *m;
	if (b->side == WHITE) {
		rank = RANK7;
		side = WHITE;
		opside = BLACK;
		brank = RANK2;
		bran2 = RANK4;
		orank = 0;
		ff = 8;
		get_rank=1;
		pset = (a->pset[WHITE]);
	} else {
		rank = RANK2;
		side = BLACK;
		opside = WHITE;
		brank = RANK7;
		bran2 = RANK5;
		orank = 56;
		ff = -8;
		get_rank=-1;
		pset = (a->pset[BLACK]);
	}

	pins = ((a->ke[side].cr_pins | a->ke[side].di_pins));

	for(int f=0; f<4; f++) {
		int piece = ptype[f];
		pmap = b->maps[piece]& b->colormaps[side];
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}
	}

// pawn push
	pmap = (pset[2])&(~rank);
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(ppos), getRank(ppos)+get_rank);
		move->move = PackMove(ppos, to, ER_PIECE, 0);
		move->qorder = move->real_score = MV_OR + P_OR;
		move++;
		ClrLO(pmap);
	}
// dpush
	pmap = (pset[3]);
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(ppos), getRank(ppos)+2*get_rank);
		move->move = PackMove(ppos, to, ER_PIECE, 0);
		move->qorder = move->real_score = MV_OR + P_OR + 1;
		move++;
		ClrLO(pmap);
	}

// king 
// !!!!! att_by_side - opside !!!!!
	from = b->king[side];

	mv = a->mvs[from] & (~b->norm) & attack.surr1[from];

	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to, ER_PIECE, 0);
		move->qorder = move->real_score = MV_OR;
		move++;
		ClrLO(mv);
	}
//incorporate castling
	mv = a->mvs[from] & (~b->norm) & (~attack.surr1[from]);
	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to, KING, 0);
		move->qorder = move->real_score = (from>to) ? CS_Q_OR:CS_K_OR ;
		move++;
		ClrLO(mv);
	}
	*m = move;
	return;
}

/*
 * tahy ktere vedou na policka, ktera jsou od nepratelskeho krale - krome pinned
 * tahy pinned ktere vedou na ^^ policka a neodkryvaji vlastniho krale
 * tahy figurami, ktere blokuji utok na nepratelskeho krale
 * taky je mozno tahnout vlastnim figurami, ktere blokuji utok na nepratelskeho krale
 */

/*
 * Serialize moves from bitmaps, for quiet/NON capture checking types of moves available at board for side
 */

/*
 * non capture moves causing check
 * ie moving own piece blocking our attack on king
 * or place piece on proper blocker ray
 * or placing knight on proper square
 */

void generateQuietCheckMovesN(const board *const b, attack_model *a, move_entry **m)
{
	int from, to, get_rank, piece;
	int ptype[] = { QUEEN, ROOK, BISHOP, KNIGHT, PAWN };
	int ptypD[] = { QUEEN, BISHOP };
	int ptypC[] = { QUEEN, ROOK };

	BITVAR mv, rank, brank, bran2;
	BITVAR epbmp, pins, tp, tq, kpin, nmf, tmp, tmp2, tx, tx2, dir, opK;
	BITVAR *pset, pmap, opins;
	bmv mm[64];
	bmv *ip,*ib,*in,*ir,*iq,*ik,*ii, *ix;

	move_entry *move;
	int orank, ff;
	unsigned char side, opside;

	move = *m;
	if (b->side == WHITE) {
		rank = RANK7;
		side = WHITE;
		opside = BLACK;
		brank = RANK2;
		bran2 = RANK4;
		orank = 0;
		ff = 8;
		get_rank=1;
		pset = (a->pset[WHITE]);
	} else {
		rank = RANK2;
		side = BLACK;
		opside = WHITE;
		brank = RANK7;
		bran2 = RANK5;
		orank = 56;
		ff = -8;
		get_rank=-1;
		pset = (a->pset[BLACK]);
	}

// protect my king regardless of piece color
	pins = ((a->ke[side].cr_pins | a->ke[side].di_pins));

// my piece protecting opside king from my attack
	opins = ((a->ke[opside].cr_pins | a->ke[opside].di_pins));

// my pin moving away
// diags
	for(int f=0; f<1; f++) {
		piece = ptypD[f];
		pmap = b->maps[piece]& b->colormaps[side] & opins;
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm) & (~a->ke[opside].di_blocker_ray);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}
	}
	for(int f=0; f<1; f++) {
		piece = ptypC[f];
		pmap = b->maps[piece]& b->colormaps[side] & opins;
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm) & (~a->ke[opside].cr_blocker_ray);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}
	}

// my pieces reaching blocker ray positions
	for(int f=0; f<1; f++) {
		piece = ptypD[f];
		pmap = b->maps[piece]& b->colormaps[side] & (~opins);
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm) & (a->ke[opside].di_blocker_ray);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}
	}
	for(int f=0; f<1; f++) {
		piece = ptypC[f];
		pmap = b->maps[piece]& b->colormaps[side] & (~opins);
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm) & (a->ke[opside].cr_blocker_ray);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}
	}
// knight
		piece = KNIGHT;
		pmap = b->maps[piece]& b->colormaps[side] & opins;
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm) & (~a->ke[opside].kn_pot_att_pos);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}
		piece = KNIGHT;
		pmap = b->maps[piece]& b->colormaps[side] & (~opins);
		while(pmap) {
			int ppos = LastOne(pmap);
			mv= a->mvs[ppos] & (~b->norm) & (a->ke[opside].kn_pot_att_pos);
			while (mv) {
				to = LastOne(mv);
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
				move++;
				ClrLO(mv);
			}
			ClrLO(pmap);
		}

// pawn push
	pmap = (pset[2])&(~rank)&opins;
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(ppos), getRank(ppos)+get_rank);
		if(NORMM(to) & (~a->ke[opside].pn_pot_att_pos)) {
			move->move = PackMove(ppos, to, ER_PIECE, 0);
			move->qorder = move->real_score = MV_OR + P_OR;
			move++;
		}
		ClrLO(pmap);
	}
// dpush
	pmap = (pset[3])&opins;
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(ppos), getRank(ppos)+2*get_rank);
		if(NORMM(to) & (~a->ke[opside].pn_pot_att_pos)) {
			move->move = PackMove(ppos, to, ER_PIECE, 0);
			move->qorder = move->real_score = MV_OR + P_OR + 1;
			move++;
		}
		ClrLO(pmap);
	}
	pmap = (pset[2])&(~rank)&(~opins);
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(ppos), getRank(ppos)+get_rank);
		if(NORMM(to) & (a->ke[opside].pn_pot_att_pos)) {
			move->move = PackMove(ppos, to, ER_PIECE, 0);
			move->qorder = move->real_score = MV_OR + P_OR;
			move++;
		}
		ClrLO(pmap);
	}
	pmap = (pset[3])&(~opins);
	while(pmap) {
		int ppos = LastOne(pmap);
		to = getPos(getFile(ppos), getRank(ppos)+2*get_rank);
		if(NORMM(to) & (a->ke[opside].pn_pot_att_pos)) {
			move->move = PackMove(ppos, to, ER_PIECE, 0);
			move->qorder = move->real_score = MV_OR + P_OR + 1;
			move++;
		}
		ClrLO(pmap);
	}

// king 
// !!!!! att_by_side - opside !!!!!
	from = b->king[side];
	if(NORMM(from) & opins) {
		mv = a->mvs[from] & (~b->norm) & attack.surr1[from] &(~((a->ke[opside].cr_blocker_ray)|(a->ke[opside].di_blocker_ray)));
		while (mv) {
			to = LastOne(mv);
			move->move = PackMove(from, to, ER_PIECE, 0);
			move->qorder = move->real_score = MV_OR;
			move++;
			ClrLO(mv);
		}
	}
//incorporate castling ???
	*m = move;
	return;
}

/*
	changes to bitmaps representing moves

	sources of change (leading to regeneration):
	removal at source square
	removal at captured
	insertion at destination (incl promotion)
	removal at rook source in castling
	insertion at rook dest in castling

	removal of previous ep
	insertion of ep

	state of PINNING (ie if piece is pin, change of)
	changes of attacks to squares around king
 */


/*
 * jak generovat jen bitmapy pro zmeny
 * tah je z from do to, 
 * - obcas brani na "to", 
 * - obcas na "to" zmena figury, 
 * - brani jinde nez "to" - ep
 * - obcas pohyb dalsi figury - vez pri rosade
 * Zmeny
 * - vsem kteri pres dane pole utoci - zmena bitmapy
 * - vsem kteri se stali blockery (blokuji utok na krale) -zmena bitmapy, blockers
 * - vsem kteri prestali byt blockery -zmena bitmapy, blockers
 * - vsem kteri zacali byt attackery -zmena attackers
 * - vsem kteri prestali byt attackery -zmena attackers
 * - tomu kdo se hnul -zmena bitmapy, blockers, attackers
 *
 * postup
 * - provest tah samotny - zmeny na sachovnici - MakeMoveNew
 * - identifikovat utocniky na krale a blockery - eval_king_checks_extN
 * - identifikovat zmeny v blockers
 * - identifikovat figury kterych se tykaji zmeny bitmap
 * - update bitmap
 *
 */

/*
  UNDO has
	int8_t side;
	int8_t captured;  //what was captured
	int8_t moved;  // promoted to in case promotion, otherwise the same as old
	int8_t old;  //what was the old piece
	int8_t ep, prev_ep; //state of ep before and after move == 0 => no EP!
	int8_t from, to;
	int8_t fRO, toRO; // if castling, rook move fRO == -1 => not performed
	int8_t whereCa; //where capture took place == -1 => no capture
 */

void getChanges(board *b, int pos, BITVAR v[4], BITVAR w[4], BITVAR s[4], BITVAR *pp){
BITVAR rw,rb, t;

// attack vectors
	v[0]=getnormvector(b->norm, pos);
	v[1]=get90Rvector(b->r90R, pos);
	v[2]=get45Rvector(b->r45R, pos);
	v[3]=get45Lvector(b->r45L, pos);

// vectored attackers
	s[0] = ((v[0]) & (b->maps[ROOK] | b->maps[QUEEN]));
	s[1] = ((v[1]) & (b->maps[ROOK] | b->maps[QUEEN]));
	s[2] = ((v[2]) & (b->maps[BISHOP] | b->maps[QUEEN]));
	s[3] = ((v[3]) & (b->maps[BISHOP] | b->maps[QUEEN]));

// changes to pawn movement and attacks
	*pp  = ((attack.pawn_att[WHITE][pos])|(attack.pawn_move2[WHITE][pos])) & b->maps[PAWN]
		& (b->colormaps[BLACK]);
	*pp |= ((attack.pawn_att[BLACK][pos])|(attack.pawn_move2[BLACK][pos])) & b->maps[PAWN]
		& (b->colormaps[WHITE]);

// doublepush 
	rb  = ((NORMM(pos)&RANK4)>>8)&(~b->norm);
	rw  = ((NORMM(pos)&RANK5)<<8)&(~b->norm);
	*pp |= ((((rb>>8)&b->maps[WHITE]) | ((rw<<8)&b->maps[BLACK])))&b->maps[PAWN];
}

/*
 * identify pieces that needs to update their moves/bitmaps
 *
 * working with board after move, UNDO containing pieces/position before & after move
 */

BITVAR ChangedToN(board *b, attack_model *a, UNDO *u)
{
BITVAR rr, rb, rw;
BITVAR king;
BITVAR d[4];
BITVAR kbo1, kbo2;
BITVAR s[4], o[4];
BITVAR v[5][4]; // from, to, fRO, toRO, whereCa
BITVAR w[5][4];
BITVAR zps[5][4]; // from, to, fRO, toRO, whereCa
BITVAR pps[5];
BITVAR eps, ch, tps;
int ks, ko;

// directly affected
	tps=0;
	getChanges(b, u->from, v[0], w[0], zps[0], &pps[0]);
	getChanges(b, u->to, v[1], w[1], zps[1], &pps[1]);
	ch = NORMM(u->from)|NORMM(u->to);
	tps|=zps[0][0]|zps[0][1]|zps[0][2]|zps[0][3];
	tps|=zps[1][0]|zps[1][1]|zps[1][2]|zps[1][3];

	if((u->whereCa != -1) && (u->whereCa!=u->to)) {
		getChanges(b, u->whereCa, v[4], w[4], zps[4], &pps[4]);
		tps|=zps[4][0]|zps[4][1]|zps[4][2]|zps[4][3]|pps[4];
		ch |= NORMM(u->whereCa);
	}
	if((u->fRO != -1) && (u->toRO!=-1)) {
		getChanges(b, u->fRO, v[2], w[2], zps[2], &pps[2]);
		getChanges(b, u->toRO, v[3], w[3], zps[3], &pps[3]);
		ch |= NORMM(u->fRO);
		ch |= NORMM(u->toRO);
		tps|=zps[2][0]|zps[2][1]|zps[2][2]|zps[2][3];
		tps|=zps[3][0]|zps[3][1]|zps[3][2]|zps[3][3];
	}

// affected by ep
	eps=0;
// get pawns that had ep, opside
	if(u->prev_ep>0) {
		eps|=attack.ep_mask[u->prev_ep] & b->maps[PAWN] & b->colormaps[Flip(u->side)];
	}
// get pawns that have ep, side to move
	if(u->ep>0) {
		eps|=attack.ep_mask[u->ep] & b->maps[PAWN] & b->colormaps[u->side];
	}
	tps|=eps;

// check king
// either something (of its side) moved in its surrounding squares
// or some attack to surrounding squares changed
// directly affected
	if(attack.surr1[b->king[WHITE]] & ch & b->colormaps[WHITE]){
		tps|=NORMM(b->king[WHITE]);
	}
	if(attack.surr1[b->king[BLACK]] & ch & b->colormaps[BLACK]){
		tps|=NORMM(b->king[BLACK]);
	}

return tps|ch;
}

/*
 * build map of pieces attacking king 
 * build map of PINS / BLOCkers , blocking attack at king
 * includes blocking rays - squares from where king can be attacked
 * stores squares attacked by during attack on king/check
 */

int eval_king_checks_extU(board const *b, king_eval *ke, int side, int from)
{
	BITVAR cr2, di2, c2, d2, c, d, c3, d3, c2s, d2s;
	BITVAR rw,rb, t, pin[8], pins;
	BITVAR v[4], w[4], s[4], z[4], x[4], u[8], aa, bb, ps, pz, pzz, pq, pp;
	int dircfr[] = { 2, 6, 0, 4, 1, 5, 3, 7 };
	int dirf[] = { 1, 2, 0, 3, 1, 2, 0, 3 };
	int dil[] = { ROOK, ROOK, BISHOP, BISHOP };

	int ff, o;
	BITVAR epbmp;

	o = Flip(side);
	epbmp = attack.ep_mask[b->ep];
	ke->ep_block = 0;

// attack vectors
	getnormvector2(b->norm, from, &v[0], &w[0]);
	get90Rvector2(b->r90R, from, &v[1], &w[1]);
	get45Rvector2(b->r45R, from, &v[2], &w[2]);
	get45Lvector2(b->r45L, from, &v[3], &w[3]);

// store blocker rays - squares from king can be attacked

	ke->cr_blocker_ray=v[0]|v[1];
	ke->di_blocker_ray=v[2]|v[3];
	ke->att_vec=0;

// and store vectors where king is already attacked
//
	pp=pq=ps=pz=0;
	BITVAR dd = (b->maps[ROOK] | b->maps[QUEEN]) & b->colormaps[o];
	for(int f=0;f<2;f++){
// vectored attackers, vectors
		ps |= (v[f]);
		if(v[f]&dd) ke->att_vec|=(v[f] & ~dd);
// potential distant attackers
		pz |= (w[f] ^ v[f]) & dd;
	}

// a piece at end of vector
	pq |= ps & b->norm;
	ke->cr_attackers = ps & dd;
	pp |= pq ^ ke->cr_attackers;

	ps=0;
	dd = (b->maps[BISHOP] | b->maps[QUEEN]) & b->colormaps[o];
	for(int f=2;f<4;f++){
		ps |= (v[f]);
		if(v[f]&dd) ke->att_vec|=(v[f] & ~dd);
		pz |= (w[f] ^ v[f]) & dd;
	}
	pq |= ps & b->norm;

	pp |= ps ^ ke->di_attackers;
	ke->di_attackers = ps & dd;

	ke->cr_all_ray = attack.maps[ROOK][from];
	ke->di_all_ray = attack.maps[BISHOP][from];

	pins=0;
	while (pz) {
		ff = LastOne(pz);
		pins |= pp & (attack.rays_dir[from][ff]);
		ClrLO(pz);
	}

	ke->cr_pins=pins & ke->cr_all_ray;
	ke->di_pins=pins ^ ke->cr_pins;

	/*
	 * check for ep pin situation - white king on 5th rank, white pawn on the same rank pinned with horizontal attack
	 * and black pawn moved two squares from 7th to 5th. In such case white pawn cannot do ep capture...
	 * pawn was pinned before doublepush, but now is not classified as such
	 */

	if (epbmp!=0) 
	  if ((attack.rays_dir[from][b->ep] & attack.rank[from])!=0) {
		c2 = c2s = (b->maps[ROOK] | b->maps[QUEEN]) & (b->colormaps[o]) & attack.rays_dir[from][b->ep];

		while (c2) {
			ff = LastOne(c2);
			cr2 = attack.rays_int[from][ff];
			c3 = cr2 & b->norm;
			if (((cr2 & c2s) == 0) && (c3 == (c3 & b->maps[PAWN]))) {
					if ((BitCount(c3 & b->maps[PAWN])==2) && ((c3 & (epbmp | normmark[b->ep]) & b->maps[PAWN]) == c3)) ke->ep_block = c3;
			}
			ClrLO(c2);
		}
	}

// incorporate knights
	ke->kn_pot_att_pos = attack.maps[KNIGHT][from];
	ke->kn_attackers = ke->kn_pot_att_pos & b->maps[KNIGHT] & b->colormaps[o];
//inorporate pawns
	ke->pn_pot_att_pos = attack.pawn_att[side][from];
	ke->pn_attackers = ke->pn_pot_att_pos & b->maps[PAWN] & b->colormaps[o];
	ke->attackers = ke->cr_attackers | ke->di_attackers | ke->kn_attackers
		| ke->pn_attackers;

	return 0;
}

int eval_king_checks_extN(board const *b, king_eval *ke, personality const *p, int side, int from){
	return eval_king_checks_extU(b, ke, side, from);
}

/*
 * it identifies (and returns) all pieces on board that must rebuild their move bitmaps
 * as side effect all attackers and pins for both sides are updated
 */

BITVAR ChangesToMove(board *b, attack_model *a, UNDO *u)
{

king_eval kk[2];
BITVAR changed, pin[2], t[2];

	kk[WHITE].cr_pins = a->ke[WHITE].cr_pins;
	kk[WHITE].di_pins = a->ke[WHITE].di_pins;
	kk[BLACK].cr_pins = a->ke[BLACK].cr_pins;
	kk[BLACK].di_pins = a->ke[BLACK].di_pins;

// get attackers/blockers
	eval_king_checks_extU(b, &(a->ke[WHITE]), 0, b->king[WHITE]);
	eval_king_checks_extU(b, &(a->ke[BLACK]), 1, b->king[BLACK]);

// get changes in PINS/BLOCKERS 
	pin[0]=(a->ke[WHITE].cr_pins ^ kk[WHITE].cr_pins) | (a->ke[WHITE].di_pins ^ kk[WHITE].di_pins);
	pin[1]=(a->ke[BLACK].cr_pins ^ kk[BLACK].cr_pins) | (a->ke[BLACK].di_pins ^ kk[BLACK].di_pins);
	changed = pin[WHITE]|pin[BLACK];

// get changes because of move itself
	changed |= ChangedToN(b, a, u);
	return changed;
}

int isMoveValid(board *b, MOVESTORE move, const attack_model *a, int side, tree_store *tree)
{
	int from, to, prom, movp, opside, pside, tot, prank, pfile;
	BITVAR bfrom, bto, m, path, path2, npins;

	king_eval kee;

	from = UnPackFrom(move);
	movp = b->pieces[from];
	if ((movp & PIECEMASK) == ER_PIECE) {
//		L3("No piece %o, %h\n", from, movp);
		return 0;
	}
	bfrom = NORMM(from);
	if (!(bfrom & b->colormaps[side])) {
//		L3("No bpiece %o\n", from);
		return 0;
	}
	to = UnPackTo(move);
	if (from == to) {
		return 0;
	}
	prom = UnPackProm(move);
	bto = NORMM(to);
	if ((bto & b->colormaps[side])) {
//		L3("BCapturing my piece %o, %o\n", from, to);
		return 0;
	}
	if (bto & b->maps[KING]) {
//		L3("King capture %o, %o\n", from, to);
		return 0;
	}
	if (side == BLACK) {
		pside = BLACKPIECE;
		opside = WHITE;
		prank = 7;
	} else {
		pside = 0;
		opside = BLACK;
		prank = 0;
	}
	// handle special moves
	switch (prom) {
	case KING:
	
// castling
		if (((movp & PIECEMASK) != KING) || (from != getPos(E1, prank))) {
			return 0;
		}
		if ((getPos(C1, prank)) == to) {
			if (!(b->castle[side] & QUEENSIDE)) {
				return 0;
			}
			else {
				path = attack.rays_int[from][getPos(A1, prank)];
				path2 = attack.rays[from][getPos(C1, prank)];
			}
		} else if ((getPos(G1, prank)) == to) {
			if (!(b->castle[side] & KINGSIDE)) {
				return 0;
			}
			else {
				path = attack.rays_int[from][getPos(H1, prank)];
				path2 = attack.rays[from][getPos(G1, prank)];
			}
		} else {
			return 0;
		}
		if (path & b->norm) {
			return 0;
		}
		if (path2
			& (a->att_by_side[opside]
				| attack.maps[KING][b->king[opside]])) {
			return 0;
		}
		return 1;
	case PAWN:
// ep
		if (movp != (PAWN | pside))
			return 0;
		if (b->ep <= 0)
			return 0;
		tot = side == WHITE ? getPos(getFile(b->ep),
			getRank(b->ep) + 1) :
			getPos(getFile(b->ep), getRank(b->ep) - 1);
		if (tot != to)
			return 0;
		if ((!(NORMM(b->ep) & b->maps[PAWN]))
			|| (!(NORMM(b->ep) & b->colormaps[opside]))
			|| (b->norm & NORMM(to)))
			return 0;
		if (a->ke[side].ep_block & bfrom)
			return 0;
		npins = ((a->ke[side].cr_pins | a->ke[side].di_pins) & bfrom);
		if (npins)
			if (!(attack.rays_dir[b->king[side]][from] & bto))
				return 0;
		return 1;
		break;

	case ER_PIECE + 1:
// doublepush
		pfile = getFile(from);
		prank = side == WHITE ? getRank(from) + 2 : getRank(from) - 2;
		tot = getPos(pfile, prank);
		if (tot != to)
			return 0;
		path = attack.rays[from][to] & (~bfrom);
		if (path & b->norm)
			return 0;
//			return 1;
		break;
	case ER_PIECE:
// ordinary movement
		tot = getRank(to);
		if ((movp == (PAWN | pside))
			&& (((side == WHITE) && (tot == 7))
				|| ((side == BLACK) && (tot == 0))))
			return 0;
		break;
// pawn promotion ie for prom == KNIGHT, BISHOP, ROOK, QUEEN
	default:
		if (movp != (PAWN | pside))
			return 0;
		tot = getRank(to);
		if (((side == WHITE) && (tot != 7))
			|| ((side == BLACK) && (tot != 0)))
			return 0;
		break;
	}
	m = 0;

	switch (movp & PIECEMASK) {
	case BISHOP:
		path = attack.rays[from][to];
		m = attack.maps[BISHOP][from];
		break;
	case QUEEN:
		path = attack.rays[from][to];
		m = attack.maps[BISHOP][from] | attack.maps[ROOK][from];
		break;
	case ROOK:
		path = attack.rays[from][to];
		m = attack.maps[ROOK][from];
		break;
	case KING:
		m = attack.maps[KING][from];
		path = attack.rays[from][to];
		eval_king_checks_oth(b, &kee, NULL, side, to);
		if (((kee.attackers) & (~bto)) != 0) {
//			L3("Kee attackers %o\n", from);
			return 0;
		}
		m &= ~attack.maps[KING][b->king[opside]];
		break;
	case KNIGHT:
		m = attack.maps[KNIGHT][from];
		path = 0;
		break;
	case PAWN:
		m = attack.pawn_move[side][from] & (~b->norm);
		m |= (attack.pawn_att[side][from] & (b->colormaps[opside]));
		path = attack.rays[from][to];
		break;
	default:
		return 0;
		break;
	}
	if (!(m & bto)) {
//		L3("Entering prohibited square %o, %o\n", from, to);
		return 0;
	}

	if (path & (~(bfrom | bto)) & b->norm) {
		L3("Something blocking path %o, %o\n", from, to);
		return 0;
	}
// handle pins
	npins = ((a->ke[side].cr_pins | a->ke[side].di_pins) & bfrom);
	if (npins) {
		m &= attack.rays_dir[b->king[side]][from];
		if (!(m & bto)) {
//			L3("Pinned %o, %o\n", from, to);
//			printmask(a->ke[side].cr_pins, "CR");
//			printmask(a->ke[side].di_pins, "DI");
//			printmask(attack.rays_dir[b->king[side]][from], "rays");
			return 0;
		}
	}

	return 1;
}

/*
 Make proposed move and update board information
 - key (hash)
 - bitboards
 - ep
 - sideToMove
 - rule50move
 - move
 - castling
 -
 - material[ER_SIDE][ER_PIECE] ???
 - mcount[ER_SIDE] ???
 - king[ER_SIDE] ??????
 - positions[102] ???
 - posnorm[102] ???
 - gamestage ???

 Store information for UNDO the move

 */

int MakeMoveNew(board *b, MOVESTORE move, int *pos, UNDO *ret)
{
//	UNDO ret;
//	CHANGE CCC, *cha;
	int8_t from;
	int8_t to;
	int8_t prom;
	int8_t opside;
	int8_t siderooks, opsiderooks, kingbase;
	int8_t oldp, movp, capp;
	int *tmidx;
	int *omidx;

	int midx;
	int sidx, oidx;
	int rookf, rookt;
	personality *p;
	char b2[256];
//	cha = &CCC;

	int vcheck = 0;
//	BITVAR changed;

//	boardCheck(b, "beforemove");

//	printBoardNice(b);
//	sprintfMoveSimple(move, b2);
//	L0("moveX %s\n", b2);
	if (b->side == WHITE) {
		opside = BLACK;
		siderooks = A1;
		opsiderooks = A8;
		kingbase = E1;
		tmidx = MATIdxIncW;
		omidx = MATIdxIncB;
		sidx = 1;
		oidx = -1;
	} else {
		opside = WHITE;
		siderooks = A8;
		opsiderooks = A1;
		kingbase = E8;
		tmidx = MATIdxIncB;
		omidx = MATIdxIncW;
		sidx = -1;
		oidx = 1;
	}

	ret->move = move;
	ret->side = b->side;
	ret->prev_castle[WHITE] = b->castle[WHITE];
	ret->prev_castle[BLACK] = b->castle[BLACK];

	ret->rule50move = b->rule50move;
	ret->prev_ep  = b->ep;
	ret->prev_mindex= b->mindex;
	ret->ep = b->ep = 0;
	ret->captured=ER_PIECE;

	ret->key = b->key;
	ret->pawnkey = b->pawnkey;
	ret->mindex_validity = b->mindex_validity;
	ret->psq_b = b->psq_b;
	ret->psq_e = b->psq_e;

	ret->from = from = UnPackFrom(move);
	ret->to = to = UnPackTo(move);
	ret->whereCa = ret->fRO = -1;

	ret->old = movp = oldp = b->pieces[from] & PIECEMASK;

	p = b->pers;
	prom = UnPackProm(move);
	capp = b->pieces[to] & PIECEMASK;
/*
	prom encodes promotion to piece and some special moves
	ER_PIECE - no promotion
	ER-PIECE+1 - doublepush
	PAWN - ep move
	KING - castling
 */
 
/* 
	handle capture part of move 
 */

	if((capp != ER_PIECE || prom == PAWN)) {
		if(prom == PAWN) {
			ret->whereCa = ret->prev_ep;
			capp = PAWN;
		} else {
			ret->whereCa = to;
		}
		ClearAll(ret->whereCa, opside, capp, b);
		ret->captured = capp;
		b->rule50move = b->move;
		midx = omidx[capp];

/*
  deal with PSQ update 
  psq is not side relative
  so white is positive, black negative
 */

		b->psq_b -= (oidx * p->piecetosquare[MG][opside][capp][to]);
		b->psq_e -= (oidx * p->piecetosquare[EG][opside][capp][to]);
			
/*
	deal with specifics related to type of captured piece
 */			
		switch (capp) {
			case BISHOP:
//				if (NORMM(to) & BLACKBITMAP) midx = omidx[DBISHOP];
				midx = (NORMM(to) & BLACKBITMAP) ? omidx[DBISHOP]:0;
				break;
			case PAWN:
				b->pawnkey ^= randomTable[opside][to][PAWN];  //pawnhash
				break;
			case ROOK:
				if ((to == opsiderooks)	&& (b->castle[opside] != NOCASTLE)) {
					b->castle[opside] &= (~QUEENSIDE);
					if (b->castle[opside] != ret->prev_castle[opside]) b->key ^= castleKey[opside][QUEENSIDE];
				} else if ((to == (opsiderooks + 7)) && (b->castle[opside] != NOCASTLE)) {
					b->castle[opside] &= (~KINGSIDE);
					if (b->castle[opside] != ret->prev_castle[opside]) b->key ^= castleKey[opside][KINGSIDE];
				}
				break;
			default:
				break;
		}
/*
	deal with material index update
 */		
		b->mindex -= midx;
		b->key ^= randomTable[opside][to][capp];
		if (b->mindex_validity == 0) vcheck = 1;
	}

/*
	deal with special moves / castling / promotion
 */
// move part of move. both capture and noncapture
// pawn movement/special treatment needed ?
	switch(oldp) {
		case PAWN:
			b->rule50move = b->move;
// was it 2 rows ?
			if (((to > from) ? to - from : from - to) == 16) b->ep = to;
			b->pawnkey ^= randomTable[b->side][from][PAWN];  //pawnhash
			b->pawnkey ^= randomTable[b->side][to][PAWN];  //pawnhash
			break;
// king moved
		case KING:
			b->king[b->side] = to;
			if ((from == kingbase) && (b->castle[b->side] != NOCASTLE)) {
				b->castle[b->side] = NOCASTLE;
				b->key ^= castleKey[b->side][ret->prev_castle[b->side]];
			}
			break;
		case ROOK:
// rook move side screwed castle ?
// 	was the move from my corners ?
			if((b->castle[b->side] != NOCASTLE) && (from == siderooks || from ==(siderooks+7))) {
				int8_t ss;
				ss = (from == siderooks) ? ~QUEENSIDE : ~KINGSIDE;
				b->castle[b->side] &= ss;
				if (b->castle[b->side] != ret->prev_castle[b->side]) b->key ^= castleKey[b->side][~ss];
			}
			break;
		default:
			break;
	}
	
/*
  deal with promotion + board representation update
 */

	switch (prom) {

		case KING:
				b->castle[b->side] = NOCASTLE;
				b->key ^= castleKey[b->side][ret->prev_castle[b->side]];
				if (to > from) {
// kingside castling
					ret->fRO  = from + 3;
					ret->toRO = to - 1;
				} else {
					ret->fRO  = from - 4;
					ret->toRO = to + 1;
				}
// update rook movement
				MoveFromTo(ret->fRO, ret->toRO, b->side, ROOK, b);

				b->key ^= randomTable[b->side][ret->fRO][ROOK];  //hash
				b->key ^= randomTable[b->side][ret->toRO][ROOK];  //hash

				b->psq_b -= (sidx * p->piecetosquare[MG][b->side][ROOK][ret->fRO]);
				b->psq_e -= (sidx * p->piecetosquare[EG][b->side][ROOK][ret->fRO]);
				b->psq_b += (sidx * p->piecetosquare[MG][b->side][ROOK][ret->toRO]);
				b->psq_e += (sidx * p->piecetosquare[EG][b->side][ROOK][ret->toRO]);
		case ER_PIECE:
		case ER_PIECE+1:
		case PAWN:
			MoveFromTo(from, to, b->side, oldp, b);
			break;
		default:		
			b->pawnkey ^= randomTable[b->side][from][PAWN];  //pawnhash
			movp = prom;
			b->rule50move = b->move;
			b->mindex -= tmidx[PAWN];
// fix for dark bishop
			midx = ((prom == BISHOP) && (NORMM(to) & BLACKBITMAP)) ? tmidx[DBISHOP] : tmidx[prom];
			b->mindex += midx;
// check validity of mindex and ev. fix it
			if (b->mindex_validity != 0) vcheck = 1;
			ClearAll(from, b->side, oldp, b);
			SetAll(to, b->side, movp, b);
			break;
	}

/*
	update board representation with move
 */
 
/*
	update PSQ values
 */
	b->psq_b -= (sidx * p->piecetosquare[MG][b->side][oldp][from]);
	b->psq_e -= (sidx * p->piecetosquare[EG][b->side][oldp][from]);
	b->psq_b += (sidx * p->piecetosquare[MG][b->side][movp][to]);
	b->psq_e += (sidx * p->piecetosquare[EG][b->side][movp][to]);

	/* change HASH:
	 - update target
	 - restore source
	 - remove ep - set to NO
		- if there is no ep ret.ep is set 0
		- which has epKey set to 0 - so it makes no change to hash
	 - set ep
	 - change side
	 - set castling to proper state
	 - update 50key and 50position restoration info
	 */

	b->key ^= epKey[ret->prev_ep];
	b->key ^= randomTable[b->side][from][oldp];
	b->key ^= randomTable[b->side][to][movp];
	b->key ^= sideKey;
	b->key ^= epKey[b->ep];

	if (vcheck)
		check_mindex_validity(b, 1);
	b->move++;
	b->positions[b->move - b->move_start] = b->key;
	b->posnorm[b->move - b->move_start] = b->norm;
	b->side = opside;

/*
	store new castling state
 */
	ret->castle[WHITE] = b->castle[WHITE];
	ret->castle[BLACK] = b->castle[BLACK];
	ret->moved = movp;
	
//	boardCheck(b, "aftermove");
	return 0;
}

int MakeNullMove(board *b, UNDO *ret)
{
//	UNDO ret;
	int8_t opside;

	opside = Flip(b->side);

	ret->move = NULL_MOVE;
	ret->side = b->side;
//	ret->prev_castle[WHITE] = b->castle[WHITE];
//	ret->prev_castle[BLACK] = b->castle[BLACK];
	ret->rule50move = b->rule50move;
	ret->prev_ep = b->ep;
	ret->key = b->key;
	ret->pawnkey = b->pawnkey;
	ret->mindex_validity = b->mindex_validity;

	b->key ^= epKey[b->ep];
	ret->ep = b->ep = 0;
	b->key ^= sideKey;  //hash
	b->rule50move = b->move;
	b->move++;
	b->positions[b->move - b->move_start] = b->key;
	b->posnorm[b->move - b->move_start] = b->norm;
	b->side = opside;
	return 0;
}

void UnMakeNullMove(board *b, UNDO *u)
{
	b->ep = u->prev_ep;
	b->move--;
	b->rule50move = u->rule50move;
//	b->castle[WHITE] = u->prev_castle[WHITE];
//	b->castle[BLACK] = u->prev_castle[BLACK];
	b->side = u->side;
	b->key = u->key;
	b->mindex_validity = u->mindex_validity;
}

int MakeMove(board *b, MOVESTORE move, UNDO *u){
int pos[4];
	MakeMoveNew(b, move, pos, u);
	return 0;
}

void UnMakeMoveNew(board *b, UNDO *u, int *pos)
{
	int8_t from, to, prom;
	int rookf, rookt;

	b->mindex_validity = u->mindex_validity;
	b->mindex = u->prev_mindex;
	b->ep = u->prev_ep;
	b->move--;
	b->rule50move = u->rule50move;
	b->castle[WHITE] = u->prev_castle[WHITE];
	b->castle[BLACK] = u->prev_castle[BLACK];

	if (u->moved != u->old) {
		ClearAll(u->to, u->side, u->moved, b);
		SetAll(u->from, u->side, u->old, b);
	} else
		MoveFromTo(u->to, u->from, u->side, u->old, b);  //moving actually backwards

	if (u->whereCa != -1) {
// including EP
		SetAll(u->whereCa, b->side, u->captured, b);
	}
	if (u->old == KING) {
		b->king[u->side] = u->from;
// castling restore rook position
		if(u->fRO != -1) 
			MoveFromTo(u->toRO, u->fRO, u->side, ROOK, b);
	}
	b->side = u->side;
	b->key = u->key;
	b->pawnkey = u->pawnkey;
	b->psq_b = u->psq_b;
	b->psq_e = u->psq_e;
}

void UnMakeMove(board *b, UNDO *u){
int pos[4];
	UnMakeMoveNew(b, u, pos);
}

void generateInCheckMovesN2(const board *const b, attack_model *a, move_entry **m, int gen_u)
{
	int from, to, ff, orank;
	BITVAR mv, rank, brank, bran2, piece, epbmp, pins, tmp, tmp1, tmp2, tmp3, tx2, nmf, kpin, tx, x, all, pmap;
	BITVAR *pset, attacker;
	move_entry *move, *mi;
	int ep_add, epn;
	unsigned char side, opside;
	bmv mm[64];
	bmv *ipa,*ib,*in,*ir,*iq,*ik,*ii, *ix, *ipc, *ipp;
	int ptype[] = { QUEEN, ROOK, BISHOP, KNIGHT, PAWN };
	int get_rank;

	move = *m;
	if (b->side == WHITE) {
		rank = RANK7;
		side = WHITE;
		opside = BLACK;
		brank = RANK2;
		bran2 = RANK4;
		orank = 0;
		ff = 8;
		ep_add = 8;
		pset = (a->pset[WHITE]);
		get_rank=1;
	} else {
		rank = RANK2;
		opside = WHITE;
		side = BLACK;
		brank = RANK7;
		bran2 = RANK5;
		orank = 56;
		ff = -8;
		ep_add = -8;
		pset = (a->pset[BLACK]);
		get_rank=-1;
	}

	pins = ((a->ke[side].cr_pins | a->ke[side].di_pins));

	if (BitCount(a->ke[side].attackers) == 1) {
		attacker = a->ke[side].attackers;
		int att = LastOne(attacker);
		all = (attack.rays_int[b->king[side]][att]);
// capture single attacker

		for(int f=0; f<4; f++) {
			int piece = ptype[f];
			pmap = b->maps[piece]& b->colormaps[side];
			while(pmap) {
				int ppos = LastOne(pmap);
				mv= a->mvs[ppos] & b->colormaps[opside] & attacker;
				while (mv) {
					to = LastOne(mv);
					move->move = PackMove(ppos, to, ER_PIECE, 0);
					move->qorder = move->real_score =
						b->pers->LVAcap[piece][b->pieces[to] & PIECEMASK];
					move++;
					ClrLO(mv);
				}
			ClrLO(pmap);
			}
		}

// pawn attacks non promoting
		pmap = (pset[0]) & (~rank);
		while(pmap) {
			int ppos = LastOne(pmap);
			to = getPos(getFile(ppos)-1, getRank(ppos)+get_rank);
			if(attacker & NORMM(to)) {
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score =
						b->pers->LVAcap[PAWN][b->pieces[to] & PIECEMASK];
				move++;
			}
			ClrLO(pmap);
		}
		pmap = (pset[1]) & (~rank);
		while(pmap) {
			int ppos = LastOne(pmap);
			to = getPos(getFile(ppos)+1, getRank(ppos)+get_rank);
			if(attacker & NORMM(to)) {
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score =
						b->pers->LVAcap[PAWN][b->pieces[to] & PIECEMASK];
				move++;
			}
			ClrLO(pmap);
		}

// pawn attacks promoting
		if (gen_u != 0) {
			pmap = (pset[0]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)-1, getRank(ppos)+get_rank);
				if(attacker & NORMM(to)) {
					move->move = PackMove(ppos, to, QUEEN, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
					move++;
					move->move = PackMove(ppos, to, KNIGHT, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
					move++;
//underpromotion
					move->move = PackMove(ppos, to, BISHOP, 0);
					move->qorder = move->real_score = A_OR2;
					move++;
					move->move = PackMove(ppos, to, ROOK, 0);
					move->qorder = move->real_score = A_OR2;
					move++;
				}
				ClrLO(pmap);
			}
			pmap = (pset[1]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)+1, getRank(ppos)+get_rank);
				if(attacker & NORMM(to)) {
					move->move = PackMove(ppos, to, QUEEN, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
					move++;
					move->move = PackMove(ppos, to, KNIGHT, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
					move++;
//underpromotion
					move->move = PackMove(ppos, to, BISHOP, 0);
					move->qorder = move->real_score = A_OR2;
					move++;
					move->move = PackMove(ppos, to, ROOK, 0);
					move->qorder = move->real_score = A_OR2;
					move++;
				}
				ClrLO(pmap);
			}
		} else {
			pmap = (pset[0]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)-1, getRank(ppos)+get_rank);
				if(attacker & NORMM(to)) {
					move->move = PackMove(ppos, to, QUEEN, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
					move++;
					move->move = PackMove(ppos, to, KNIGHT, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
					move++;
				}
				ClrLO(pmap);
			}
			pmap = (pset[1]) & (rank);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos)+1, getRank(ppos)+get_rank);
				if(attacker & NORMM(to)) {
					move->move = PackMove(ppos, to, QUEEN, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 1][b->pieces[to] & PIECEMASK];
					move++;
					move->move = PackMove(ppos, to, KNIGHT, 0);
					move->qorder = move->real_score = b->pers->LVAcap[KING + 2][b->pieces[to] & PIECEMASK];
					move++;
				}
				ClrLO(pmap);
			}
		}

// ep capture
		pmap = (pset[4]);
		while(pmap) {
			int ppos = LastOne(pmap);
			if(attacker & NORMM(b->ep)) {
				to = getPos(getFile(b->ep), getRank(b->ep) + get_rank);
				move->move = PackMove(ppos, to, PAWN,0);
				move->qorder = move->real_score = b->pers->LVAcap[PAWN][PAWN];
				move++;
			}
			ClrLO(pmap);
		}


// block attack from single attacker
		for(int f=0; f<4; f++) {
			int piece = ptype[f];
			pmap = b->maps[piece]& b->colormaps[side] & (~pins);
			while(pmap) {
				int ppos = LastOne(pmap);
				mv= a->mvs[ppos] & (~b->norm) & all;
				while (mv) {
					to = LastOne(mv);
					move->move = PackMove(ppos, to, ER_PIECE, 0);
					move->qorder = move->real_score = b->pers->LVAcap[piece][ER_PIECE];
					move++;
					ClrLO(mv);
				}
				ClrLO(pmap);
			}
		}

// pawn push
		pmap = (pset[2])&(~rank) & (~pins);
		while(pmap) {
			int ppos = LastOne(pmap);
			to = getPos(getFile(ppos), getRank(ppos)+get_rank);
			if(NORMM(to) & all) {
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = MV_OR + P_OR;
				move++;
			}
			ClrLO(pmap);
		}
// dpush
		pmap = (pset[3]) & (~pins);
		while(pmap) {
			int ppos = LastOne(pmap);
			to = getPos(getFile(ppos), getRank(ppos)+2*get_rank);
			if(NORMM(to) & all) {
				move->move = PackMove(ppos, to, ER_PIECE, 0);
				move->qorder = move->real_score = MV_OR + P_OR + 1;
				move++;
			}
			ClrLO(pmap);
		}

// pawn non attack promoting
		if (gen_u != 0) {
			pmap = (pset[2]) & (rank) & (~pins);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos), getRank(ppos)+get_rank);
				if(NORMM(to) & all) {
					move->move = PackMove(ppos, to, QUEEN, 0);
					move->qorder = move->real_score = A_QUEEN_PROM;
					move++;
					move->move = PackMove(ppos, to, KNIGHT, 0);
					move->qorder = move->real_score = A_KNIGHT_PROM;
					move++;
// underpromotion
					move->move = PackMove(ppos, to, BISHOP, 0);
					move->qorder = move->real_score = A_MINOR_PROM + B_OR;
					move++;
					move->move = PackMove(ppos, to, ROOK, 0);
					move->qorder = move->real_score = A_MINOR_PROM + R_OR;
					move++;
				}
				ClrLO(pmap);
			}
		} else {
// pawn non attack promoting
			pmap = (pset[2]) & (rank) & (~pins);
			while(pmap) {
				int ppos = LastOne(pmap);
				to = getPos(getFile(ppos), getRank(ppos)+get_rank);
				if(NORMM(to) & all) {
					move->move = PackMove(ppos, to, QUEEN, 0);
					move->qorder = move->real_score = A_QUEEN_PROM;
					move++;
					move->move = PackMove(ppos, to, KNIGHT, 0);
					move->qorder = move->real_score = A_KNIGHT_PROM;
					move++;
				}
				ClrLO(pmap);
			}
		}
	}
// move king out of check
	from = b->king[side];
	mv = a->mvs[from] & (b->colormaps[opside]);
	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to, ER_PIECE, 0);
		move->qorder = move->real_score =
			b->pers->LVAcap[KING][b->pieces[to] & PIECEMASK];
		move++;
		ClrLO(mv);
	}
	mv = a->mvs[from] & (~b->norm) & attack.surr1[from];
	while (mv) {
		to = LastOne(mv);
		move->move = PackMove(from, to, ER_PIECE, 0);
		move->qorder = move->real_score = MV_OR;
		move++;
		ClrLO(mv);
	}
	*m = move;
}


void generateInCheckMovesN(const board *const b, attack_model *a, move_entry **m, int gen_u)
{
	generateBitmaps(b, a, b->colormaps[b->side], b->side);
	mvsfromk22(b, a, b->side);
	generateInCheckMovesN2(b, a, m, gen_u);
}

int alternateMovGen(board *b, MOVESTORE *filter)
{

//fixme all!!!
	int i, f, n, tc, cc, th, f2, t2, piece, ff, prom;
	int t2t;
	move_entry mm[300], *m;
	attack_model *a, aa;
	char b2[512], b3[512];

// is side to move in check ?

// fix filter
	/*
	 * prom field
	 * 		PAWN means EP
	 * 		KING means Castling
	 *		ER_PIECE+1 means DoublePush
	 * 		 fix the prom field!
	 */
		th = filter[0];
		f2 = UnPackFrom(th);
		t2 = UnPackTo(th);
		piece = b->pieces[f2];
		prom=UnPackProm(th);
		switch (piece & PIECEMASK) {
		case KING:
			if ((f2 == (E1 + b->side * 56))
				&& ((t2 == (A1 + b->side * 56))
					|| (t2 == (C1 + b->side * 56)))) {
				t2 = (C1 + b->side * 56);
				prom=KING;
			} else if ((f2 == (E1 + b->side * 56))
				&& ((t2 == (H1 + b->side * 56))
					|| (t2 == (G1 + b->side * 56)))) {
				t2 = (G1 + b->side * 56);
				prom=KING;
			}
			break;
		case PAWN:
// test for EP
// b->ep points to target if EP is available
			t2t = t2;
			if (b->side == WHITE)
				t2t -= 8;
			else
				t2t += 8;
			if ((b->ep > 0) && (b->ep == t2t)
				&& (getFile(t2) == getFile(t2t))) {
				prom=PAWN;
			} else {
				ff = (b->side == WHITE) ? t2 - f2 : f2 - t2;
				if (ff == 16) {
					prom=ER_PIECE+1;
				} else {
				}
			}
			break;
		case ER_PIECE:
			printBoardNice(b);
			sprintfMove(b, *filter, b2);
			LOGGER_0("no piece at FROM %s\n", b2);
			abort();
			break;
		}
		th = PackMove(f2, t2, prom, 0);
		filter[0] = th;
	return 1;
}

//+PSQSearch(from, to, KNIGHT, side, a->phase, p)

/*
 * Sorts moves, they should be stored in order generated
 * it should resort captures to MVVLVA 
 * "bad" captures are rechecked via SEE - triggered in sorting
 * noncaptures sorted with HHeuristics - triggered in sorting
 */

// mv->lastp-1 - posledni element
// mv->next - prvni element
void SelectBestO(move_cont *mv)
{
	move_entry *t, a1, *j;
	j = mv->next;
	t=j+1;
	while (t < mv->lastp) {
		a1 = *t;
		while ((j >= mv->next) && (j->qorder < a1.qorder)) {
			*(j + 1) = *j;
			j--;
		}
		*(j + 1) = a1;
		j=t;
		t++;
	}
}

// mv->next points to move to be selected/played
// mv->lastp points behind generated moves

void SelectBest(move_cont *mv)
{
	move_entry *t, a1;

	for (t = mv->lastp - 1; t > (mv->next); t--) {
		if (t->qorder > (t - 1)->qorder) {
			a1 = *(t - 1);
			*(t - 1) = *t;
			*t = a1;
		}
	}
}

void ScoreNormal(board *b, move_cont *mv, int side)
{
	move_entry *t;
	int fromPos, ToPos, piece, opside, dist;

	opside = Flip(side);
	for (t = mv->lastp - 1; t > mv->next; t--) {
		fromPos = UnPackFrom(t->move);
		ToPos = UnPackTo(t->move);
		piece = b->pieces[fromPos] & PIECEMASK;
		t->qorder = checkHHTable(b->hht, side, piece, ToPos) + 20;
//		L0("HH table:%d\n", t->qorder);
// assign priority based on distance to enemy king or promotion
#if 1
		if (piece == PAWN) {
			dist = side == WHITE ? 7 - getRank(ToPos) :
				getRank(ToPos);
			t->qorder += (7 - dist)*3;
		} 
		else {
			dist = attack.distance[ToPos][b->king[opside]];
			t->qorder += (7 - dist)*3;
		}
#endif
//		L0("HH after:%d\n", t->qorder);
	}
}

int ExcludeMove(move_cont *mv, MOVESTORE mm)
{
	move_entry *t;

	t = mv->excl;
	while (t < mv->exclp) {
		if (t->move == mm) {
			return 1;
		}
		t++;
	}
	return 0;
}

void invalidDump(board *b, MOVESTORE m, int side)
{
	char bb[256];
	printBoardNice(b);
	sprintfMoveSimple(m, bb);
	LOGGER_1("failed move %s\n",bb);
	printboard(b);
	return;
}

int getNextMove(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree)
{
	MOVESTORE pot;
	int r;
	move_entry *m;
	
	switch (mv->phase) {
	case INIT:
		// setup everything
		mv->lastp = mv->move;
		mv->next = mv->lastp;
		mv->badp = mv->bad;
		mv->exclp = mv->excl;
		mv->count = 0;
		mv->phase = PVLINE;
		mv->quiet = NULL;
// previous PV move
	case PVLINE:
		mv->phase = HASHMOVE;
	case HASHMOVE:
		mv->phase = GENERATE_CAPTURES;
		if ((mv->hash.move != DRAW_M) && (b->hs != NULL)
			&& isMoveValid(b, mv->hash.move, a, side, tree)
			&& (!ExcludeMove(mv, mv->hash.move))) {
			mv->next->move = mv->hash.move;
			*(mv->exclp) = *(mv->next);
			mv->next->phase=HASHMOVE;
			*mm = mv->next;
			mv->next->ord=mv->count;
			mv->next++;
			mv->exclp++;
			mv->lastp=mv->next;
			return ++mv->count;
		}
	case GENERATE_CAPTURES:
		mv->phase = CAPTUREA;
//		mv->next = mv->lastp;
		if (incheck == 1) {
			generateInCheckMovesN(b, a, &(mv->lastp), 1);
			mv->quiet=mv->lastp;
			SelectBestO(mv);
			DEB_S2(m=mv->lastp-1;for(;m>=mv->next; m--) m->state=0; )
			m=mv->lastp-1;
			for(;m>=mv->next; m--) if(!is_quiet_move(b, a, m)) break; else m->phase=NORMAL;
			if(m>=mv->next && m>mv->lastp-1) mv->quiet=m+1;
			goto rest_moves;
		}
		generateCapturesN2(b, a, &(mv->lastp), 1);
		DEB_S2(move_entry *m=mv->lastp-1; for(;m>=mv->next; m--) m->state=0; )
		mv->tcnt = 95;
//		mv->actph = CAPTUREA;
	case CAPTUREA:
		while ((mv->next < mv->lastp) && (mv->tcnt > 0)) {
			mv->tcnt--;
			SelectBest(mv);
			if (isMoveValid(b, mv->next->move, a, side, tree))
			  if (((mv->next->qorder < A_OR2_MAX)
				&& (mv->next->qorder > A_OR2))
				&& (SEEx(b, mv->next->move) < 0)) {
				mv->next->phase=OTHER;
				*(mv->badp) = *(mv->next);
				mv->badp++;
				mv->next++;
				continue;
			}
			*mm = mv->next;
			mv->next->phase=CAPTUREA;
			mv->next->ord=mv->count;
			mv->next++;
			return ++mv->count;
		}
		mv->phase = SORT_CAPTURES;
	case SORT_CAPTURES:
		if(mv->next < mv->lastp) {
			SelectBestO(mv);
		}
//		mv->actph = CAPTURES;
		mv->phase = CAPTURES;
	case CAPTURES:
		while (mv->next < mv->lastp) {
			if (isMoveValid(b, mv->next->move, a, side, tree))
			  if (((mv->next->qorder < A_OR2_MAX)
				&& (mv->next->qorder > A_OR2))
				&& (SEEx(b, mv->next->move) < 0)) {
				mv->next->phase=OTHER;
				*(mv->badp) = *(mv->next);
				mv->badp++;
				mv->next++;
				continue;
			}
			*mm = mv->next;
			mv->next->phase=CAPTURES;
			mv->next->ord=mv->count;
			mv->next++;
			return ++mv->count;
		}
		mv->phase = KILLER1;
	case KILLER1:
		mv->phase = KILLER2;
		if ((b->pers->use_killer >= 1)) {
			r = get_killer_move(b->kmove, ply, 0, &pot);
			if (r && isMoveValid(b, pot, a, side, tree)
				&& (!ExcludeMove(mv, pot))) {
				mv->lastp->move = pot;
//				mv->actph = KILLER1;
				mv->lastp->phase=KILLER1;
				*(mv->exclp) = *(mv->lastp);
				mv->exclp++;
				*mm = mv->lastp;
				mv->lastp->ord=mv->count;
				mv->lastp++;
				mv->next = mv->lastp;
//				mv->actph = KILLER1;
				return ++mv->count;
			}
		}
	case KILLER2:
		mv->phase = KILLER3;
		if ((b->pers->use_killer >= 1)) {
			r = get_killer_move(b->kmove, ply, 1, &pot);
			if (r && isMoveValid(b, pot, a, side, tree)
				&& (!ExcludeMove(mv, pot))) {
				mv->lastp->move = pot;
//				mv->actph = KILLER2;
				mv->lastp->phase=KILLER2;
				mv->lastp->ord=mv->count;
				*mm = mv->lastp;
				*(mv->exclp) = *(mv->lastp);
				mv->exclp++;
				mv->lastp++;
				mv->next = mv->lastp;
				return ++mv->count;
			}
		}
	case KILLER3:
		mv->phase = KILLER4;
		if ((b->pers->use_killer >= 1)) {
			if (ply > 2) {
				r = get_killer_move(b->kmove, ply - 2, 0, &pot);
				if (r && isMoveValid(b, pot, a, side, tree)
					&& (!ExcludeMove(mv, pot))) {
					mv->lastp->move = pot;
//					mv->actph = KILLER3;
					mv->lastp->phase=KILLER3;
					mv->lastp->ord=mv->count;
					*mm = mv->lastp;
					*(mv->exclp) = *(mv->lastp);
					mv->exclp++;
					mv->lastp++;
					mv->next = mv->lastp;
					return ++mv->count;
				}
			}
		}
	case KILLER4:
		mv->phase = GENERATE_NORMAL;
		if ((b->pers->use_killer >= 1)) {
			if (ply > 2) {
				r = get_killer_move(b->kmove, ply - 2, 1, &pot);
				if (r && isMoveValid(b, pot, a, side, tree)
					&& (!ExcludeMove(mv, pot))) {
					mv->lastp->move = pot;
//					mv->actph = KILLER4;
					mv->lastp->phase=KILLER4;
					mv->lastp->ord=mv->count;
					*mm = mv->lastp;
					*(mv->exclp) = *(mv->lastp);
					mv->exclp++;
					mv->lastp++;
					mv->next = mv->lastp;
					return ++mv->count;
				}
			}
		}
	case GENERATE_NORMAL:
		mv->quiet = mv->next = mv->lastp;
		generateMovesN2(b, a, &(mv->lastp));
		DEB_S2(m=mv->lastp-1;for(;m>=mv->next; m--) m->state=0; )
		// get HH values and sort
		ScoreNormal(b, mv, side);
		SelectBestO(mv);
rest_moves: mv->phase = NORMAL;
//		mv->actph = NORMAL;
	case NORMAL:
		while (mv->next < mv->lastp) {
			if (ExcludeMove(mv, mv->next->move)) {
				mv->next++;
				continue;
			}
			mv->next->phase=NORMAL;
			*mm = mv->next;
			mv->next->ord=mv->count;
			mv->next++;
			return ++mv->count;
		}
		mv->phase = OTHER_SET;
	case OTHER_SET:
		mv->phase = OTHER;
		mv->next = mv->bad;
	case OTHER:
		while (mv->next < mv->badp) {
			if (ExcludeMove(mv, mv->next->move)) {
				mv->next++;
				continue;
			}
//			mv->actph = OTHER;
			mv->next->phase=OTHER;
			*mm = mv->next;
			mv->next->ord=mv->count;
			mv->next++;
			return ++mv->count;
		}
		mv->phase = DONE;
	case DONE:
		break;
//	default:
	}
	return 0;
}

int getNextCheckin(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree)
{
char b2[512];
	move_entry *m;
	
	switch (mv->phase) {
	case INIT:
		// setup everything
		mv->lastp = mv->move;
		mv->next = mv->lastp;
		mv->badp = mv->bad;
		mv->exclp = mv->excl;
		mv->count = 0;
		mv->phase = PVLINE;
		mv->quiet = NULL;
// previous PV move
	case PVLINE:
		mv->phase = GENERATE_NORMAL;
	case GENERATE_NORMAL:
		mv->quiet = mv->lastp;
		generateQuietCheckMovesN(b, a, &(mv->lastp));
		DEB_S2(m=mv->lastp-1;for(;m>=mv->next; m--) m->state=0; )
//		mv->actph = NORMAL;
		mv->phase = NORMAL;
	case NORMAL:

		while (mv->next < mv->lastp) {
			mv->next->phase=NORMAL;
			*mm = mv->next;
			mv->next++;
			return ++mv->count;
		}
		mv->phase = OTHER;
		mv->next = mv->bad;
	case OTHER:
		mv->phase = DONE;
	case DONE:
		break;
	}
	return 0;
}

int getNextCap(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store *tree)
{
	move_entry *m;

	switch (mv->phase) {
	case INIT:
		// setup everything
		mv->lastp = mv->move;
		mv->badp = mv->bad;
		mv->exclp = mv->excl;
		mv->count = 0;
		mv->phase = PVLINE;
		mv->quiet = NULL;
		mv->lpcheck = ! ( 
			(BitCount(
			  ((b->maps[BISHOP] | b->maps[ROOK] | b->maps[QUEEN]) & b->colormaps[Flip(side)]))==0)
			&& (BitCount
			  ((b->maps[PAWN]) & b->colormaps[Flip(side)])==1))
			  ;
		LOGGER_SE("Init\n");

// previous PV move
	case PVLINE:
		mv->phase = GENERATE_CAPTURES;
		LOGGER_SE("PVLINE\n");
	case GENERATE_CAPTURES:
		mv->phase = CAPTUREA;
		mv->next = mv->lastp;
		generateCapturesN2(b, a, &(mv->lastp), 0);
		DEB_S2(m=mv->lastp-1;for(;m>=mv->next; m--) m->state=0; )
		mv->tcnt = 0;
//		mv->actph = CAPTUREA;
		LOGGER_SE("GEN CAP\n");
	case CAPTUREA:
		while ((mv->next < mv->lastp) && (mv->tcnt > 0)) {
			mv->tcnt--;
			SelectBest(mv);
			if (((mv->next->qorder < A_OR2_MAX)
				&& (mv->next->qorder > A_OR2))
				&& (SEEx(b, mv->next->move) < 0)
				&& mv->lpcheck) {
				mv->next->phase=OTHER;
				mv->next++;
				continue;
			}
			mv->next->phase=CAPTUREA;
			*mm = mv->next;
			mv->next++;
			LOGGER_SE("CAPTUREA\n");
			return ++mv->count;
		}
		mv->phase = SORT_CAPTURES;
	case SORT_CAPTURES:
		SelectBestO(mv);
//		mv->actph = CAPTURES;
		mv->phase = CAPTURES;
		LOGGER_SE("SORT CAP\n");
	case CAPTURES:
		while (mv->next < mv->lastp) {
			if (((mv->next->qorder < A_OR2_MAX)
				&& (mv->next->qorder > A_OR2))
				&& (SEEx(b, mv->next->move) < 0)
				&& mv->lpcheck) {
				mv->next->phase=OTHER;
				mv->next++;
				continue;
			}
			mv->next->phase=CAPTURES;
			*mm = mv->next;
			mv->next++;
			LOGGER_SE("CAPTURES\n");
			return ++mv->count;
		}
		mv->phase = DONE;
	case DONE:
		LOGGER_SE("DONE\n");
		break;
	}
	return 0;
}

int sortMoveListNew_Init(board *b, attack_model *a, move_cont *mv)
{
	mv->phase = INIT;
	mv->hash.move = DRAW_M;
#if 1
	for(int f=0;f<299;f++) {
	mv->move[f].ord=-1;
	mv->bad[f].ord=-1;
	mv->excl[f].ord=-1;
	}
#endif
	return 0;
}

/*
 * Degrade second move in row with the same piece, scale by phase; maximal effect in beginning of the game
 * doesnt affect promotions, ep, rochade
 */

int gradeMoveInRow(board *b, attack_model *a, MOVESTORE square, move_entry *n, int count)
{
	int q;

	int s, p, min;
	long int val;
	s = UnPackTo(square);
	p = UnPackProm(square);

	for (q = 0; q < count; q++) {
		if ((UnPackFrom(n[q].move) == s) && (p == ER_PIECE)) {
			min = 0;
			val = n[q].qorder;
			if ((val >= A_OR) && (val < A_OR_MAX))
				min = A_OR;
			else if ((val >= A_OR_N) && (val < A_OR_N_MAX))
				min = A_OR_N;
			else if ((val >= A_OR2) && (val < A_OR2_MAX))
				min = A_OR2;
			else if ((val >= MV_BAD) && (val < MV_BAD_MAX))
				min = MV_BAD;
			else if ((val >= MV_OR) && (val < MV_OR_MAX))
				min = MV_OR;
			n[q].qorder = min + (val - min) * a->phase / 255;
			break;
		}
	}
	return count;
}

void sprintfMoveSimple(MOVESTORE m, char *buf)
{
	int from, to, prom;
	char b2[100];

	switch (m & (~CHECKFLAG)) {
	case DRAW_M:
		sprintf(buf, " Draw ");
		return;
	case MATE_M:
		sprintf(buf, "# ");
		return;
	case NA_MOVE:
		sprintf(buf, " N/A ");
		return;
	case NULL_MOVE:
		sprintf(buf, " NULL ");
		return;
	case WAS_HASH_MOVE:
		sprintf(buf, " WAS_HASH ");
		return;
	case BETA_CUT:
		sprintf(buf, " WAS_BETA_CUT ");
		return;
	case ALL_NODE:
		sprintf(buf, " WAS_ALL_NODE ");
		return;
	case ERR_NODE:
		sprintf(buf, " ERR_NODE ");
		return;
	}

	from = UnPackFrom(m&(~CHECKFLAG));
	to = UnPackTo(m);
	sprintf(buf, "%s%s", SQUARES_ASC[from], SQUARES_ASC[to]);
	prom = UnPackProm(m);
	b2[0] = '\0';
	if (prom != ER_PIECE) {
		if (prom == QUEEN)
			sprintf(b2, "q");
		else if (prom == KNIGHT)
			sprintf(b2, "n");
		else if (prom == ROOK)
			sprintf(b2, "r");
		else if (prom == BISHOP)
			sprintf(b2, "b");
		strcat(buf, b2);
	}
}

/*
 * SAN move d4, Qa6xb7#, fxg1=Q+,
 * rozliseni pokud vice figur stejneho typu muze na stejne misto
 * 1. pocatecni sloupec hned za oznaceni figury
 * 2. pocatecni radek hned za oznaceni figury
 * 3. pocatecni souracnice za oznaceni figury
 *
 * oznaceni P pro pesce se neuvadi
 * zapis brani pescem obsahuje pocatecni sloupec
 *
 * [MovingPiece][from][x]TO[promotion][+]
 *
 */
/*
 * moves encoding
 * prom to
 * 		- KING, means rochade (rochade can be encoded without it, but this helps)
 * 		- PAWN, means EP
 * 		- ER_PIECE, normal movement
 * 		- ER_PIECE+1, doublepush (can be encoded without it, but it helps)
 */

//
void sprintfMove(board *b, MOVESTORE m, char *buf)
{
	int from, to, prom, cap, side, mate;

	unsigned char pto, pfrom;
	char b2[512], b3[512];
	int ep_add;
	BITVAR aa;

	ep_add = b->side == WHITE ? +8 : -8;
	buf[0] = '\0';
	from = UnPackFrom(m);
	to = UnPackTo(m);
	prom = UnPackProm(m);
	mate = 0;
	pfrom = b->pieces[from] & PIECEMASK;
	pto = b->pieces[to] & PIECEMASK;
	side = (b->pieces[from] & BLACKPIECE) == 0 ? WHITE : BLACK;
	if ((pfrom == PAWN) && (to == (b->ep + ep_add)))
		pto = PAWN;
	switch (m & (~CHECKFLAG)) {
	case DRAW_M:
		strcat(buf, " Draw ");
		return;
	case MATE_M:
		strcat(buf, "# ");
		mate = 1;
		return;
	case NA_MOVE:
		strcat(buf, " N/A ");
		return;
	case NULL_MOVE:
		strcat(buf, " NULL ");
		return;
	case WAS_HASH_MOVE:
		strcat(buf, " WAS_HASH ");
		return;
	case BETA_CUT:
		strcat(buf, " WAS_BETA_CUT ");
		return;
	case ALL_NODE:
		strcat(buf, " WAS_ALL_NODE ");
		return;
	case ERR_NODE:
		strcat(buf, " ERR_NODE ");
		return;
	}
//to
	sprintf(b2, "%s", SQUARES_ASC[to]);
	sprintf(buf, "%s", b2);
	cap = 0;
//capture
	if (pto != ER_PIECE) {
		sprintf(b2, "x%s", buf);
		sprintf(buf, "%s", b2);
		cap = 1;
	}
// who is moving?
// who is attacking this destination
	aa = 0;
	switch (pfrom) {
	case BISHOP:
		aa = BishopAttacks(b, to);
		aa = aa & b->maps[pfrom];
		aa = (aa & (b->colormaps[side]));
		sprintf(b2, "B");
		break;
	case QUEEN:
		aa = QueenAttacks(b, to);
		aa = aa & b->maps[pfrom];
		aa = (aa & (b->colormaps[side]));
		sprintf(b2, "Q");
		break;
	case KNIGHT:
		aa = attack.maps[KNIGHT][to];
		aa = aa & b->maps[pfrom];
		aa = (aa & (b->colormaps[side]));
		sprintf(b2, "N");
		break;
	case ROOK:
		aa = RookAttacks(b, to);
		aa = aa & b->maps[pfrom];
		aa = (aa & (b->colormaps[side]));
		sprintf(b2, "R");
		break;
	case PAWN:
		b2[0] = '\0';
		if (cap) {
			aa = ((attack.pawn_att[WHITE][to] & b->maps[PAWN]
				& (b->colormaps[BLACK]))
				| (attack.pawn_att[BLACK][to] & b->maps[PAWN]
					& (b->colormaps[WHITE])));
			aa &= b->colormaps[side];
		} else {
			aa = NORMM(from);
		}
		break;
	case KING:
//FIXME				
		aa = NORMM(from);
		sprintf(b2, "K");
		break;
	default:
		sprintf(b2, "Unk");
		L0("ERROR unknown piece %d\n", pfrom);
		sprintf(b2, "%s", SQUARES_ASC[from]);
		L0("ERROR unknown piece from %s\n", b2);
		assert(0);
	}
// provereni zdali je vic figur stejneho typu ktere mohou na cilove pole

	b3[0] = '\0';
	if ((BitCount(aa) > 1) || ((cap == 1) && (pfrom == PAWN))) {
		if (BitCount(attack.file[from] & aa) == 1) {
			sprintf(b3, "%c", getFile(from) + 'a');
// file is enough
		} else if (BitCount(attack.rank[from] & aa) == 1) {
// rank is enough
			sprintf(b3, "%c", getRank(from) + '1');
		} else {
// file&rank are needed
			sprintf(b3, "%c%c", getFile(from) + 'a',
				getRank(from) + '1');
		}
	}
// poskladame vystup do buf
	strcat(b2, b3);
	strcat(b2, buf);
	strcpy(buf, b2);

	if ((pfrom == KING) && (prom == KING)) {
		if (from > to)
			sprintf(b2, "O-O-O");
		else
			sprintf(b2, "O-O");
		sprintf(buf, "%s", b2);
	} else if ((pfrom == PAWN)) {
		if (prom == QUEEN)
			strcat(buf, "=Q");
		else if (prom == KNIGHT)
			strcat(buf, "=N");
		else if (prom == BISHOP)
			strcat(buf, "=B");
		else if (prom == ROOK)
			strcat(buf, "=R");
	}
	if ((m & CHECKFLAG) && (mate != 1)) {
		strcat(buf, "+");
	}
}

