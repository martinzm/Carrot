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

#ifndef EVALUATE_H
#define EVALUATE_H

#include "bitmap.h"
#include "pers.h"

int eval_king_checks(board const *b, king_eval *ke, personality const *p, int side);
int eval_king_checks_oth(board *b, king_eval *ke, personality *p, int side, int from);
int eval_king_checks_ext(board const *b, king_eval *ke, personality const *p, int side, int from);
int eval_king_checks_n(board *b, king_eval *ke, personality *p, int side);
int eval_king_checks_n_full(board *b, king_eval *ke, personality *p, int side);
int eval_ind_attacks(const board *const b, king_eval *ke, personality *p, int side, int from);
int eval_king_checks_ext_adapt(board const *b, king_eval *ke, BITVAR filter, int side, int from);



int eval(board *b, attack_model *a, personality const *p, stacker *st);
uint8_t eval_phase(board const *b, personality const*);
int mat_info(struct materi[]);
int mat_faze(uint8_t*);
int is_draw(board *b, attack_model *a, personality *p);
int EvaluateOwn(board *b, int pos);

int eval_king_checks_all(board *b, attack_model *a);

int check_mindex_validity(board *b, int force);
int premake_pawn_model(board const *b, attack_model const *a, hashPawnEntry **hhh, personality const *p, stacker *st);

int mpsq_eval(board *b, attack_model *a, personality *p);
int psq_eval(board *b, attack_model *a, personality *p);
int lazyEval(board *b, attack_model *a, int alfa, int beta, int side, int ply, int depth, personality const *p, int *fullrun);

int getlazyEval(board *, personality const *);
int get_material_eval_f(board*, personality*);
int init_eval_run(board *, personality const *);

int meval_table_gen(meval_t*, personality*, int);
int meval_t_gen(personality*);

int MVVLVA_gen(int table[ER_PIECE + 2][ER_PIECE+1], _values Values);
int SEE(board *b, MOVESTORE m);
int SEEx(board *b, MOVESTORE m);
int SEE0(board *b, int to, int side, int val);
int PSQSearch(int, int, int, int, int, personality*);

#define NW_MI 1
#define NB_MI (NW_MI*3)
#define BWL_MI (NB_MI*3)
#define BWD_MI (BWL_MI*2)
#define BBL_MI (BWD_MI*2)
#define BBD_MI (BBL_MI*2)
#define RW_MI (BBD_MI*2)
#define RB_MI (RW_MI*3)
#define QW_MI (RB_MI*3)
#define QB_MI (QW_MI*2)
#define PW_MI (QB_MI*2)
#define PB_MI (PW_MI*9)
#define XX_MI (PB_MI*9)

#define MATidx(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb)\
 (PW_MI*pw+PB_MI*pb+NW_MI*nw+NB_MI*nb+BWL_MI*bwl+BBL_MI*bbl+BWD_MI*bwd+BBD_MI*bbd+QW_MI*qw+QB_MI*qb+RW_MI*rw+RB_MI*rb)

#define MAXMAT_IDX MATidx(8,8,2,2,1,1,1,1,2,2,1,1)

/*
 * pro mindex2 ud�l�me max hodnoty figur v po�ad� N,B,R,Q,P
 */

#define NW_MI2 1LL
#define NB_MI2 (NW_MI2*16)
#define BWL_MI2 (NB_MI2*16)
#define BBL_MI2 (BWL_MI2*16)
#define BWD_MI2 (BBL_MI2*16)
#define BBD_MI2 (BWD_MI2*16)
#define RW_MI2 (BBD_MI2*16)
#define RB_MI2 (RW_MI2*16)
#define QW_MI2 (RB_MI2*16)
#define QB_MI2 (QW_MI2*16)
#define PW_MI2 (QB_MI2*16)
#define PB_MI2 (PW_MI2*16)
#define XX_MI2 (PB_MI2*16)

#define MATidx2(pw,pb,nw,nb,bwl,bwd,bbl,bbd,rw,rb,qw,qb) (pw*PW_MI2+PB_MI2*pb+NW_MI2*nw+NB_MI2*nb+BWL_MI2*bwl+BBL_MI2*bbl+BWD_MI2*bwd+BBD_MI2*bbd+QW_MI2*qw+QB_MI2*qb+RW_MI2*rw+RB_MI2*rb)

typedef enum {
	NO_INFO = 0, INSUFF, UNLIKELY, DIV2, DIV4, DIV8
} score_types;

#define isMATE(x) ((x>=MATEMIN && x<=MATEMAX) ? 1 : ((x<=-MATEMIN && x>=-MATEMAX) ? -1 : 0))
#define isMATE2(x) ((-MATEMIN<x && x<MATEMIN) ? 0 : (x>0) ? 1 : -1)
/* 
 fixes mate score as seen from level 0
 x = value
 y = matescore
 z = depth
 */

#define MATESCORE (MATEMAX)
#define FixMateScore2(x,z) ((x>0) ? (x-z) : (x+z) )
#define DecMateScore(x) ((x>0) ? (x--) : (x++))
#define GenerateMATESCORE(ply) (MATESCORE-ply)
// dist in plies
#define GetMATEDist(x) ((x>0) ? MATESCORE-x : MATESCORE+x)

#define PlaceValueWon 5
#define PlaceValueDef 10
#define MobilitySquare 1

#define isIsolatedP 0x1
#define isPassedP 0x2
#define isBlockedP 0x4
#define isDoubledP 0x8
#define hasLeftP 0x10
#define isConnectedP 0x20
#define isBackwardP 0x40

#endif
