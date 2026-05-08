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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <fenv.h>
#include <errno.h>
#include "bitmap.h"
#include "generate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tuner.h"
#include "hash.h"
#include "defines.h"
#include "evaluate.h"
#include "utils.h"
#include "ui.h"
#include "openings.h"
#include "globals.h"
#include "search.h"
#include "pers.h"
#include "tests.h"

#include <ctype.h>
#include <limits.h>
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <omp.h>

#define KOSC 100.0

int COUNTER=0;

int variables_reinit_material(void *data)
{
	tuner_variables_pass *v;

	v = (tuner_variables_pass*) data;
	if (v->stage == 0) {
		meval_table_gen(v->p->mat, v->p, 0);
	} else {
		meval_table_gen(v->p->mate_e, v->p, 1);
	}
	return 0;
}

int variables_restore_material(void *data)
{
	tuner_variables_pass *v;

	v = (tuner_variables_pass*) data;
	if (v->stage == 0) {
		meval_table_gen(v->p->mat, v->p, 0);
	} else {
		meval_table_gen(v->p->mate_e, v->p, 1);
	}
	return 0;
}

double enforce_positive(double val)
{
	if (val > 0)
		return val;
	return 0;
}

double enforce_negative(double val)
{
	if (val < 0)
		return val;
	return 0;
}

double material_norm(double val)
{
	if (val < -40000)		return -40000;
	if (val > 40000)
		return 40000;
	return val;
}

int free_matrix(matrix_type *m, int count)
{
	int i;
	if (m != NULL) {
		for (i = 0; i < count; i++) {
//			if (m[i].init_data != NULL)
//				free(m[i].init_data);
		}
		if (m != NULL)
			free(m);
	}
	return 0;
}

int ff_dummy(void *data)
{
	return 0;
}

DEB_X(MAT_DUO(mat[i], mat[i+1], p, backward_penalty[0], backward_penalty[1], i, map); i+=2;)
DEB_X(for(sq=0;sq<=5;sq++) { MAT_DUO(mat[i], mat[i+1], p, passer_bonus[0][WHITE][sq], passer_bonus[1][WHITE][sq], i, map); 
MAT_DUO_ADD(mat[i], mat[i+1], p, passer_bonus[0][BLACK][sq], passer_bonus[1][BLACK][sq]); i+=2; })

#define MAT_SIN(MM, P, FF, I, MAP) \
    MM.upd=0;\
    MM.u[0]=MAP->p.FF;\
    MM.value_type=2; \
    MM.counterpart=-1;\
    for(int xxx=1;xxx<MATRIX_F_MAX;xxx++) MM.u[xxx]=MAP->p.FF;\
    MM.tunable=1;\
    MM.norm_f=NULL;

#define MAT_DUO(MM1, MM2, P, FF1, FF2, I, MAP) \
    MM1.upd=0;\
    MM1.u[0]=MAP->p.FF1;\
    for(int xxx=1;xxx<MATRIX_F_MAX;xxx++) MM1.u[xxx]=MAP->p.FF1;\
    MM1.tunable=1;\
    MM2.upd=0;\
    MM2.u[0]=MAP->p.FF2;\
    for(int xxx=1;xxx<MATRIX_F_MAX;xxx++) MM2.u[xxx]=MAP->p.FF2;\
    MM2.tunable=1;\
    MM1.value_type=0; \
    MM2.value_type=1; \
    MM1.counterpart=I+1;\
    MM2.counterpart=I;\
    MM1.cnp=MAP->p.FF2;\
    MM2.cnp=MAP->p.FF1;\
    MM1.norm_f=NULL;\
    MM2.norm_f=NULL;

#define MAT_DUO_ADD(MM1, MM2, P, FF1, FF2, MAP) \
    MM1.upd++;\
    MM1.u[MM1.upd]=MAP->p.FF1;\
    MM2.upd++;\
    MM2.u[MM2.upd]=MAP->p.FF2;

int to_matrix(matrix_type **m, personality *p, pers_uni *map)
{
	int i, max, pi, sq, ii;
	int len = 16384;
	matrix_type *mat;

	int pieces_all[] = { 0, 1, 2, 3, 4, 5, -1 };
	int pieces_nP[] = { 1, 2, 3, 4, 5, -1 };
	int pieces_in[] = { 1, 2, 3, 4, -1 };
	int pieces_in2[] = { 5, -1 };
	int pieces_in3[] = { 0, -1 };
	int mob_lengths[] = { 4, 9, 14, 15, 28, 9, -1 };
	int mob_lengths2[] = { 4, 9, 14, 15, 28, 9, -1 };

	mat = malloc(sizeof(matrix_type) * len);
	*m = mat;
	i = 0;
// BIAS must be index 0/1
DEB_X(MAT_DUO(mat[i], mat[i+1], p, eval_BIAS, eval_BIAS_e, i, map);  i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pawn_ah_penalty[0], pawn_ah_penalty[1], i, map);  i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, rook_on_seventh[0], rook_on_seventh[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, rook_on_open[0], rook_on_open[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, rook_on_semiopen[0], rook_on_semiopen[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pawn_weak_onopen_penalty[0], pawn_weak_onopen_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pawn_weak_center_penalty[0], pawn_weak_center_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, king_castle_pot_bonus[0], king_castle_pot_bonus[1], i, map);
  mat[i+1].tunable=0; i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, king_moved_away_bonus[0], king_moved_away_bonus[1], i, map);
  mat[i+1].tunable=0; i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pawn_iso_center_penalty[0], pawn_iso_center_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pawn_iso_onopen_penalty[0], pawn_iso_onopen_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, isolated_penalty[0], isolated_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, backward_penalty[0], backward_penalty[1], i, map); i+=2;)
DEB_0(for(sq=0;sq<=5;sq++) { MAT_DUO(mat[i], mat[i+1], p, passer_bonus[0][WHITE][sq], passer_bonus[1][WHITE][sq], i, map); mat[i].norm_f=enforce_positive;
		MAT_DUO_ADD(mat[i], mat[i+1], p, passer_bonus[0][BLACK][sq], passer_bonus[1][BLACK][sq], map); mat[i].norm_f=enforce_positive; i+=2; })
DEB_X(for(sq=0;sq<=5;sq++) { MAT_DUO(mat[i], mat[i+1], p, pot_passer_bonus[0][WHITE][sq], pot_passer_bonus[1][WHITE][sq], i, map); MAT_DUO_ADD(mat[i], mat[i+1], p, pot_passer_bonus[0][BLACK][sq], pot_passer_bonus[1][BLACK][sq], map); i+=2; })
DEB_0(for(sq=0;sq<=6;sq++) { MAT_DUO(mat[i], mat[i+1], p, passer_my_king_bonus[0][WHITE][sq], passer_my_king_bonus[1][WHITE][sq], i, map); MAT_DUO_ADD(mat[i], mat[i+1], p, passer_my_king_bonus[0][BLACK][sq], passer_my_king_bonus[1][BLACK][sq], map);
  mat[i].tunable=0; i+=2; })
DEB_0(for(sq=0;sq<=6;sq++) { MAT_DUO(mat[i], mat[i+1], p, passer_op_king_penalty[0][WHITE][sq], passer_op_king_penalty[1][WHITE][sq], i, map); MAT_DUO_ADD(mat[i], mat[i+1], p, passer_op_king_penalty[0][BLACK][sq], passer_op_king_penalty[1][BLACK][sq], map);
  mat[i].tunable=0; i+=2; })

DEB_0(for(sq=0;sq<=6;sq++) { MAT_DUO(mat[i], mat[i+1], p, pawn_op_king_bonus[0][WHITE][sq], pawn_op_king_bonus[1][WHITE][sq], i, map); MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_op_king_bonus[0][BLACK][sq], pawn_op_king_bonus[1][BLACK][sq], map);
  mat[i].tunable=1; i+=2; })

DEB_0(MAT_DUO(mat[i], mat[i+1], p, passer_unstop_bonus[0], passer_unstop_bonus[1], i, map); mat[i].tunable=0; i+=2;)

DEB_X(for(sq=0;sq<=4;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pawn_blocked_penalty[0][WHITE][sq], pawn_blocked_penalty[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_blocked_penalty[0][BLACK][sq], pawn_blocked_penalty[1][BLACK][sq], map);
	  i+=2; } )
DEB_X(for(sq=0;sq<=4;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pawn_stopped_penalty[0][WHITE][sq], pawn_stopped_penalty[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_stopped_penalty[0][BLACK][sq], pawn_stopped_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_X(for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pawn_issues_penalty[0][WHITE][sq], pawn_issues_penalty[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_issues_penalty[0][BLACK][sq], pawn_issues_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_0(
		 ii=0; 
		 while(pieces_in[ii]!=-1) {
		 pi=pieces_in[ii];
		 for(sq=0;sq<=63;sq++){
		 MAT_DUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map);
		 MAT_DUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map);
		 i+=2; }
		 ii++; }
		 ii=0; 
		 while(pieces_in2[ii]!=-1) { 
		 pi=pieces_in2[ii]; 
		 for(sq=0;sq<=63;sq++){ MAT_DUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map); 
		 MAT_DUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map);
		   mat[i].tunable=0;
		 i+=2; }
		 ii++; }
		 )

DEB_X(
		 ii=0; 
		 pi=KING+1;
		 for(sq=0;sq<=63;sq++){ MAT_DUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map); 
		 MAT_DUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map);
		   mat[i].tunable=0;
		 i+=2; }
		 ii++;
		 )

DEB_0(
		 ii=0;
		 while(pieces_in3[ii]!=-1) { 
		 pi=pieces_in3[ii]; 
		 for(sq=8;sq<=55;sq++){ 
		 MAT_DUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map); 
		 MAT_DUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map); 
		 i+=2; } 
		 ii++; }
		 )
DEB_0(
		for(pi=0;pi<=5;pi++) { for(sq=0;sq<mob_lengths[pi];sq++){ MAT_DUO(mat[i], mat[i+1], p, mob_val[0][WHITE][pi][sq], mob_val[1][WHITE][pi][sq], i, map); MAT_DUO_ADD(mat[i], mat[i+1], p, mob_val[0][BLACK][pi][sq], mob_val[1][BLACK][pi][sq], map); i+=2; } })
DEB_0(
		for(pi=0;pi<=5;pi++) { for(sq=0;sq<mob_lengths2[pi];sq++){ MAT_DUO(mat[i], mat[i+1], p, mob_uns[0][WHITE][pi][sq], mob_uns[1][WHITE][pi][sq], i, map); MAT_DUO_ADD(mat[i], mat[i+1], p, mob_uns[0][BLACK][pi][sq], mob_uns[1][BLACK][pi][sq], map); i+=2; } })
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_open_penalty[0], pshelter_open_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_isol_penalty[0], pshelter_isol_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_hopen_penalty[0], pshelter_hopen_penalty[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_double_penalty[0], pshelter_double_penalty[1], i, map); i+=2;)

DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_prim_bonus[0], pshelter_prim_bonus[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_sec_bonus[0], pshelter_sec_bonus[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, pshelter_out_penalty[0], pshelter_out_penalty[1], i, map); i+=2;)
DEB_X(for(sq=0;sq<=4;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pshelter_blocked_penalty[0][WHITE][sq], pshelter_blocked_penalty[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pshelter_blocked_penalty[0][BLACK][sq], pshelter_blocked_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_X(for(sq=0;sq<=4;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pshelter_stopped_penalty[0][WHITE][sq], pshelter_stopped_penalty[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pshelter_stopped_penalty[0][BLACK][sq], pshelter_stopped_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_X(for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pshelter_dir_protect[0][WHITE][sq], pshelter_dir_protect[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pshelter_dir_protect[0][BLACK][sq], pshelter_dir_protect[1][BLACK][sq], map);
      i+=2; } )
DEB_X(for(sq=0;sq<=7;sq++) {
	  MAT_DUO(mat[i], mat[i+1], p, pawn_n_protect[0][WHITE][sq], pawn_n_protect[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_n_protect[0][BLACK][sq], pawn_n_protect[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, pawn_pot_protect[0][WHITE][sq],pawn_pot_protect[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_pot_protect[0][BLACK][sq], pawn_pot_protect[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
	  MAT_DUO(mat[i], mat[i+1], p, pawn_dir_protect[0][WHITE][sq], pawn_dir_protect[1][WHITE][sq], i, map);
	  MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_dir_protect[0][BLACK][sq], pawn_dir_protect[1][BLACK][sq], map);
	  i+=2; })
DEB_X(for(sq=0;sq<=7;sq++) {
      MAT_DUO(mat[i], mat[i+1], p, doubled_n_penalty[0][WHITE][sq], doubled_n_penalty[1][WHITE][sq], i, map);
      MAT_DUO_ADD(mat[i], mat[i+1], p, doubled_n_penalty[0][BLACK][sq], doubled_n_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_X(MAT_DUO(mat[i], mat[i+1], p, bishopboth[0], bishopboth[1], i, map); i+=2;)
DEB_X(MAT_DUO(mat[i], mat[i+1], p, rookpair[0], rookpair[1], i, map); i+=2; )
DEB_X(MAT_DUO(mat[i], mat[i+1], p, knightpair[0], knightpair[1], i, map); i+=2; )
DEB_X(for(sq=0;sq<=7;sq++) {
				MAT_DUO(mat[i], mat[i+1], p, pawn_protect_count[0][WHITE][sq], pawn_protect_count[1][WHITE][sq], i, map);
				MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_protect_count[0][BLACK][sq], pawn_protect_count[1][BLACK][sq], map);
			i+=2;})
DEB_X(for(sq=0;sq<=7;sq++) {
				MAT_DUO(mat[i], mat[i+1], p, pawn_prot_over_penalty[0][WHITE][sq], pawn_prot_over_penalty[1][WHITE][sq], i, map);
				MAT_DUO_ADD(mat[i], mat[i+1], p, pawn_prot_over_penalty[0][BLACK][sq], pawn_prot_over_penalty[1][BLACK][sq], map);
				i+=2;})

DEB_0(
	ii = 0;
	while (pieces_in[ii] != -1) {
		sq = pieces_nP[ii];
		MAT_DUO(mat[i], mat[i+1], p, Values[0][sq], Values[1][sq], i, map);
		i += 2;
		ii++;
	}
#if 1
	ii = 0;
	while (pieces_in3[ii] != -1) {
		sq = pieces_in3[ii];
		MAT_DUO(mat[i], mat[i+1], p, Values[0][sq], Values[1][sq], i, map);
		mat[i].tunable=0;
		i += 2;
		ii++;
	}
#endif
)
return i;
}



#define MAT_tSIN(MM, P, FF, I, MAP) \
    MM.upd=0;\
    MM.u[0]=MAP->p.FF;\
    MM.value_type=2; \
    MM.counterpart=-1;\
    for(int xxx=1;xxx<MATRIX_F_MAX;xxx++) MM.u[xxx]=MAP->p.FF;\
    MM.tunable=1;

#define MAT_tDUO(MM1, MM2, P, FF1, FF2, I, MAP) \
    MM1.upd=0;\
    MM1.u[0]=MAP->p.FF1;\
    for(int xxx=1;xxx<MATRIX_F_MAX;xxx++) MM1.u[xxx]=MAP->p.FF1;\
    MM1.tunable=1;\
    MM2.upd=0;\
    MM2.u[0]=MAP->p.FF2;\
    for(int xxx=1;xxx<MATRIX_F_MAX;xxx++) MM2.u[xxx]=MAP->p.FF2;\
    MM2.tunable=1;\
    MM1.value_type=0; \
    MM2.value_type=1; \
    MM1.counterpart=I+1;\
    MM2.counterpart=I;\
    MM1.cnp=MAP->p.FF2;\
    MM2.cnp=MAP->p.FF1;

#define MAT_tDUO_ADD(MM1, MM2, P, FF1, FF2, MAP)


int text_map (matrix_type **m, personality *p, pers_uni *map)
{
	int i, max, pi, sq, ii;
	int len = 16384;
	matrix_type *mat;

	int pieces_all[] = { 0, 1, 2, 3, 4, 5, -1 };
	int pieces_nP[] = { 1, 2, 3, 4, 5, -1 };
	int pieces_in[] = { 1, 2, 3, 4, -1 };
	int pieces_in2[] = { 5, -1 };
	int pieces_in3[] = { 0, -1 };
	int mob_lengths[] = { 4, 9, 14, 15, 28, 9, -1 };
	int mob_lengths2[] = { 4, 9, 14, 15, 28, 9, -1 };

	mat = malloc(sizeof(matrix_type) * len);
	*m = mat;
	i = 0;
// BIAS must be index 0/1
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, eval_BIAS, eval_BIAS_e, i, map);  i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pawn_ah_penalty[0], pawn_ah_penalty[1], i, map);  i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, rook_on_seventh[0], rook_on_seventh[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, rook_on_open[0], rook_on_open[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, rook_on_semiopen[0], rook_on_semiopen[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pawn_weak_onopen_penalty[0], pawn_weak_onopen_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pawn_weak_center_penalty[0], pawn_weak_center_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, king_castle_pot_bonus[0], king_castle_pot_bonus[1], i, map);
  mat[i+1].tunable=0; i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, king_moved_away_bonus[0], king_moved_away_bonus[1], i, map);
  mat[i+1].tunable=0; i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pawn_iso_center_penalty[0], pawn_iso_center_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pawn_iso_onopen_penalty[0], pawn_iso_onopen_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, isolated_penalty[0], isolated_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, backward_penalty[0], backward_penalty[1], i, map); i+=2;)
DEB_0(for(sq=0;sq<=5;sq++) { MAT_tDUO(mat[i], mat[i+1], p, passer_bonus[0][WHITE][sq], passer_bonus[1][WHITE][sq], i, map); MAT_tDUO_ADD(mat[i], mat[i+1], p, passer_bonus[0][BLACK][sq], passer_bonus[1][BLACK][sq], map); i+=2; })
DEB_0(for(sq=0;sq<=5;sq++) { MAT_tDUO(mat[i], mat[i+1], p, pot_passer_bonus[0][WHITE][sq], pot_passer_bonus[1][WHITE][sq], i, map); MAT_tDUO_ADD(mat[i], mat[i+1], p, pot_passer_bonus[0][BLACK][sq], pot_passer_bonus[1][BLACK][sq], map); i+=2; })
DEB_0(for(sq=0;sq<=6;sq++) { MAT_tDUO(mat[i], mat[i+1], p, passer_my_king_bonus[0][WHITE][sq], passer_my_king_bonus[1][WHITE][sq], i, map); MAT_tDUO_ADD(mat[i], mat[i+1], p, passer_my_king_bonus[0][BLACK][sq], passer_my_king_bonus[1][BLACK][sq], map);
  mat[i].tunable=0; i+=2; })
DEB_0(for(sq=0;sq<=6;sq++) { MAT_tDUO(mat[i], mat[i+1], p, passer_op_king_penalty[0][WHITE][sq], passer_op_king_penalty[1][WHITE][sq], i, map); MAT_tDUO_ADD(mat[i], mat[i+1], p, passer_op_king_penalty[0][BLACK][sq], passer_op_king_penalty[1][BLACK][sq], map);
  mat[i].tunable=0; i+=2; })
DEB_0(for(sq=0;sq<=4;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pawn_blocked_penalty[0][WHITE][sq], pawn_blocked_penalty[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_blocked_penalty[0][BLACK][sq], pawn_blocked_penalty[1][BLACK][sq], map);
	  i+=2; } )
DEB_0(for(sq=0;sq<=4;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pawn_stopped_penalty[0][WHITE][sq], pawn_stopped_penalty[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_stopped_penalty[0][BLACK][sq], pawn_stopped_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pawn_issues_penalty[0][WHITE][sq], pawn_issues_penalty[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_issues_penalty[0][BLACK][sq], pawn_issues_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_0(
		 ii=0; 
		 while(pieces_in[ii]!=-1) {
		 pi=pieces_in[ii];
		 for(sq=0;sq<=63;sq++){
		 MAT_tDUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map);
		 MAT_tDUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map);
		 i+=2; }
		 ii++; }
		 ii=0; 
		 while(pieces_in2[ii]!=-1) { 
		 pi=pieces_in2[ii]; 
		 for(sq=0;sq<=63;sq++){ MAT_tDUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map); 
		 MAT_tDUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map);
		   mat[i].tunable=0;
		 i+=2; }
		 ii++; }
		 )

DEB_0(
		 ii=0; 
		 pi=KING+1;
		 for(sq=0;sq<=63;sq++){ MAT_tDUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map); 
		 MAT_tDUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map);
		   mat[i].tunable=0;
		 i+=2; }
		 ii++;
		 )

DEB_0(
		 ii=0;
		 while(pieces_in3[ii]!=-1) { 
		 pi=pieces_in3[ii]; 
		 for(sq=8;sq<=56;sq++){ 
		 MAT_tDUO(mat[i], mat[i+1], p, piecetosquare[0][WHITE][pi][sq], piecetosquare[1][WHITE][pi][sq], i, map); 
		 MAT_tDUO_ADD(mat[i], mat[i+1], p, piecetosquare[0][BLACK][pi][Square_Swap[sq]], piecetosquare[1][BLACK][pi][Square_Swap[sq]], map); 
		 i+=2; } 
		 ii++; }
		 )
DEB_0(
		for(pi=0;pi<=5;pi++) { for(sq=0;sq<mob_lengths[pi];sq++){ MAT_tDUO(mat[i], mat[i+1], p, mob_val[0][WHITE][pi][sq], mob_val[1][WHITE][pi][sq], i, map); MAT_tDUO_ADD(mat[i], mat[i+1], p, mob_val[0][BLACK][pi][sq], mob_val[1][BLACK][pi][sq], map); i+=2; } })
DEB_0(
		for(pi=0;pi<=5;pi++) { for(sq=0;sq<mob_lengths2[pi];sq++){ MAT_tDUO(mat[i], mat[i+1], p, mob_uns[0][WHITE][pi][sq], mob_uns[1][WHITE][pi][sq], i, map); MAT_tDUO_ADD(mat[i], mat[i+1], p, mob_uns[0][BLACK][pi][sq], mob_uns[1][BLACK][pi][sq], map); i+=2; } })
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_open_penalty[0], pshelter_open_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_isol_penalty[0], pshelter_isol_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_hopen_penalty[0], pshelter_hopen_penalty[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_double_penalty[0], pshelter_double_penalty[1], i, map); i+=2;)

DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_prim_bonus[0], pshelter_prim_bonus[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_sec_bonus[0], pshelter_sec_bonus[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, pshelter_out_penalty[0], pshelter_out_penalty[1], i, map); i+=2;)
DEB_0(for(sq=0;sq<=4;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pshelter_blocked_penalty[0][WHITE][sq], pshelter_blocked_penalty[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pshelter_blocked_penalty[0][BLACK][sq], pshelter_blocked_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=4;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pshelter_stopped_penalty[0][WHITE][sq], pshelter_stopped_penalty[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pshelter_stopped_penalty[0][BLACK][sq], pshelter_stopped_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pshelter_dir_protect[0][WHITE][sq], pshelter_dir_protect[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pshelter_dir_protect[0][BLACK][sq], pshelter_dir_protect[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
	  MAT_tDUO(mat[i], mat[i+1], p, pawn_n_protect[0][WHITE][sq], pawn_n_protect[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_n_protect[0][BLACK][sq], pawn_n_protect[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, pawn_pot_protect[0][WHITE][sq],pawn_pot_protect[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_pot_protect[0][BLACK][sq], pawn_pot_protect[1][BLACK][sq], map);
      i+=2; } )
DEB_0(for(sq=0;sq<=7;sq++) {
	  MAT_tDUO(mat[i], mat[i+1], p, pawn_dir_protect[0][WHITE][sq], pawn_dir_protect[1][WHITE][sq], i, map);
	  MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_dir_protect[0][BLACK][sq], pawn_dir_protect[1][BLACK][sq], map);
	  i+=2; })
DEB_0(for(sq=0;sq<=7;sq++) {
      MAT_tDUO(mat[i], mat[i+1], p, doubled_n_penalty[0][WHITE][sq], doubled_n_penalty[1][WHITE][sq], i, map);
      MAT_tDUO_ADD(mat[i], mat[i+1], p, doubled_n_penalty[0][BLACK][sq], doubled_n_penalty[1][BLACK][sq], map);
      i+=2; } )
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, bishopboth[0], bishopboth[1], i, map); i+=2;)
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, rookpair[0], rookpair[1], i, map); i+=2; )
DEB_0(MAT_tDUO(mat[i], mat[i+1], p, knightpair[0], knightpair[1], i, map); i+=2; )
DEB_0(for(sq=0;sq<=7;sq++) {
				MAT_tDUO(mat[i], mat[i+1], p, pawn_protect_count[0][WHITE][sq], pawn_protect_count[1][WHITE][sq], i, map);
				MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_protect_count[0][BLACK][sq], pawn_protect_count[1][BLACK][sq], map);
			i+=2;})
DEB_0(for(sq=0;sq<=7;sq++) {
				MAT_tDUO(mat[i], mat[i+1], p, pawn_prot_over_penalty[0][WHITE][sq], pawn_prot_over_penalty[1][WHITE][sq], i, map);
				MAT_tDUO_ADD(mat[i], mat[i+1], p, pawn_prot_over_penalty[0][BLACK][sq], pawn_prot_over_penalty[1][BLACK][sq], map);
				i+=2;})

DEB_0(
	ii = 0;
	while (pieces_in[ii] != -1) {
		sq = pieces_nP[ii];
		MAT_tDUO(mat[i], mat[i+1], p, Values[0][sq], Values[1][sq], i, map);
		i += 2;
		ii++;
	}
#if 1
	ii = 0;
	while (pieces_in3[ii] != -1) {
		sq = pieces_in3[ii];
		MAT_tDUO(mat[i], mat[i+1], p, Values[0][sq], Values[1][sq], i, map);
		mat[i].tunable=0;
		i += 2;
		ii++;
	}
#endif
)

int start_in2[] = { 1, 2, 3, 4, -1 };
DEB_0(
  ii = 0;
  while (start_in2[ii] != -1)
	{
		sq = start_in2[ii];
		for(int x=0;x<(PAWNS_TOT);x++) {
		MAT_SIN(mat[i], p, dvalues[sq][x], i, map);
		i++;
	  }
	  ii++;
	}
)
return 1;
}

void pers_to_koef(double *koef, matrix_type *m, personality *p, int pcount)
{
pers_uni *pp;
	int f,ff;
	
	pp=(pers_uni *)p;
	for(f=0;f<pcount;f++) {
		koef[f]= pp->u[m[f].u[0]];
	}
}

void koef_to_pers(double *koef, matrix_type *m, personality *p, int pcount)
{
pers_uni *pp;
	int f,ff;

	pp=(pers_uni *)p;
	for(f=0;f<pcount;f++) {
		for(ff=0;ff<=m[f].upd;ff++) {
			pp->u[m[f].u[ff]]= (int) lround(koef[f]);
		}
	}
}

void replay_stacker(stacker *st, pers_uni *uw, pers_uni *ub){
//pers_uni uu;
stacks *t;
int f, side, vr, scl;
char bb[128];

	memset(&(uw->u),0, sizeof(int)*NTUNL);
	memset(&(ub->u),0, sizeof(int)*NTUNL);
// generate BAs
	t=st->head[BAs];
	f=0;
	while(t!=st->tail[BAs]) {
//		LOGGER_0("%d: Basic feature:%d, value:%d, side:%d\n", f, t->index, t->value, t->side);
#if 0
		if(t->side==WHITE) uw->u[t->index]+=t->value; else ub->u[t->index]+=t->value;
#else
		if(t->side==WHITE) uw->u[t->index]+=t->value*KOSC; else ub->u[t->index]+=t->value*KOSC;
#endif
		f++;
		t++;
	}
	
	for(side=0;side<2;side++) {
	if(st->variant[side]!=BAs) {
// add variant
		vr=st->variant[side];
//		L0("Variant %d, side %d\n", vr, side);
		t=st->head[vr];
		f=0;
		while(t!=st->tail[vr]) {
//			LOGGER_0("%d: Variant feature:%d, value:%d, side:%d\n", f, t->index, t->value, t->side);
			if(t->side==side) {
#if 0
			  if(side==WHITE) uw->u[t->index]+=t->value; else ub->u[t->index]+=t->value;
#else
			  scl=t->value*KOSC*st->var_scale[side]/100;
//			  L0("variant side:%d, value=%d, scale %d => %d\n", side, t->value, st->var_scale[side], scl);
			  if(side==WHITE) uw->u[t->index]+=scl; else ub->u[t->index]+=scl;
#endif
			}
			f++;
			t++;
		}
//		L0("Variant Features added %d\n", f);
	}
	}
}

int compute_neval_dir(board *b, attack_model *a, personality *p, stacker *st, pers_uni *uw, pers_uni *ub)
{
	int vi;
	struct _ui_opt uci_options;
	struct _statistics s;
	int ev;
	b->stats = &s;
	b->uci_options = &uci_options;
	// eval - white pov

	eval_king_checks_extU(b, &(a->ke[WHITE]), 0, b->king[WHITE]);
	eval_king_checks_extU(b, &(a->ke[BLACK]), 1, b->king[BLACK]);

	generateBitmaps(b, a, FULLBITMAP, WHITE);
	generateBitmaps(b, a, FULLBITMAP, BLACK);

	a->att_by_side[BLACK] = regenerateSQAttacked(b, a, BLACK);
	a->att_by_side[WHITE] = regenerateSQAttacked(b, a, WHITE);
	
	mvsfromk22(b, a, WHITE);
	mvsfromk22(b, a, BLACK);

//	vi = b->mindex_validity;
//	b->mindex_validity=0;
	// mindex_validity 0 means no scaling, ie scaling multiplicator 128
	ev = eval(b, a, p, st);
//	b->mindex_validity=vi;
	if(b->mindex_validity!=0) 
		if((b->side==WHITE && ev>=0)||(b->side==BLACK && ev<=0)){
			a->sc.scaling = (p->mat_info[b->mindex].info[b->side]);
			ev *= a->sc.scaling;
			ev /=128;
			a->sc.complete = ev;
		}

	replay_stacker(st, uw, ub);
	return ev;
}

/*
		a->sc.score_b = a->sc.score_b_w - a->sc.score_b_b + temp_b;
		a->sc.score_e = a->sc.score_e_w - a->sc.score_e_b + temp_e;

#ifndef TUNING
		a->sc.score_b+=b->psq_b;
		a->sc.score_e+=b->psq_e;
#endif 

#ifdef TUNING
	ADD_STACKER(st, eval_BIAS, 1, BAs, b->side, 0)
	ADD_STACKER(st, eval_BIAS_e, 1, BAs, b->side, 1)
#endif
	a->sc.score_b += p->eval_BIAS;
	a->sc.score_e += p->eval_BIAS_e;

		a->sc.score_nsc = a->sc.score_b * a->phase
			+ a->sc.score_e * (255 - a->phase);
 
 */
 

int eval_dir_stacker(board *b, attack_model *a, personality *p, stacker *st)
{
	int vi;
	struct _ui_opt uci_options;
	struct _statistics s;
	int ev, val;
	stacks *t;
	pers_uni *vals;
	int f, side, vr, scl;
	int sc[4];
		sc[0]=sc[1]=sc[2]=sc[3]=0;
	int *ss;
	b->stats = &s;
	b->uci_options = &uci_options;
	vals = (pers_uni *) p;

	// eval - white pov
	a->att_by_side[WHITE] = KingAvoidSQ(b, a, WHITE);
	a->att_by_side[BLACK] = KingAvoidSQ(b, a, BLACK);

	eval_king_checks_all(b, a);

#if 0
	simple_pre_movegen_n2(b, a, WHITE);
	simple_pre_movegen_n2(b, a, BLACK);
#endif

// get stacker and normal eval
	ev = eval(b, a, p, st);

//compute eval from stacker
// generate BAs
	t=st->head[BAs];
	f=0;
	while(t!=st->tail[BAs]) {
		ss= t->side==WHITE ? &sc[0] : &sc[2];
		val=t->value*KOSC*vals->u[t->index];
		L0("%d:%d:%d=> %d*%d=%d\n", t->side, t->type, t->index, t->value, vals->u[t->index], val);
		switch (t->type) {
		case 0:
				ss[0]+=val;
				break;
		case 1:
				ss[1]+=val;
				break;
		case 2:
				ss[0]+=val;
				ss[1]+=val;
				break;
		default:
			break;
			
		}
		f++;
		t++;
	}
	
	for(side=0;side<2;side++) {
		ss= side==WHITE ? &sc[0] : &sc[2];
	if(st->variant[side]!=BAs) {
// add variant
		vr=st->variant[side];
		t=st->head[vr];
		f=0;
		while(t!=st->tail[vr]) {
			if(t->side==side) {
				scl=t->value*st->var_scale[side]*vals->u[t->index];
				L0("%d:%d:%d=> %d*%d=%d\n", t->side, t->type, t->index, t->value, vals->u[t->index], scl);
				switch (t->type) {
				case 0:
					ss[0]+=scl;
					break;
				case 1:
					ss[1]+=scl;
					break;
				case 2:
					ss[0]+=scl;
					ss[1]+=scl;
					break;
				default:
					break;
				}
			}
			f++;
			t++;
		}
	}
	}
	sc[0]/=100;
	sc[1]/=100;
	sc[2]/=100;
	sc[3]/=100;
	
	int ssc_b=sc[0]-sc[2];
	int ssc_e=sc[1]-sc[3];
	int ssc_nsc=ssc_b*a->phase+ssc_e*(255-a->phase);
	int ssc_sc=ssc_nsc * a->sc.scaling / 128 / 255;
	
	L0("Eval %d<=>%d, %d:%d, %d:%d, %d:%d, %d:%d, phase %d, scale %d\n", ev, ssc_sc, a->sc.score_b_w, sc[0], a->sc.score_e_w, sc[1], \
		a->sc.score_b_b, sc[2], a->sc.score_e_b, sc[3], a->phase, a->sc.scaling);
		
	if((a->sc.score_b_w!=sc[0])||( a->sc.score_e_w!=sc[1]) ||
		(a->sc.score_b_b!=sc[2]) || ( a->sc.score_e_b!=sc[3]) ||
		(ev!=ssc_sc)) {
			L0("VALIDATION ERROR!\n");
		}
	return ssc_sc;
}

int init_ntuner_jac(ntuner_run *state, matrix_type *m, int pcount)
{
	int i;
	for (i = 0; i < pcount; i++)
		state[i].or1 = 0.00000001;
	for (i = 0; i < pcount; i++)
		state[i].or2 = 0.00000001;
	for (i = 0; i < pcount; i++)
		state[i].grad = 0;
	return 0;
}

int allocate_ntuner(ntuner_run **tr, int pcount)
{
	*tr = malloc(sizeof(ntuner_run) * (pcount));
	return 0;
}

int free_ntuner(ntuner_run *tr)
{
	free(tr);
	return 1;
}

int allocate_njac(long records, int params, njac **state)
{
	LOGGER_0("sizeof %lld, %lld, %lld\n", sizeof(feat) * params * records,
		params, records);
	printf("sizeof %ld", sizeof(njac) * records);
	*state = (njac*) malloc(sizeof(njac) * records);
	if (*state != NULL) {
		L0("start %p, end %p\n", *state, (*state) + records);
		return 1;
	}
	return 0;
}

int free_njac(njac *state, long ulen)
{
	long i;
	
	if (state != NULL) {
		for (i = 0; i < ulen; i++)
			free(state[i].ftp);
//		free(state);
	}
	return 0;
}

int dump_grads(ntuner_run *state, int pcount)
{
	int i;
	NLOGGER_0("\n");
	NLOGGER_0("OR1\t");
	for (i = 0; i < pcount; i++)
		NLOGGER_0("%f\t", state[i].or1);
	NLOGGER_0("\n");
	NLOGGER_0("OR2\t");
	for (i = 0; i < pcount; i++)
		NLOGGER_0("%f\t", state[i].or2);
	NLOGGER_0("\n");
	NLOGGER_0("GR\t");
	for (i = 0; i < pcount; i++)
		NLOGGER_0("%f\t", state[i].grad);
	NLOGGER_0("\n");

	return 0;
}

// callback funkce
#define CBACK long (*get_next)(char *fen, int8_t *res, void *data)

double comp_cost_vkx(double evaluation, double entry_result, double K)
{
	double sigmoid;
	double power;
	double expp;
	errno=0;
//	feclearexcept(FE_ALL_EXCEPT);
	expp=exp((-K) * evaluation);
	sigmoid = (2.0 / (1.0 + expp));
#if 0
	if((errno!=0)||(fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW)!=0)){
		L0("value error1!\n");
		L0("sigmoid= %.50f\n", sigmoid);
		L0("evaluation= %.50f\n", evaluation);
		L0("entry_result= %.50f\n", entry_result);
		L0("K= %.50f\n", K);
		L0("exp= %.50f\n", expp);
		if(isnan(sigmoid)) L0("NAN\n");
		if((errno==ERANGE)||(fetestexcept(FE_OVERFLOW | FE_UNDERFLOW)!=0)) L0("erange\n");
//		abort();
	}
#endif
//	feclearexcept(FE_ALL_EXCEPT);
	power = pow((entry_result - sigmoid) / 2.0, 2.0);
#if 0
	if((errno!=0)||(fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW)!=0)){
		L0("value error2!\n");
		L0("sigmoid= %.50f\n", sigmoid);
		L0("evaluation= %.50f\n", evaluation);
		L0("entry_result= %.50f\n", entry_result);
		L0("K= %.50f\n", K);
		L0("exp= %.50f\n", expp);
		if(isnan(sigmoid)) L0("NAN\n");
		if((errno==ERANGE)||(fetestexcept(FE_OVERFLOW | FE_UNDERFLOW)!=0)) L0("erange\n");
//		abort();
	}
#endif
	return power;
}

/*
 * b  - board
 * nj - features derived from this position
 * m  - info about features to derive/tune
 * st - features identified for this position
 * p  - ?
 */

int populate_njac(board *b, njac *nj, personality *p, matrix_type *m, int pcount, stacker *st)
{
// potential problem
	feat FF[10000];
	int8_t vi;
	pers_uni uw, ub;
	int16_t f_b, f_w;
	pers_uni *pp;

	double diff_step;
	double fxh, fxh1, fxh2;
	int sce1, sce2, scb1, scb2;
	int scb1_w, scb1_b, sce1_w, sce1_b;
	int scb2_w, scb2_b, sce2_w, sce2_b;
	//!!!!
	int i, ii, count, iix;
	int o, on;
	attack_model a;

	fxh = compute_neval_dir(b, &a, p, st, &uw, &ub);
//	write_personality((personality *)&ub.p, "zxb.xml");
//	write_personality((personality *)&uw.p, "zxw.xml");
//	printBoardNice(b);

//#pragma omp critical
//{
//	if((st->variant[WHITE]!=BAs)||(st->variant[BLACK]!=BAs)){
//		L0("Variant %d:%d\n", st->variant[WHITE], st->variant[BLACK]);
//		printBoardNice(b);
//	}
//}

	pp = (pers_uni *)p;
	scb2_w = a.sc.score_b_w;
	scb2_b = a.sc.score_b_b;
	sce2_w = a.sc.score_e_w;
	sce2_b = a.sc.score_e_b;

//	if(a.sc.scaling!=128) return 0;
//	L0("tun %d\n", a.sc.scaling);

/*
 * Iterate features to tune and get their values for position on board
 */

	for (i = 0; i < pcount; i++) {
		// do not get feature values when it is END value - we get features from start/middle value counterpart
		if (m[i].value_type == 1)
			continue;

// myidx -- tuned feature index
		FF[i].idx = i;
		switch (m[i].value_type) {
		case -1:
		case 0:
			f_w=f_b=0;
			for(int n=0; n<=m[n].upd; n++) {
				iix=m[i].u[n];
				f_b+=ub.u[iix];
				f_w+=uw.u[iix];
			}
#if 0
			FF[i].f_b = (int16_t) KOSC * f_b;
			FF[i].f_w = (int16_t) KOSC * f_w;
#else
//			FF[i].f_b = (int16_t) f_b;
//			FF[i].f_w = (int16_t) f_w;
			FF[i].f_x = (int16_t) f_w - f_b;
#endif
//			FF[m[i].counterpart].f_b = FF[i].f_b;
//			FF[m[i].counterpart].f_w = FF[i].f_w;
			FF[m[i].counterpart].f_x = FF[i].f_x;
			FF[m[i].counterpart].idx = m[i].counterpart;
			break;
		case 2:
			f_w=f_b=0;
			for(int n=0; n<=m[n].upd; n++) {
				iix=m[i].u[n];
				f_b+=ub.u[iix];
				f_w+=uw.u[iix];
			}
#if 0
			FF[i].f_b = (int16_t) KOSC * f_b;
			FF[i].f_w = (int16_t) KOSC * f_w;
#else 
//			FF[i].f_b = (int16_t) f_b;
//			FF[i].f_w = (int16_t) f_w;
			FF[i].f_x = (int16_t) f_w - f_b;
#endif
			break;
		default:
			break;
		}
	}

	count = 0;
	for (i = 0; i < pcount; i++) {
		// count non zero features
//		if (FF[i].f_w !=0 || FF[i].f_b!=0) {
//		if ((FF[i].f_w != FF[i].f_b)){
		if ((FF[i].f_x!=0)){
			count++;
//			int iix = m[FF[i].idx].u[0];
//			LOGGER_0("%d, %d, %d, %d, %d\n", uw.p.Values[MG][0], uw.p.Values[MG][1], uw.p.Values[MG][2], uw.p.Values[MG][3], uw.p.Values[MG][4]);
//			LOGGER_0("%d, %d, %d, %d, %d\n", uw.p.Values[EG][0], uw.p.Values[EG][1], uw.p.Values[EG][2], uw.p.Values[EG][3], uw.p.Values[EG][4]);
//			LOGGER_0("%d, %d, %d, %d, %d\n", ub.p.Values[MG][0], ub.p.Values[MG][1], ub.p.Values[MG][2], ub.p.Values[MG][3], ub.p.Values[MG][4]);
//			LOGGER_0("%d, %d, %d, %d, %d\n", ub.p.Values[EG][0], ub.p.Values[EG][1], ub.p.Values[EG][2], ub.p.Values[EG][3], ub.p.Values[EG][4]);
		}
	}
	nj->fcount = count;
// not enough features counted skip processing and signal it upstream
//	if (count <= (BitCount(b->norm)*3)+3)
//	if (count <= 0)
//		return 0;
		
	nj->ftp = (feat*) malloc(sizeof(feat) * count);
count = 0;
	for (i = 0; i < pcount; i++) {
//???? Is it really true 
//		if (FF[i].f_w !=0 || FF[i].f_b!=0) {
//		if ((FF[i].f_w != FF[i].f_b)){
		if ((FF[i].f_x!=0)){
			nj->ftp[count] = FF[i];
//			L0("F %d:%d:%d w:%d, b:%d\n", count, i, m[i].u[0], FF[i].f_w, FF[i].f_b);
			count++;
		}
	}

	// compute classical evaluation
	nj->phb = (a.phase * a.sc.scaling) / 128.0 / 255.0;
	nj->phe = ((255 - a.phase) * a.sc.scaling) / 128.0 / 255.0;

	// recompute score
	fxh2 = 0;
	for (i = 0; i < nj->fcount; i++) {
		ii = nj->ftp[i].idx;
		switch (m[ii].value_type) {
		case -1:
		case 0:
			fxh2 += (pp->u[m[ii].u[0]])
//				* (nj->ftp[i].f_w - nj->ftp[i].f_b)
				* (nj->ftp[i].f_x)
				* nj->phb/ KOSC;
			break;
		case 1:
			fxh2 += (pp->u[m[ii].u[0]])
//				* (nj->ftp[i].f_w - nj->ftp[i].f_b)
				* (nj->ftp[i].f_x)
				* nj->phe/ KOSC;
			break;
		case 2:
			fxh2 += (pp->u[m[ii].u[0]])
//				* (nj->ftp[i].f_w - nj->ftp[i].f_b)
				* (nj->ftp[i].f_x)
				* (nj->phb+nj->phe)/ KOSC;
			break;
		default:
			break;
		}
	}

	// score from evaluation not affected by tuning
	fxh2 = trunc(fxh2);
	nj->rem = fxh - fxh2;
	// score from eval
	nj->fx0 = fxh;
	nj->fxnew = fxh;
	return count;
}

// parameter, feature value, result, eval value, lambda, K
// just the derivation of coefficient+sigmoid+difference squared, summing and (derivation of) regularization is handled in caller
double Xnjac_pderiv(double koef, int16_t fea, double res, double ev, double phase, double K)
{
	double der, O;
	O = exp(-K * ev);
	der = -(4.0 * K * fea / KOSC * phase * O * (res - (2.0 / (O + 1.0))))
		/ pow((O + 1.0), 2.0);
	return der;
}

/*
 * computes evaluation of one position - *nj
 * based on actual values for coefficients in ko. ko has values of all coefficients being tuned. In order imposed by to_matrix function.
 * m contains description/info of all coefficients being tuned 
 * nj->rem - non mutable evaluation - value to add to position, but not affected by tuning
 * nj->fcount - number of non zero features in this position. 
 *   Feature is for example number of rooks if position has non zero num of rooks and material value of rook is being tuned
 * nj->ftp[ii].idx is order/index into array of coefficients ko, m
 * nj->phb, nj->phe are contributions to final score based on phase. phb is for beginning and bhe for end phase. Contains score scaling
 * nj->ftp[ii].f_w - feature for white
 * nj->ftp[ii].f_b - feature for black
 * m[i].value_type - indicates if this is beginning/middle game value=>0 or end game value=>1; -1 means value with no relation to phase
 */

double njac_eval(double *ko, njac *nj, matrix_type *m)
{
	int i, ii;
	double eval, e1,e2;
	i = -1;
	e1=e2=eval = 0;
	for (ii = 0; ii < nj->fcount; ii++) {
		i = nj->ftp[ii].idx;
		switch (m[i].value_type) {
		case -1:
		case 0:
			e1+=ko[i] * (nj->ftp[ii].f_x);
			break;
		case 1:
			e2+=ko[i] * (nj->ftp[ii].f_x);
			break;
		case 2:
			eval += ko[i] * (nj->ftp[ii].f_x);
			break;
		default:
			break;
		}
	}
	return ((e1+eval)*nj->phb + (e2+eval)*nj->phe) / KOSC + nj->rem;
}

int compute_evals(double *ko, njac *nj, matrix_type *m, long start, long end, long *indir)
{
		long f, i;
		if (indir == NULL) {
#pragma omp parallel for
			for (i = start; i < end; i++) {
//			L0("eval:%d, %p\n",i, nj+i);
				nj[i].fxnew = njac_eval(ko, nj + i, m);
			}
		} else {
#pragma omp parallel for
			for (f = start; f < end; f++) {
				long i1 = indir[f];
//			L0("eval.:%d, %p\n",i1, nj+i1);
				nj[i1].fxnew = njac_eval(ko, nj + i1, m);
			}
		}
	return 0;
}

double compute_njac_error_dir(double *ko, njac *nj, long start, long stop, matrix_type *m, double K)
{
	double err;
//	njac *NN;

	err = 0;
	compute_evals(ko, nj, m, start, stop, NULL);
#pragma omp parallel for reduction(+:err)
	for (long i = start; i < stop; i++) {
		njac *NN = nj + i;
		err += comp_cost_vkx(NN->fxnew, NN->res, K);
	}
	return err;
}

int dump_njac(double *ko, njac *nj, long start, long stop, matrix_type *m)
{
	long i, n, ii;
	njac *NN;
	char buff[1024], buf2[256];
	;

	for (i = start; i < stop; i++) {
		NN = nj + i;
		sprintf(buff,
			"Pos:%ld, PHB %f, PHE %f, fxnew %f, fx0 %f, rem %f, count %d",
			i, NN->phb, NN->phe, NN->fxnew, NN->fx0, NN->rem,
			NN->fcount);
		for (n = 0; n < NN->fcount; n++) {
			ii = NN->ftp[n].idx;
			sprintf(buf2, ", idx %ld, type %d, val %d, fwb %d:%d",
				ii, m[ii].value_type, (m[ii].u[0]),
				NN->ftp[n].f_x, NN->ftp[n].f_x);
			strcat(buff, buf2);
		}
		NLOGGER_0("%s\n", buff);
	}
	return 0;
}

double compute_njac_error(double *ko, njac *nj, long start, long stop, matrix_type *m, long *indir, double K)
{
	double err;
	long i;
//	njac *NN;

	err = 0;
	compute_evals(ko, nj, m, start, stop, indir);
#pragma omp parallel for reduction(+:err)
	for (i = start; i < stop; i++) {
		njac *NN = nj + indir[i];
		err += comp_cost_vkx(NN->fxnew, NN->res, K);
	}
	return err;
}

// phase*fea/KOSC
int njac_init_cop(double *ko, njac *nj, matrix_type *m, long start, long len, ntuner_global *tun)
{
	long end, i;
	double x, y, r, x_hat, y_hat;

	end = start + len;

		long q, f;
		int16_t ft;

#pragma omp parallel for
		for (f = start; f < end; f++) {
			for (int ii = 0; ii < nj[f].fcount; ii++) {
				long i2 = nj[f].ftp[ii].idx;
				if (m[i2].tunable != 1) continue;
				double phase = ((m[i2].value_type) != 1) ? nj[f].phb : nj[f].phe;
//				nj[f].ftp[ii].cop = (double) phase*nj[f].ftp[ii].f_x / KOSC;
				nj[f].ftp[ii].cop = (double) 4.0*phase*nj[f].ftp[ii].f_x*tun->K / KOSC;
			}
		}
	return 0;
}

/*
 * Linear eval function eval= Ax+By+C, A is koef and x is feature value
 * includes ridge regularization, lambda describes how much to regularize
 * sigmoid wth e^ -. K is sigmoid koef.
 */

// K*phase*fea/KOSC - konstantni per pozice
// COPE, COPB verze
//
double njac_pderiv_cop(double koef, double cop, double res, double ev, double K)
{
	double der, O;
	O = exp(-K * ev);
	der = -(cop * O * (res - (2.0 / (O + 1.0)))) / pow((O + 1.0), 2.0);
	return der;
}

/*
 * koef values - values being tuned (total number of koefs is pcount)
 * feature values - features triggered by position
 * njac - description of each position - original evaluation, result of position, non mutable part of eval
 * matrix info - desc of koefs
 * tuner state
 * param counts - num of values tuned
 * start - index of first position
 * len - number of positions to 
 */

/*
 * xxx
 */

// compute parameter updates based on batch of positions. 
int njac_pupdate(double *ko, njac *nj, matrix_type *m, ntuner_run *state, int pcount, long start, long len, ntuner_global *tun, long *indir, int iter)
{
	// compute evals
	long end, i;
	double reg;

// compute evaluations for positions in the batch
	end = start + len;
	compute_evals(ko, nj, m, start, end, indir);

	reg=0;
// if BIAS is inplace we start at index 2
//	state[0].grad=state[1].grad=0;
//#pragma omp parallel for simd reduction(+:reg)
///	for (i = 2; i < pcount; i++) {
//		reg+= ko[i];
//		state[i].grad = 0;
//	}

// derivation according to a 
// 2*L*a-(4*K*x*e^(-K*x*a)*(r-2/(e^(-K*x*a)+1)))/(e^(-K*x*a)+1)^2
	double KFS=tun->reg_la;

// recompute gradients based on this batch
#pragma omp parallel for
	for (i = 0; i < pcount; i++) state[i].grad = 0;

	long q;
	int16_t ft;

#pragma omp parallel for simd schedule(static,1)
		for (q = start; q < end; q++) {
			long f = indir[q];
// derive position
			double EX=exp(-tun->K*nj[f].fxnew);
			double O=EX+1.0;
//			double STAT=((tun->K*EX*(nj[f].res-2.0/O))/ pow(O,2.0));
			double STAT=((EX*(nj[f].res-2.0/O))/ pow(O,2.0));
			
			for (long ii = 0; ii < nj[f].fcount; ii++) {
				long i2 = nj[f].ftp[ii].idx;
				if (m[i2].tunable != 1) continue;
				double cop = nj[f].ftp[ii].cop;
#pragma omp atomic update
				state[i2].grad += (KFS*ko[i2]-cop*STAT);
			}
		}

//#pragma omp parallel for
//	for (i = 0; i < pcount; i++) {
//		if (m[i].tunable != 1) continue;
//		state[i].grad /= len;
//	}
//}

#pragma omp parallel for
	for (i = 0; i < pcount; i++) {
	double x, y, r, x_hat, y_hat;
		if (m[i].tunable != 1) continue;
		state[i].grad /= len;
		switch (tun->method) {
		case 2:
			state[i].or2 = (state[i].or2 * tun->la2)
				+ (pow(state[i].grad, 2.0)) * (1.0 - tun->la2);
			y = sqrt(state[i].or2 + tun->small_c);
			r = 0.0 - state[i].grad / y;
			break;
		case 1:
			state[i].or2 = (state[i].or2 * tun->la2)
				+ (pow(state[i].grad, 2.0)) * (1.0 - tun->la2);
			x = sqrt(state[i].or1);
			y = sqrt(state[i].or2 + tun->small_c);
			r = 0.0 - state[i].grad * x / y;
			state[i].or1 = (state[i].or1 * tun->la1)
				+ (pow(r, 2.0)) * (1.0 - tun->la1);
			break;
		case 0:
			state[i].or2 = (state[i].or2 * tun->la2)
				+ (pow(state[i].grad, 2.0)) * (1.0 - tun->la2);
			state[i].or1 = (state[i].or1 * tun->la1)
				+ (pow(state[i].grad, 1.0)) * (1.0 - tun->la1);
			y_hat = state[i].or2 / (1.0 - pow(tun->la2, iter));
			x_hat = state[i].or1 / (1.0 - pow(tun->la1, iter));
			x = x_hat;
			y = sqrt(y_hat) + tun->small_c;
			r = 0.0 - x / y;
			break;
		default:
			abort();
		}
		// update koefs
//		dump_grads(state, pcount);
//		LOGGER_0("Tun ko[%d]:%.20f, %.20f * %.20f = %.20f\n", i, ko[i], r, tun->temp_step, r*tun->temp_step);
		ko[i] += (r * tun->temp_step);
		if (m[i].norm_f != NULL)
			ko[i] = (m[i].norm_f)(ko[i]);
	}
	return 0;
}

long file_load_driver(int long max, njac *state, matrix_type **m, personality *p, int pcount, int filters, CBACK, void *cdata)
{
	int long ix;
	long counter = 0;

//	max -= 100;

	board b;
	struct _ui_opt uci_options;
	struct _statistics s;

	b.uci_options = &uci_options;
	b.stats = &s;
	b.hs = allocateHashStore(HASHSIZE, 2048);
	b.hht = allocateHHTable();
	b.hps = NULL;

	pers_uni map;
	for(int f=0; f<NTUNL; f++) map.u[f]=f;

// paralelize 

	ix = 0;

#pragma omp parallel firstprivate(b) default(none) shared(counter, p, ix, max, state, filters, cdata, pcount, get_next, map)
#pragma omp parallel num_threads(1)
	{
		char fen[100];
		long ll, xx;
		int8_t res;
		int res2;
		matrix_type *mx;
		int stop;
		njac nj;
		board *bb;
		stacker s, *ss;
//		pers_small ps;

// instantiate personality to each thread
// and map it to tuner matrix
		bb = &b;
		bb->pers = init_personality(NULL);
		copyPers(p, bb->pers);
		to_matrix(&mx, bb->pers, &map);
		
		ss=&s;
	

//  stop=0;
		while ((counter < max) && (ix != -1)) {
#pragma omp critical
			{
				if ((counter < max) && (ix != -1)) {
					xx = get_next(fen, &res, cdata);
					if (xx == -1) ix = -1;
				} else
					ix = -1;
			}
//			L0("7\n");
			if (ix != -1) {
//			L0("8 %s\n", fen);
				setup_FEN_board_fast(bb, fen);
				// setup result, fix STM relative
//			L0("9\n");
				if (res >= 3 && res <= 5) {
				if (b.side == BLACK) {
						if (res == 3)
							res = 5;
						else if (res == 5)
							res = 3;
					}
					res -= 3;
				}
//			L0("10\n");
				check_mindex_validity(bb, 1);
				
// assign map to stacker here, reinitialize later
				ss->map=&map;
				nj.res = res;
				res2 = populate_njac(bb, &nj, bb->pers, mx,
					pcount, ss);
//				L0("FCount %d\n", res2);

//			write_personality((personality *) &(map.p), "zmap.xml");

// import only those that have more than X features, where exact value of X is in populate_njac
				if (res2 > 0 && res!=1) {
#pragma omp critical
					{
						ll = counter++;
						if (ll >= max) {
							ix = -1;
						} else {
//							nj.b=b;
							state[ll] = nj;
						}
					}
					if (ll % 1000 == 0)
						printf("ll:%ld\n", ll);
				} else {
//					if (nj.fcount > 0)
//						free(nj.ftp);
				}
			}
		}
		ix = -1;

		free_matrix(mx, pcount);
		free(bb->pers);
	}
	freeHHTable(b.hht);
	freeHashStore(b.hs);
	printf("Imported %ld positions\n", counter);
	LOGGER_0("Imported %ld positions\n", counter);
	return counter;
}

typedef struct __file_load_cb_data {
	FILE *handle;
	long offs;
	long nth;
	long len;
	long pos;
} file_load_cb_data;

long file_load_cback2(char *fen, int8_t *res, void *data)
{
	char *p;
	char buffer[2048], rrr[128], *xx, *name;
	file_load_cb_data *i;

	i = (file_load_cb_data*) data;
	while (!feof(i->handle)) {
		p=fgets(buffer, 2047, i->handle);
		if(p!=NULL){
		if ((i->pos % (i->nth)) == i->offs) {
			if (parseEPD(buffer, fen, NULL, NULL, NULL, NULL, rrr,
				NULL, &name) > 0) {
//				L0("buf %s, fen %s, rrr %s, name %s\n", buffer, fen, rrr, name);
				free(name);
				if (!strcmp(rrr, "1.0"))
					*res = 5;
				else if (!strcmp(rrr, "0.0"))
					*res = 3;
				else if (!strcmp(rrr, "0.5"))
					*res = 4;
				else {
					L0("%s\n", rrr);
					printf("2 Result parse error\n");
					abort();
				}
				i->len++;
				i->pos++;
				return i->len - 1;
			}
		}
		i->pos++;
		}
	}
	return -1;
}

long file_load_cback1(char *fen, int8_t *res, void *data)
{
	char *p;
	char buffer[2048], rrr[128], *xx, *name;
	file_load_cb_data *i;

	i = (file_load_cb_data*) data;
	while (!feof(i->handle)) {
		p=fgets(buffer, 2047, i->handle);
		if(p!=NULL){
//		L0("PP: %s\n", p);
		if ((i->pos % (i->nth)) == i->offs) {
			if (parseEPD(buffer, fen, NULL, NULL, NULL, NULL, rrr,
				NULL, &name) > 0) {
				L4("buf %s, fen %s, rrr %s, name %s\n", buffer, fen, rrr, name);
				free(name);
				if (!strcmp(rrr, "1-0"))
					*res = 2;
				else if (!strcmp(rrr, "0-1"))
					*res = 0;
				else if (!strcmp(rrr, "1/2-1/2"))
					*res = 1;
				else {
					L0("%s\n",rrr);
					printf("1 Result parse error\n");
					abort();
				}
//				L4("6 %s\n", rrr);
				i->len++;
				i->pos++;
				return i->len - 1;
			}
		}
		i->pos++;
		}
	}
	return -1;
}

int texel_file_load1(char *sts_tests[], long nth, long offs, file_load_cb_data *data)
{
	char filename[256];
	strcpy(filename, sts_tests[0]);
	if ((data->handle = fopen(filename, "r")) == NULL) {
		printf("File %s is missing\n", filename);
		return -1;
	}
	data->offs = offs;
	data->nth = nth;
	data->len = 0;
	data->pos = 0;
	return 0;
}

int texel_file_stop1(file_load_cb_data *data)
{
	file_load_cb_data *i;
	i = (file_load_cb_data*) data;
	printf("Loaded %ld positions from file\n", i->pos);
	LOGGER_0("Loaded %ld positions from file\n", i->pos);
	fclose(data->handle);
	return 1;
}

int koef_load(double **ko, matrix_type *m, personality *p, int pcount)
{
pers_uni *pp;

	pp = (pers_uni*) p;
	if(*ko==NULL) *ko=malloc(sizeof(double)*pcount);
	if(*ko==NULL) return 0;
	for(int f=0;f<pcount;f++) {
		(*ko)[f]=pp->u[m[f].u[0]];
//		L0("Koef %d:%d=>%d\n", f, m[f].u[0], pp->u[m[f].u[0]]);
	}
	return 1;
}

int print_koefs(double *ko, int pcount)
{
	int i;
	NLOGGER_0("Koefs: ");
	for (i = 0; i < pcount; i++) {
		NLOGGER_0("xk %d:%f, ", i, ko[i]);
	}
	NLOGGER_0("\n");
	return 0;
}

int copy_koefs(double *from, double *to, int pcount)
{
	int f;
	for (f = 0; f < pcount; f++)
		to[f] = from[f];
	return 0;
}

#define NONUPDATES_MAX 10
#define DIFF 1E-08

void texel_loop_njac(ntuner_global *tuner, double *koefs, char *base_name, njac *ver, long vlen)
{
	ntuner_run *state;
	double *best;

	unsigned long long int totaltime;
	struct timespec start, end;

	int gen, perc, ccc;
	long *rnd, *rids;
	char nname[1024];
	double fxh, fxh2 = 0, fxh3, fxb, t, vxh, vxh3;

	long i, l;
	int nonup=0;

	if (ver == NULL)
		vlen = 0;
	allocate_ntuner(&state, tuner->pcount);
	init_ntuner_jac(state, tuner->m, tuner->pcount);

	rids = rnd = NULL;
	// randomization init
	rnd = malloc(sizeof(long) * tuner->len);
	rids = malloc(sizeof(long) * tuner->len);

	best = (double*) malloc(sizeof(double) * tuner->pcount);

	for (i = 0; i < tuner->len; i++) {
		rnd[i] = i;
		rids[i] = i;
	}

	srand(time(NULL));
	switch (tuner->method) {
	case 2:
		t = tuner->rms_step;
		break;
	case 1:
		t = tuner->adadelta_step;
		break;
	case 0:
		t = tuner->adam_step;
		break;
	default:
		abort();
	}
//	tuner->temp_step = t * (tuner->batch_len);
	tuner->temp_step = t;
	copy_koefs(koefs, best, tuner->pcount);
	
	// looping over testing ...
	// compute loss with current parameters
//	fxh = compute_njac_error_dir(koefs, tuner->nj, 0, tuner->len, tuner->m,
//		tuner->K) / tuner->len;
	fxh = vxh = vxh3 = 9999;
//	if (vlen != 0)
//		vxh = vxh3 = compute_njac_error_dir(koefs, ver, 0, vlen,
//			tuner->m, tuner->K) / vlen;
	for (gen = 1; gen <= tuner->generations; gen++) {

#if 1
int r1, r2, rrid;
// randomize for each generation
	  for (i = 0; i < tuner->len; i++)
		{
		  r1 = rnd[i];
		  if(r1!=i) continue;
		  rrid = rand() % tuner->len;
		  r2 = rnd[rrid];
		  rnd[i] = r2;
		  rnd[rrid] = r1;
		  rids[r2] = i;
		  rids[r1] = rrid;
		}
#endif

// print_koefs(koefs, tuner->pcount);

		{
			readClock_wall(&start);
			sprintf(nname, "%s_%ld_%d.xml", base_name,
				tuner->batch_len, gen);
			// compute loss prior tuning
			LOGGER_0(
				"GEN %d, bl %ld, Iloss=%.10f JAC\n",
				gen, tuner->batch_len, fxh);
			// tuning part
			// in minibatches
			ccc = 1;
			i = 0;
			perc = 10;
			while (tuner->len > i) {
				// set new batch
				l = ((tuner->len - i) >= tuner->batch_len) ?
					tuner->batch_len : tuner->len - i;
				// update parameters based on this batch
				njac_pupdate(koefs, tuner->nj, tuner->m, state,
					tuner->pcount, i, l, tuner, rnd, ccc);
				ccc++;
				if ((i * 100 / tuner->len) > perc) {
					fflush(stdout);
					perc += 10;
				}
				i += l;
			}
			// compute loss based on new parameters
			fxh3 = compute_njac_error_dir(koefs, tuner->nj, 0, tuner->len, tuner->m, tuner->K);
			fxh2 = fxh3 / tuner->len;

//		  koefs_to_matrix(koefs, tuner->m, tuner->pcount);
//		  compute_njac_test_dir(koefs, tuner->nj, tuner->pi, 0, tuner->len, tuner->m, tuner->K);

			if (vlen != 0) vxh3 = compute_njac_error_dir(koefs, ver, 0, vlen, tuner->m, tuner->K) / vlen;
			readClock_wall(&end);
//			totaltime = diffClock (start, end);
			printf("GEN %d, bl %ld, Floss=%.20f:%.20f, VLoss %.20f:%.20f", gen, tuner->batch_len, fxh2, fxh, vxh3, vxh);
			LOGGER_0("GEN %d, bl %ld, Floss=%.20f:%.20f, VLoss %.20f:%.20f", gen, tuner->batch_len, fxh2, fxh, vxh3, vxh);
			if (((fxh-fxh2)>=DIFF) && (((vxh-vxh3) >=DIFF) || (vlen == 0))) {
				fxh = fxh2;
				vxh = vxh3;
				printf(" Update");
				NLOGGER_0(" Update");
				nonup=0;
				copy_koefs(koefs, best, tuner->pcount);
//					print_koefs(koefs, tuner->pcount);
				koef_to_pers(koefs, tuner->m, tuner->pi, tuner->pcount);
				write_personality(tuner->pi, nname);
// verify classical evaluation vs tuned
			} else if(nonup>=NONUPDATES_MAX) break; else nonup++;
			printf(" \n");
			NLOGGER_0(" \n");
		}
	}

	copy_koefs(best, koefs, tuner->pcount);
	free(best);
	free_ntuner(state);
	if (rnd != NULL)
		free(rnd);
	if (rids != NULL)
		free(rids);
}

void texel_test(char *str)
{

int thr;
	thr=atoi(str);
	if(thr>0) omp_set_num_threads(thr-1);
//	omp_set_num_threads(1);

	int i;
	double fxb1, fxb2, lambda, K, *koefs, KL, KH, Kstep, x, *initk;
	ntuner_global ntun;
	file_load_cb_data tmpdata;
	njac *vnj;
	long vlen;
	pers_uni map;

// batch driver
// input file, output prefix, personality seed
	char *inpf[] = { "../texel/libig3-res.book", "../texel/lichess-quiet.txt", "../texel/ccrl.epd",
		"../texel/quiet-labeled.epd" };
	char *outf[] = { "../texel/libig3-res", "../texel/lichess-quiet", "../texel/ccrl",
		"../texel/quiet-labeled" };

	L0("Tuner start\n");
	printf("Tuner start\n");

	ntun.max_records = 2000000;
// load personality
		ntun.pi = (personality*) init_personality("../texel/pers.xml");
// put references to tuned params into structure  
	for(int f=0; f<NTUNL; f++) map.u[f]=f;
		ntun.pcount = to_matrix(&ntun.m, ntun.pi, &map);
// allocate koeficients array and setup values from personality loaded/matrix...
		koefs=NULL;
		if(koef_load(&koefs, ntun.m, ntun.pi, ntun.pcount)==0) abort();
// allocate njac records + num of threads
		if (allocate_njac(ntun.max_records+20, ntun.pcount, &ntun.nj) == 0)
			abort();

	initk = (double*) malloc(sizeof(double) * ntun.pcount);
	copy_koefs(koefs, initk, ntun.pcount);

		/*
		 * setup verification
		 */

		vnj = NULL;
		vlen = 0;

#if 1
  if (allocate_njac (10000000, ntun.pcount, &vnj) == 0)
	abort ();
//  ntun.nth = 1;
//  texel_file_load1 (&(inpf[8]), 1, 0, &tmpdata);
//	vlen=file_load_driver (8000000, vnj, &ntun.m, ntun.pi,
//					ntun.pcount, 0, file_load_cback1, &tmpdata);
//  texel_file_stop1 (&tmpdata);
#endif 

char *files1[] = { "../texel/quiet-labeled.epd" };
int idxs[] = { 1, 0, 2, 3 };
int vers[] = { 3, 3, 3, 0 };
int vers_old=-1;
int lll;
	for (int ll = 0; ll < 1; ll++) {
		lll=idxs[ll];
		L0("Input %s, verification %s\n", (inpf[lll]), (inpf[vers[ll]]));
		printf("Input %s, verification %s\n", (inpf[lll]), (inpf[vers[ll]]));
#if 1
//	if(vlen>0) {
		if(vers[ll]!=vers_old) {
			ntun.nth = 1;
			texel_file_load1 (&(inpf[vers[ll]]), 1, 0, &tmpdata);
			vlen=file_load_driver (8000000, vnj, &ntun.m, ntun.pi,
						ntun.pcount, 0, file_load_cback1, &tmpdata);
			texel_file_stop1 (&tmpdata);
			vers_old=vers[ll];
//		}
	}
#endif
		char outpf[1024];
		ntun.generations = 50000;
		ntun.batch_len = 16384;
//		ntun.batch_len = 32768;
		ntun.records_offset = 0;
		ntun.nth = 1;
		ntun.small_c = 1E-9;
		ntun.rms_step = 0.000020;
		ntun.adam_step = 2.0;
		ntun.K = 0.00004;
		ntun.la1 = 0.9;
		ntun.la2 = 0.999;
		ntun.method = 0;
		K = 0.00072323115;
// LAMBDA
//		lambda = 0;
		lambda = 1E-10;

// initiate files load
		texel_file_load1(&(inpf[lll]), ntun.nth, ntun.records_offset,
			&tmpdata);
// load each position into njc
		ntun.len = file_load_driver(ntun.max_records, ntun.nj, &ntun.m,
			ntun.pi, ntun.pcount, 0, file_load_cback1, &tmpdata);
// finish loading process
		texel_file_stop1(&tmpdata);

	copy_koefs(initk, koefs, ntun.pcount);

// setup K
		KL = 0.0;
		KH = 1.0;
		Kstep = 0.05;
		fxb1 = compute_njac_error_dir(koefs, ntun.nj, 0, ntun.len,
			ntun.m, KL) / ntun.len;
		for (i = 0; i < 10; i++) {
			x = KL - Kstep;
			while (x < KH) {
				x += Kstep;
				fxb2 = compute_njac_error_dir(koefs, ntun.nj, 0,
					ntun.len, ntun.m, x) / ntun.len;
				LOGGER_0("K computation i:%d K: %.30f, loss= %.30f",
					i, x, fxb2);
				printf("K computation i:%d K: %.30f, loss= %.30f\n",
					i, x, fxb2);
				if (fxb2 <= fxb1) {
					fxb1 = fxb2;
					KL = x;
					K = x;
					NLOGGER_0(" Update\n");
				} else
					NLOGGER_0("\n");
			}
			KH = KL + Kstep;
			KL -= Kstep;
			Kstep /= 10.0;
		}
		if (K < 1E-6)
			K = 0.00072323115;
//  K=0.00072323115;

		ntun.K = K * 1.0;
		LOGGER_0("Using K=%.30f\n", K);
		printf("Using K=%.30f\n", K);

// update cop
//		njac_init_cop(koefs, ntun.nj, ntun.m, 0, ntun.len, &ntun);

		int loo;
		char nname[256];

		loo=0;
		copy_koefs(initk, koefs, ntun.pcount);
		njac_init_cop(koefs, ntun.nj, ntun.m, 0, ntun.len, &ntun);
		ntun.reg_la = 2*lambda;
//		for (loo = 0; loo < 1; loo++) {
			LOGGER_0("Lambda %e\n", lambda);
//			ntun.reg_la = 2*(pow(10, -loo));
//			LOGGER_0("Lambda %e\n", ntun.reg_la);
//			koefs_to_matrix(koefs, ntun.m, ntun.pcount);
//			pers_to_koef(koefs, ntun.m, ntun.pi, ntun.pcount);

// run tuner itself
			L0("Tune prefix %s, version %d,  stepping %d\n", (outf[lll]), loo, ntun.adam_step);
			sprintf(nname, "%s_%s_%d_", outf[lll], "ptest_ptune", loo);
			texel_loop_njac(&ntun, koefs, nname, vnj, vlen);
			ntun.adam_step /=10;

//		}
		free_njac(ntun.nj, ntun.len);
	}
	if (vnj != NULL) {
		free_njac(vnj, vlen);
		free(vnj);
	}
	if(ntun.nj!=NULL) free(ntun.nj);
	free(ntun.pi);
	free_matrix(ntun.m, ntun.pcount);
	free(koefs);
	L0("Tuner stop\n");
	printf("Tuner stop\n");
}
