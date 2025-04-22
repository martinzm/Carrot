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

#ifndef MOVGEN_H
#define MOVGEN_H

#include "bitmap.h"

typedef struct _undo {
	MOVESTORE move;
	BITVAR key;
	BITVAR pawnkey;
	BITVAR old50key;  //what was the old key
	BITVAR old50pos;  //what was the old position

	int psq_b;
	int psq_e;
	int prev_mindex;
	int16_t rule50move;
	int8_t side;
	int8_t captured;  //what was captured
	int8_t moved;  // promoted to in case promotion, otherwise the same as old
	int8_t old;  //what was the old piece
	int8_t ep, prev_ep; //state of ep before and after move
	int8_t mindex_validity;
	int8_t from, to;
	int8_t fRO, toRO; // if castling, rook move
	int8_t whereCa; //where capture took place
	int8_t prev_castle[ER_SIDE], castle[ER_SIDE];
/*
 * sources of changes
 * from, to, fRO, toT, whereCapt, captured, promoted, moved, oldEP, newEP, oldCast, newCast
 */
} UNDO;

typedef struct __changed {
	int8_t from, to;
	int8_t fRO, toRO;
	int8_t captured, whereCa; 
	int8_t moved, promoted;
	int8_t ep, prev_ep;
	int8_t prev_castle[ER_SIDE], castle[ER_SIDE];
} CHANGE;

#define CHECKFLAG (1<<15)

#define PackMove(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6) | (((prom) & 7) <<12))))
#define PackMoveF(from,to,prom,spec)  ((MOVESTORE)((((from) & 0x3F) | (((to) & 0x3F) <<6)))|(ER_PIECE<<12))
#define UnPackFrom(a)  ((int) ((a) & 0x3F))
#define UnPackTo(a)    ((int) (((a)>>6) & 0x3F))
#define UnPackProm(a)  ((int) (((a) >>12) & 7))
#define UnPackCheck(a) ((int) ((a>>15) & 1))
#define UnPackPPos(a)  ((MOVESTORE) ((a) & 0xFFF)) //only from & to extracted

int isMoveValid(board*, MOVESTORE, const attack_model*, int, tree_store*);

BITVAR isInCheck_Eval(board *b, attack_model *a, int side);
void generateCapturesN(const board *const b, attack_model *a, move_entry **m, int gen_u);
void generateMovesN(const board *const b, attack_model *a, move_entry **m);
void generateInCheckMovesN(const board *const b, attack_model *a, move_entry **m, int gen_u);
void generateQuietCheckMovesN(const board *const b, attack_model *a, move_entry **m);
int alternateMovGen(board *b, MOVESTORE *filter);
int MakeMove(board *b, MOVESTORE move, UNDO *ret);
int MakeMoveNew(board *b, MOVESTORE move, int *pos , UNDO *ret);
int MakeNullMove(board *b, UNDO *ret);
void UnMakeMove(board *b, UNDO *u);
void UnMakeNullMove(board *b, UNDO *u);
int is_quiet_move(board const * const , attack_model const * const, move_entry const * const);

void printfMove(board *b, MOVESTORE m);
void sprintfMoveSimple(MOVESTORE m, char *buf);
void sprintfMove(board *b, MOVESTORE m, char *buf);
int gradeMoveInRow(board*, attack_model*, MOVESTORE, move_entry*, int);

void SelectBestO(move_cont *mv);
int sortMoveListNew_Init(board *b, attack_model *a, move_cont *mv);
int getNextMove(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store*);
int getNextCap(board *b, attack_model *a, move_cont *mv, int ply, int side, int incheck, move_entry **mm, tree_store*);

int simple_pre_movegen_n(board *b, attack_model *a, int side);
int simple_pre_movegen_n2(board const *b, attack_model *a, int side);
int simple_pre_movegen_n2check(const board * constb, attack_model *a, int side);

int getNextCheckin(board*, attack_model*, move_cont*, int, int, int, move_entry**, tree_store*);

void generateCapturesN2(const board *const b, attack_model *a, move_entry **m, int gen_u);
void generateMovesN2(const board *const b, attack_model *a, move_entry **m);
void generateInCheckMovesN2(const board *const b, attack_model *a, move_entry **m, int gen_u);
void mvsfrom2(const board * const, int const, int const, FuncAttacks, bmv **, BITVAR const, BITVAR const);
void mvsfroma2(const board * const, attack_model *, int const, int const, bmv **, BITVAR const, BITVAR const);


// board, attack, piece_type, side_to_generate, piece_function, index, dest_squares_set, pieces_moving_set
#define MVSFROM2(B, A, P, S, F, I, M, L) \
{ BITVAR v; v=B->maps[P] & (L);\
  while(v) { (I)->fr=LastOne(v);\
		(I)->pi = P;\
		(A)->mvs[(I)->fr] = (I)->mm = F(B, (I)->fr) & M;\
		(I)->mr = attack.rays_dir[B->king[S]][(I)->fr];\
		(I)++;\
		ClrLO(v);\
	}\
};

// board, attack, piece_type, side_to_generate, piece_function, index, dest_squares_set, source_squares_set, pieces_moving_set

#define MVSFROM21(B, A, P, S, F, I, M, L, X) \
{ BITVAR v; v=B->maps[P] & (L);\
  while(v) {\
		int fr=LastOne(v);\
		BITVAR mr = attack.rays_dir[B->king[S]][fr];\
		(A)->mvs[fr] = ((((X >> fr) &1)-1)|mr) & F(B, fr) & M;\
		ClrLO(v);\
	}\
};

#define MVSFROM21ooo(B, A, P, S, F, I, M, L, X) \
{ BITVAR v; v=B->maps[P] & (L);\
  while(v) {\
		(I)->fr=LastOne(v);\
		(I)->pi = P;\
		(I)->mr = attack.rays_dir[B->king[S]][(I)->fr];\
		(A)->mvs[(I)->fr] = (I)->mm = ((((X >> (I)->fr)&1)-1)|(I)->mr) & F(B, (I)->fr) & M;\
		(I)++;\
		ClrLO(v);\
	}\
};


//		mv=ix->mv = ((((pins >> (ix->fr))&1)-1)|(ix->mr))&(ix->mm) & b->colormaps[opside];



//BITVAR ChangedTo(board *b, int pos, BITVAR map, int side);
BITVAR ChangedToN(board *b, attack_model *a, UNDO *u);
BITVAR ChangesToMove(board *b, attack_model *a, UNDO *u);

int eval_king_checks_extN(board const *b, king_eval *ke, personality const *p, int side, int from);

#endif
