/* Carrot is a UCI chess playing engine by Martin Žampach.
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

#include "search.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>

#include "attacks.h"
#include "evaluate.h"
#include "globals.h"
#include "hash.h"
#include "movgen.h"
#include "openings.h"
#include "stats.h"
#include "sys/time.h"
#include "ui.h"
#include "utils.h"

#define MOVES_RET_MAX 64
#define moves_ret_update(x) if(x<MOVES_RET_MAX-1) moves_ret[x]++; else  moves_ret[MOVES_RET_MAX-2]++

int oldPVcheck;

int DEPPLY = 30;

int inPV;
unsigned long long COUNT;

#if 1
int TRIG;
#endif

/*
 * tree->tree[ply][absdep].
 * absdep = ply + depth
 * tree[ply][ply+0] contains info about bestmove from level at ply
 * tree[ply][ply+1] first move analyzed
 * tree[ply][ply+2] second move analyzed 
 */

void store_PV_tree(tree_store *tree, tree_line *pv)
{
	int f;
	pv->score = tree->score = tree->tree_board.bestscore =
		tree->tree[0][0].score;
	tree->tree_board.bestmove = tree->tree[0][0].move;
	
	for (f = 0; f <= MAXPLY; f++) {
		pv->line[f] = tree->tree[0][f];
	}
}

void restore_PV_tree(tree_line *pv, tree_store *tree)
{
	int f;
	for (f = 0; f <= MAXPLY; f++) {
		tree->tree[0][f] = pv->line[f];
	}
}

void copyTree(tree_store *tree, int level)
{
	int f;
	if (level > MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	for (f = level + 1; f <= MAXPLY; f++) {
		tree->tree[level][f] = tree->tree[level + 1][f];
//		if((tree->tree[level][f].move & (~CHECKFLAG)) == NA_MOVE) break;
	}
}

void installHashPV(tree_line *pv, board *b, int depth, struct _statistics *s)
{
	hashEntry h;
	UNDO u[MAXPLY + 1];
	int f, l;
	f = 0;
	l = 1;

	while ((f < depth) && (l != 0)) {
		l = 0;
		switch (pv->line[f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case NULL_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
		case ERR_NODE:
			break;
		default:
			storeHashX(b->hs, pv->line[f].key, pv->line[f].pld, pv->line[f].ver, s);
		}
		f++;
	}
}

/*
 * Triangular storage for PV
 * tree[ply][ply] contains bestmove at ply
 * tree[ply][ply+N] should contain bestmove N plies deeper for PV from bestmove at ply
 * tree[0][0+..] contains PV from root
 */

void sprintfPV(tree_store *tree, int depth, char *buff)
{
	UNDO u[MAXPLY + 1];
	int f, mi, ply, l;
	char b2[1024];

	buff[0] = '\0';
	depth = MAXPLY + 1;
	l = 1;
	f = 0;
	while ((f <= depth) && (l != 0)) {
		l = 0;
		switch (tree->tree[0][f].move & (~CHECKFLAG)) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
		case ERR_NODE:
			break;
		case NULL_MOVE:
		default:
			sprintfMove(&(tree->tree_board), tree->tree[0][f].move,
				b2);
			strcat(buff, b2);
			if (tree->tree[0][f + 1].move != MATE_M)
				strcat(buff, " ");
			MakeMove(&(tree->tree_board),
				tree->tree[0][f].move, u+f);
			l = 1;
			break;
		}
		f++;
	}
	if (l == 0)
		f--;
	f--;
	while (f >= 0) {
		UnMakeMove(&(tree->tree_board), u+f);
		f--;
	}

	if (isMATE(tree->tree[0][0].score)) {
		ply = GetMATEDist(tree->tree[0][0].score);
		if (ply == 0)
			mi = 1;
		else {
			mi = tree->tree_board.side == WHITE ? (ply + 1) / 2 :
				(ply / 2) + 1;
		}
	} else
		mi = -1;

	if (mi == -1)
		sprintf(b2, "EVAL:%d", tree->tree[0][0].score);
	else {
		if (isMATE(tree->tree[0][0].score) < 0)
			mi = 0 - mi;
		sprintf(b2, "MATE in:%d", mi);
	}
	strcat(buff, b2);
}

void printPV_simple(board *b, tree_store *tree, int depth, int side, struct _statistics *s, struct _statistics *s2)
{
	int f, mi, xdepth, ply;
	char buff[1024], b2[1024];
	unsigned long long int tno;

	buff[0] = '\0';
	xdepth = depth;
	xdepth = MAXPLY + 1;
	for (f = 0; f <= xdepth; f++) {
		switch (tree->tree[0][f].move & (~CHECKFLAG)) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
			f = xdepth + 1;
			break;
		case NULL_MOVE:
		default:
			sprintfMoveSimple(tree->tree[0][f].move, b2);
			strcat(buff, b2);
			strcat(buff, " ");
			break;
		}
	}
	/*
	 DIST 0 - netazeno
	 DIST 1 - prvni tahl
	 DIST 2 - tahli oba
	 tah je pocitan od bileho

	 zacne cerny a jsou dva pultahy ==> dva tahy?

	 zacal bily, tah
	 0	0

	 1	1
	 2	1
	 3	2
	 4	2
	 5	3

	 (ply+1)/2

	 zacal cerny, tah
	 0	0

	 1	1
	 2	2
	 3	2
	 4	3
	 5	3

	 (ply+2)/2 ; ply!=0

	 *
	 */

	if (isMATE(tree->tree[0][0].score)) {
		ply = GetMATEDist(tree->tree[0][0].score);
		if (ply == 0)
			mi = 1;
		else {
			mi = tree->tree_board.side == WHITE ? (ply + 1) / 2 :
				(ply / 2) + 1;
		}
	} else
		mi = -1;

	tno = readClock() - b->run.time_start;
	
	if (mi == -1) {
		sprintf(b2,
			"info depth %d seldepth %d nodes %lld score cp %d time %lld pv ",
			depth, s2->depth_max,
			s2->positionsvisited + s2->qposvisited,
			tree->tree[0][0].score / 10, tno);
		strcat(b2, buff);
	} else {
		if (isMATE(tree->tree[0][0].score) < 0)
			mi = 0 - mi;
		sprintf(b2,
			"info depth %d seldepth %d nodes %lld score mate %d time %lld pv ",
			depth, s2->depth_max,
			s2->positionsvisited + s2->qposvisited, mi, tno);
		strcat(b2, buff);
	}
	tell_to_engine(b2);
	LOGGER_1("%s\n",b2);
}

void printPV_simple_act(board *b, tree_store *tree, int depth, int side, struct _statistics *s, struct _statistics *s2)
{
	int f, xdepth;
	char buff[1024], b2[1024];

	strcpy(buff, "Line: ");
	xdepth = depth;
	xdepth = MAXPLY + 1;
	for (f = 0; f <= xdepth; f++) {
		switch (tree->tree[f][f].move & (~CHECKFLAG)) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
			f = xdepth + 1;
			break;
		case NULL_MOVE:
		default:
			sprintfMoveSimple(tree->tree[f][f].move, b2);
			strcat(buff, b2);
			if (tree->tree[f][f].move & CHECKFLAG)
				strcat(buff, "+ ");
			else
				strcat(buff, " ");
			break;
		}
	}
	LOGGER_0("%s\n", buff);
}

// called inside search
int update_status(board *b)
{
	long long int tnow, tpsd, nrun, npsd;
	long long int passed, frem;
	LOGGER_4("Nodes at check %d, mask %d, crit %d\n",b->stats->nodes, b->run.nodes_mask, b->run.time_crit);
	b->search_abort=engine_stop;
	if (b->run.time_crit == 0) return 0;
	if (b->uci_options->nodes > 0) {
		if (b->stats->positionsvisited >= b->uci_options->nodes) {
		b->search_abort=3;
		return 3;
		}
	}
//tnow milisekundy
// movetime je v milisekundach
//
	tnow = readClock();
	passed = (tnow - b->run.time_start) + 1;

	if ((b->run.time_crit <= passed)) {
		LOGGER_4("INFO: Time out loop - time_move CRIT, move: %d, crit: %d, elaps %lld, left %lld, crit left %lld, dif %d\n", b->run.time_move,b->run.time_crit,passed, b->run.time_move-passed,b->run.time_crit-passed, b->search_dif );
		if (b->depth_run <= 1) {
			LOGGER_3("INFO: Time out ignored\n" );
			return 0;
		}
		b->search_abort=3;
		return 3;
	}

	frem = (b->uci_options->movetime == 0) ?
		Min(
			(long long int )(700 / (1140 - b->search_dif * 1.0)
				* b->run.time_move * 1.0), b->run.time_crit)
			- tnow + b->run.time_start : b->run.time_crit-passed; 

	if (b->uci_options->movetime == 0) {
		if ((b->depth_run > 0) && (frem < 0) && (b->idx_root < 1)) {
			LOGGER_4("INFO: Time out loop - move EASY, frem %lld, move: %d, crit: %d, elaps %lld, left %lld, crit left %lld, dif %d\n", frem, b->run.time_move,b->run.time_crit,passed, b->run.time_move-passed,b->run.time_crit-passed, b->search_dif );
			b->search_abort=32;
			return 32;
		}
	}
	npsd = b->stats->nodes - b->run.nodes_at_iter_start + 1;
	if ((b->depth_run > 3) && (npsd > 512) && (frem > 0)) {

		// modify check counter
		tpsd = tnow - b->run.iter_start + 1;
		nrun = (frem) * npsd / (tpsd + 1);
		if (nrun > 0) {
			while (((b->run.nodes_mask + 1) * 4) < nrun) {
				b->run.nodes_mask *= 2;
				b->run.nodes_mask++;
			}
			nrun /=2;
			while ((b->run.nodes_mask + 1) > nrun) {
				b->run.nodes_mask /= 2;
			}
		}
		b->run.nodes_mask |= 7;
		LOGGER_4("nodes_mask NEW: %lld\n", b->run.nodes_mask);
	}
	return engine_stop;
}

// called after iteration
int search_finished(board *b)
{

	unsigned long long tnow, tpsd, npsd;
	unsigned long long trun, remain;
	long long int frem;

	if (engine_stop) {
		LOGGER_4("SearchF engine_stop\n");
		b->search_abort=9999;
		return 9999;
	}

	tnow = readClock();
	tpsd = tnow - b->run.iter_start + 1;
	trun = (tnow - b->run.time_start);
	remain = (b->run.time_move - trun);
// difficulty related move time
	frem =
		Min(
			(long long int )(700 / (1140 - b->search_dif * 1.0)
//			(long long int )((b->search_dif+110)/220
				* b->run.time_move * 1.0), b->run.time_crit)
			- trun;

// moznosti ukonceni hledani
	if ((b->uci_options->nodes > 0)
		&& (b->stats->positionsvisited >= b->uci_options->nodes)){
		b->search_abort=1;
		LOGGER_4("SearchF NODES stop\n");
		return 1;
		}
// pokud ponder nebo infinite, tak hledame dal
	if ((b->uci_options->infinite == 1) || (b->uci_options->ponder == 1)
		|| (b->uci_options->nodes > 0))
		goto FINISH;
	if (b->uci_options->movetime != 0) {
	  if((tnow-b->run.time_start) > b->run.time_crit) return 11;
	  else goto FINISH;
	}

	if (b->run.time_crit <= trun) {
		LOGGER_4("INFO: Time out movetime - CRIT, plan: %lld, crit: %lld, iter: %lld, left: %llu, elaps: %lld\n", b->run.time_move, b->run.time_crit, tpsd, remain, (tnow-b->run.time_start));
		b->search_abort=2;
		return 2;
	}

// deal with variable time for move
	LOGGER_4("INFO: Time out movetime - SEARCH finished, remain %d, frem %lld\n", remain, frem);
// normally next iteration needs 3times more time, than just finished one.

	if ((frem * 4) < (tpsd * 11)) {
		LOGGER_4("INFO: Time out movetime - RUN1, plan: %lld, crit: %lld, iter: %lld, left: %llu, elaps: %lld, frem %lld: %lld < %lld\n", b->run.time_move, b->run.time_crit, tpsd, remain, (tnow-b->run.time_start), frem, frem*b->stats->ebfnodespri, tpsd*b->stats->ebfnodes);
		b->search_abort=33;
		return 33;
	}
	if ((frem * 100) < ((trun + frem) * 55)) {
		LOGGER_4("INFO: Time out movetime - RUN2, plan: %lld, crit: %lld, iter: %lld, left: %llu, elaps: %lld, frem %lld\n", b->run.time_move, b->run.time_crit, tpsd, remain, (tnow-b->run.time_start), frem);
		b->search_abort=34;
		return 34;
	}

	FINISH:
	b->run.iter_start = tnow;
	b->run.nodes_at_iter_start = b->stats->nodes;
	return 0;
}
/*
 * no check
 * hash failed low
 * has no null
 * mat < beta
 * depth >= 2
 * other piece then pawn for side to move
 * 
 */

int can_do_NullMove(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side)
{

	personality const *p;

	if (b->mindex_validity != 0) {
		p = b->pers;
#if 0
		pieces=6*p->mat_info[b->mindex].m[b->side][QUEEN]
			  +6*p->mat_info[b->mindex].m[b->side][ROOK]
			  +6*p->mat_info[b->mindex].m[b->side][KNIGHT]
			  +6*p->mat_info[b->mindex].m[b->side][BISHOP]
			  +1*p->mat_info[b->mindex].m[b->side][PAWN];
		if(pieces<6) return 0;
#endif
		if ((GT_M(b, p, b->side, PIECES, 0) <= 0)
			&& (GT_M(b, p, b->side, PAWN, 0) < 6))
			return 0;
	}
	return 1;
}

/*
 *  do not reduce when
 *  - remaining depth is too low
 *  - in PVS
 *  - inCheck
 *  - good or neutral capture + promotions
 *  - move gives check (only quiet move others covered by above)
 *  -
 *  funkce je volana po make_move, takze side je strana co udelala tah
 *  b->side je strana na tahu
 */
/*
 * DEPTH klesa do 0, ply roste
 */

/*
 * LMR doesnt reduce captures, hashmove, killers, non captures with good history
 * checks not reduced normally, no pawn
 */
int can_do_LMR(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side, move_entry *move)
{

	int8_t from, movp, ToPos, rank;
	int prio;

	from = UnPackFrom(move->move);
	movp = b->pieces[from & PIECEMASK];
	if (movp == PAWN) {
		rank=getRank(from);
		if(((side==WHITE)&&(rank==RANKi7))||((side==BLACK)&&(rank==RANKi2))) return 0;
	}
	ToPos = UnPackTo(move->move);
	prio = checkHHTable(b->hht, side, movp, ToPos);
	if (prio > 0)
		return 0;
	return 1;
}

int position_quality(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side)
{
	BITVAR x;
	int from, p1, pa, opside;
	int cc, quality, stage;

	int threshold[3][4] = { { 1, 2, 2, 4 }, { 2, 3, 3, 6 }, { 3, 4, 4, 8 } };

	quality = 0;
	p1 = GT_M(b, b->pers, side, KNIGHT, 0) * 6
		+ GT_M(b, b->pers, side, BISHOP, 0) * 6
		+ GT_M(b, b->pers, side, ROOK, 0) * 9
		+ GT_M(b, b->pers, side, QUEEN, 0) * 18;
	pa = p1;
	
	if (pa > 30)
		return 1;
	if (pa >= 25)
		stage = 0;
	else if (pa >= 20)
		stage = 1;
	else if (pa >= 10)
		stage = 2;
	else
		return 1;
	
// get mobility of side to move for pieces 
// rook
	x = (b->maps[QUEEN] & b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(QueenAttacks(b, from) & (~b->norm));
		if (cc < threshold[stage][3])
			goto FIN;
		ClrLO(x);
	}
	x = (b->maps[ROOK] & b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(RookAttacks(b, from) & (~b->norm));
		if (cc < threshold[stage][2])
			goto FIN;
		ClrLO(x);
	}
	x = (b->maps[BISHOP] & b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(BishopAttacks(b, from) & (~b->norm));
		if (cc < threshold[stage][1])
			goto FIN;
		ClrLO(x);
	}
	x = (b->maps[KNIGHT] & b->colormaps[side]);
	while (x) {
		from = LastOne(x);
		cc = BitCount(KnightAttacks(b, from) & (~b->norm));
		if (cc < threshold[stage][0])
			goto FIN;
		ClrLO(x);
	}
	quality = 1;
	FIN: return quality;
}

/*
 * Quiescence looks for quiet positions, ie where no checks, no captures etc take place
 * 
 *
 */

int QuiesceCheckN(board *b, int talfa, int tbeta, int depth, int ply, int side, tree_store *tree, int checks, const attack_model *tolev)
{
	move_cont mvs;
	attack_model ATT, *att;

	move_entry *m, mdum = { MATE_M, 0, 0 - GenerateMATESCORE(ply) }, *mb;
	int opside = Flip(side);

	UNDO u;
	DEB_SE(char b2[256];)
//	char b3[256];
	
	int aftermovecheck = 0;

	b->stats->nodes++;
	b->stats->qposvisited++;

	if (!(b->stats->nodes & b->run.nodes_mask)) {
		if(update_status(b)!=0) {
			return 0;
		}
	}

	if (b->stats->depth_max < ply) {
		b->stats->depth_max = ply;
		if (ply >= MAXPLY - 1)
			return tbeta;
	}
	
	att = &ATT;
	mb = &mdum;

// eval_king_check of side is done on level above, we just copying it
	att->ke[side] = tolev->ke[side];

// generating attacks from opposite to be sure not to move King to check
	att->att_by_side[opside] = KingAvoidSQ(b, att, opside);

	LOGGER_SE("%*d, *C , QCQC, amove ch:?, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);

	sortMoveListNew_Init(b, att, &mvs);
	while ((getNextMove(b, att, &mvs, ply, side, 1, &m, tree) != 0)
		&& (b->search_abort == 0)) {

#if 0
		if (!isMoveValid(b, m->move, att, side, tree)) {
			printBoardNice(b);
			sprintfMoveSimple(m->move, b3);
			L0("Invalid MOVE %s\n",b3);
			continue;
		}
#endif

		tree->tree[ply][ply].move = m->move;
		MakeMove(b, m->move, &u);

		eval_king_checks(b, &(att->ke[opside]), NULL, opside);
		if (isInCheck_Eval(b, att, opside)) {
			tree->tree[ply][ply].move |= CHECKFLAG;
			aftermovecheck = 1;
		}

		DEB_SE(
				sprintfMoveSimple(m->move, b2);
				LOGGER_0("%*d, +C , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, actph %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, mvs.actph);
		)

		if (((checks > 0)) && (aftermovecheck != 0))
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa,
				depth - 1, ply + 1, opside, tree, checks - 1,
				att);
		else
			if(checks > 0)
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, checks - 1, att);
			else 
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, 0, att);

		UnMakeMove(b, &u);
		LOGGER_SE("%*d, -C , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, m->real_score);
		if (m->real_score >= tbeta) {
			if (m == mvs.move)
				b->stats->qfirstcutoffs++;
			b->stats->qcutoffs++;
			b->stats->failhigh++;
			mb = m;
			goto ESTOP;
		}
		if (m->real_score > mb->real_score) {
			mb = m;
			if (mb->real_score > talfa) {
				talfa = mb->real_score;
				copyTree(tree, ply);
			}
		}
	}

// restore best
	tree->tree[ply][ply].score = mb->real_score;
	tree->tree[ply][ply].move = mb->move;

	if (mb->real_score <= talfa) {
		b->stats->faillow++;
	} else
		b->stats->failnorm++;

	ESTOP:
	b->stats->qmovestested += mvs.count;
	b->stats->qpossiblemoves += ((mvs.lastp - mvs.move));

	return mb->real_score;
}

int QuiesceNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store *tree, int checks, const attack_model *tolev)
{
	move_cont mvs;
	move_entry *m, mdum = { MATE_M, 0, 0 - GenerateMATESCORE(ply) }, *mb;
	attack_model *att, ATT;

	int opside, scr, fullrun;
	int incheck, talfa, tbeta, gmr, aftermcheck;
	UNDO u;
	DEB_SE( char b2[256]; )
//	char b3[256];

	scr = gmr = -mdum.real_score;
	
	// mate distance pruning
	if (((gmr) <= alfa)||(-gmr >= beta)||(ply >= MAXPLY - 1)) {
		if (gmr <= alfa) return alfa;
		return beta;
	}
	b->stats->nodes++;
	b->stats->qposvisited++;
	
	LOGGER_SE("%*d, *Q , EEEE, amove ch:X, depth %d, alfa %d, beta %d\n", 2+ply, ply, depth, alfa, beta);
	
	if (!(b->stats->nodes & b->run.nodes_mask)) {
		if(update_status(b)!=0){
			return 0;
		}
	}

	if (b->stats->depth_max < ply) {
		b->stats->depth_max = ply;
	}

	opside = Flip(side);
	mb = &mdum;
	att = &ATT;

// copy PINs, attacks at my king 
	att->ke[side] = tolev->ke[side];

// opside attacked squares
	att->att_by_side[opside] = KingAvoidSQ(b, att, opside);
	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move) != 0);

	if ((checks > 0) && (is_draw(b, att, b->pers) > 0) && (!incheck)) {
//	if ((is_draw(b, att, b->pers) > 0) && (!incheck)) {
			tree->tree[ply][ply].move = DRAW_M;
			return 0;
	}

	scr = lazyEval(b, att, alfa, beta, side, ply, depth, b->pers, &fullrun);
	if ((scr >= beta)) return scr;
	if(!incheck)
	{
		talfa = scr > alfa ? scr : alfa;
		mb->real_score = scr;
	}
	else talfa=alfa;

#if 1
	if ((b->pers->use_quiesce == 0) || (ply >= MAXPLY)
	|| (ply > ((b->depth_run * (b->pers->quiesce_depth_limit_multi + 10)) / 10))) {
#else
	if ((b->pers->use_quiesce == 0) || (ply >= MAXPLY)){
#endif
		tree->tree[ply][ply].move = NA_MOVE;
		return scr;
	}
	tbeta = beta;

// check for king capture & for incheck solution
// find if any move hits other king
//	if (fullrun == 0)
		att->att_by_side[side] = KingAvoidSQ(b, att, side);
	if (att->att_by_side[side] & normmark[b->king[opside]])
// i have captured king!
		return -gmr;

	LOGGER_SE("%*d, *Q , QQQQ, amove ch:X, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);
	
	sortMoveListNew_Init(b, att, &mvs);
	LOGGER_SE("%*d, *Q , SORT, amove ch:X, dpth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);
	while ((getNextCap(b, att, &mvs, ply, side, incheck, &m, tree) != 0)
		&& (b->search_abort == 0)) {

		tree->tree[ply][ply].move = m->move;

		MakeMove(b, m->move, &u);
		eval_king_checks(b, &(att->ke[opside]), NULL, opside);
		if (isInCheck_Eval(b, att, opside)) {
			tree->tree[ply][ply].move |= CHECKFLAG;
			aftermcheck = 1;
		} else
			aftermcheck = 0;

		DEB_SE(
				sprintfMoveSimple(m->move, b2);
				LOGGER_0("%*d, +Q , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score);
		)

		/*
		 * How to work with checks?
		 * incheck
		 * aftermcheck
		 * checks
		 */
		int incheck2;
/*
 * incheck I'm incheck before makemove
 * aftermcheck - has makemove delivered check?
 */
 #if 1
		if (incheck) {
			eval_king_checks(b, &(att->ke[side]), NULL, side);
			incheck2 = att->ke[side].attackers != 0;
			if ((incheck2 != 0)) {
				UnMakeMove(b, &u);
				LOGGER_SE("%*d, -Q2 , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score, m->real_score);
				continue;
			}
		}
#endif
		if (((checks > 0) || ((checks <= 0) && (mb == &mdum)))
//		if (((checks > 0) )
			&& (aftermcheck))
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa,
				depth - 1, ply + 1, opside, tree, checks - 1,
				att);
		else
			if(checks > 0)
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, checks - 1, att);
			else
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, 0, att);
		UnMakeMove(b, &u);

		LOGGER_SE("%*d, -Q , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score, m->real_score);
		if (m->real_score >= tbeta) {
			if (m == mvs.move)
				b->stats->qfirstcutoffs++;
			b->stats->qcutoffs++;
			b->stats->failhigh++;
			mb = m;
			goto ESTOP;
		}
		if (m->real_score > mb->real_score) {
			mb = m;
			if (mb->real_score > talfa) {
				talfa = mb->real_score;
				copyTree(tree, ply);
			}
		}
	}
	LOGGER_SE("%*d, *Q , ALOP, amove ch:X, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);
// what to do when in check and no capture improved alpha?

#if 1
// generate checks
	if((checks>0) && (mb->real_score<talfa)&&(b->search_abort==0)&&(incheck==0)) {
	tree->tree[ply][ply+1].move=NA_MOVE;

		b->stats->qmovestested+=mvs.count;
		sortMoveListNew_Init(b, att, &mvs);
		while ((getNextCheckin(b, att, &mvs, ply, side, incheck, &m, tree)!=0)&&(b->search_abort==0)) {
			tree->tree[ply][ply].move=m->move;
//			L0("---\n");
//			sprintfMoveSimple(m->move, b3);
//			L0("Qcheck MOVE %s\n", b3);
			MakeMove(b, m->move, &u);

DEB_SE(
			sprintfMoveSimple(m->move, b2);
		LOGGER_0("%*d, +G , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, 1, depth, talfa, tbeta, mb->real_score);
)

			eval_king_checks(b, &(att->ke[opside]), NULL, opside);
			tree->tree[ply][ply].move|=CHECKFLAG;
			tree->tree[ply][ply+1].move=NA_MOVE;
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa, depth-1, ply+1, opside, tree, checks-1, att);
//			L0("+++\n");
			UnMakeMove(b, &u);
			LOGGER_SE("%*d, -G , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, 1, depth, talfa, tbeta, mb->real_score, m->real_score);
			if(m->real_score>=tbeta) {
				b->stats->qcutoffs++;
				b->stats->failhigh++;
				mb=m;
				goto ESTOP;
			}
			if(m->real_score>mb->real_score) {
				mb=m;
				if(mb->real_score>talfa) {
					talfa=mb->real_score;
					copyTree(tree, ply);
				}
			}
		}
	}
#endif

	if (b->search_abort != 0) {
		mb->real_score = 0;
		goto ESTOP;
	}

// restore best
	tree->tree[ply][ply].score = mb->real_score;
	tree->tree[ply][ply].move = mb->move;

	if (mb->real_score <= alfa) {
		b->stats->faillow++;
		tree->tree[ply][ply + 1].move = ALL_NODE;
	} else b->stats->failnorm++;

ESTOP:
	b->stats->qmovestested += mvs.count;
	b->stats->qpossiblemoves += ((mvs.lastp - mvs.move));
	return mb->real_score;
}

// ttbeta 
int SearchMoveNew(board *b, int talfa, int tbeta, int ttbeta, int depth, int ply, int extend, int reduce, int side, tree_store *tree, int nulls, const attack_model *att)
{
	int val, ext;
	int isPV;
	int opside = Flip(side);

	isPV = (talfa != (ttbeta - 1));
	b->stats->zerototal += (1 - isPV);
	ext = depth - reduce + extend - 1;
	val = 0;
	if (((ext > 0) && (ply < MAXPLY))) {
		val = -ABNew(b, -ttbeta, -talfa, ext, ply + 1, opside, tree,
			nulls, att);
#if 1
		if ((val > talfa) && ((reduce) > 0))
			val = -ABNew(b, -ttbeta, -talfa, depth + extend - 1,
				ply + 1, opside, tree, nulls, att);
#endif
	} else
		val = -QuiesceNew(b, -ttbeta, -talfa, ext, ply + 1, opside,
			tree, b->pers->quiesce_check_depth_limit, att);

	if ((val > talfa && val < tbeta && ttbeta < tbeta)
		&& (b->search_abort == 0)) {
		ext = depth + extend - 1;
		b->stats->zerorerun++;
		if (ext > 0)
			val = -ABNew(b, -tbeta, -talfa, ext, ply + 1, opside,
				tree, nulls, att);
		else
			val = -QuiesceNew(b, -tbeta, -talfa, ext, ply + 1,
				opside, tree,
				b->pers->quiesce_check_depth_limit, att);
		if (val <= talfa)
			b->stats->fhflcount++;
		if (reduce > 0)
			b->stats->lmrrerun++;
	}
	return val;
}

/*
 * FailSoft - patricne se upravuje AlfaBeta okno, ale vraci se vypocitana hodnota i kdyz je mimo okno
 * FailHard - upravuje se AlfaBeta okno a vraci se vypocitana hodnota nebo hranice, pokud je hodnota mimo okno
 
 * terminologie
 * PV-node - score je uvnitr hranic, vracena hodnota je exaktni, score S je mezi A a B
 * Cut-nodes / fail-high node - proveden beta-cutoff, plati S>=B, cili S neni exaktnim ohodnocenim,
 * ale je spodni hranici hledaneho ohodnoceni
 * All-nodes / fail-low node - nic nezlepsilo A, score S<=A, S je horni hranici hledaneho
 *
 * v hash typ
 * 0 - N/A
 * 1 - AllNodes - znamena (vsechny moznosti prolezeny) a tohle je maximum, a v hledani to neprekrocilo alfa
 * 2 - Exact - presne cislo a v danem hledani se to trefilo mezi alfa - beta
 * 3 - BCutoff - v danem hledani tohle prekrocilo beta, realna hodnota muze byt jeste vyssi
 *
 * FAILLOW_SC znamena, ze v dane pozici nic neprekrocilo ALFA, byly vyhodnoceny vsechny moznosti a dana hodnota
 * je Horni hranici Score dane pozice, ktere muze byt nizsi, tedy UPPER BOUND, node je AllNodes/fail-low
 * plati pouze v prvni iteraci 
 *
 * FAILHIGH_SC znamena, ze v dane pozici doslo k Beta-CutOff (prekroceni BETA),
 * uvedena hodnota je Spodni hranici Score pro danou pozici, ktere muze byt vyssi, tedy LOWER BOUND, node je Cut-node/failhigh
 *
 * upravovat Alfa - hodnota ktere urcite mohu dosahnout
 * upravovat Beta - hodnota kterou kdyz prekrocim tak si o uroven vyse tah vedouci do teto pozice nevyberou
 * udrzovat aktualni hodnotu nezavisle na A a B
 
 * - best - zatim nejvyssi hodnota
 * - bestmove - odpovidajici tah
 * - val - hodnota prave spocitaneho tahu
 */

int ABNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store *tree, int nulls, const attack_model *tolev)
// depth - jak hluboko mam jit, 0 znamena pouze evaluaci pozice, zadne dalsi pultahy
// ply - jak jsem hluboko, 0 jsem v root pozici
{
	int qual;
	move_entry *m, mdum = { MATE_M, 0, 0 - GenerateMATESCORE(ply) }, *mb, *mn,
			mt;
	move_cont mvs;
	int opside;
	int isPV = (alfa != (beta - 1));
	int pval, sval;
	int incheck, talfa, tbeta, ttbeta, gmr, aftermovecheck;
	int reduce, extend, ext;
	int reduce_o, extend_o;
	unsigned long long nodes_stat, null_stat;
	hashEntry hash;
	BITVAR pld;
	DEB_SE( char b2[256];)

	UNDO u;
	attack_model *att, ATT;

	b->stats->nodes++;
	b->stats->positionsvisited++;
	tree->tree[ply][ply].move = NA_MOVE;
	
	mb = &mdum;
	if (!(b->stats->nodes & b->run.nodes_mask)) {
		if(update_status(b)!=0) {
			return 0;
		}
	} LOGGER_SE("%*d, *S , EEEE, amove ch:X, depth %d, talfa %d, tbeta %d,incheck ?\n", 2+ply, ply, depth, alfa, beta);

// mate distance pruning
	gmr = -mb->real_score;

// mate distance pruning
	if (gmr <= alfa) {
		b->stats->faillow++;
		return alfa;
	}
	if (-gmr >= beta) {
		b->stats->failhigh++;
		return beta;
	}

/*
	if (b->pers->negamax == 0) {
		// nechceme AB search, ale klasicky minimax
		alfa = 0 - iINFINITY;
		beta = iINFINITY;
	}
*/

	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move) != 0);
	opside = Flip(side);
	att = &ATT;

// copy analysis of attacks at my king including PINs, checks that was done ply above
	att->ke[b->side] = tolev->ke[b->side];
	
// create map of squares attacked by opside
	att->att_by_side[opside] = KingAvoidSQ(b, att, opside);

	if ((is_draw(b, att, b->pers) > 0) && (!incheck)) {
		mb->move = tree->tree[ply][ply].move = DRAW_M;
		mb->real_score = 0;
		if (mb->real_score <= alfa)
			b->stats->faillow++;
		else if (mb->real_score >= beta)
			b->stats->failhigh++;
		else
			b->stats->failnorm++;
		goto ABFINISH2;
	}

// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move = NA_MOVE;
	tree->tree[ply + 1][ply + 1].move = NA_MOVE;
	tree->tree[ply][ply + 1].move = NA_MOVE;

	talfa = alfa;
	tbeta = beta;
	if (tbeta > gmr)
		tbeta = gmr;
	if (talfa < -gmr)
		talfa = -gmr;

	mt.move = DRAW_M;
	
	int hresult;
// time to check hash table
// TT CUT off?
	if (b->hs != NULL) {
		hash.key = b->key;
		hash.scoretype = NO_NULL;
		hresult = 0;
		hresult = retrieveHash(b->hs, &hash, side, ply, depth,
			b->pers->use_ttable_prev, b->norm, b->stats);
		if (hresult != 0) {
			mt.real_score = hash.value;
			mt.move = hash.bestmove;
			if ((mt.move == NULL_MOVE)
				|| (isMoveValid(b, mt.move, att, side, tree))) {
				if ((hash.depth >= depth)) {
					tree->tree[ply][ply].move =
						hash.bestmove;
					tree->tree[ply][ply].score = hash.value;
					if ((hash.scoretype != FAILHIGH_SC)
						&& (hash.value <= talfa)) {
						b->stats->faillow++;
						b->stats->failhashlow++;
						mb = &mt;
						goto ABFINISH2;
					} else if ((hash.scoretype != FAILLOW_SC)
						&& (hash.value >= tbeta)) {
						b->stats->failhigh++;
						b->stats->failhashhigh++;
						mb = &mt;
						goto ABFINISH2;
					} else if (hash.scoretype == EXACT_SC) {
						b->stats->failhashnorm++;
						if (b->pers->use_hash) {
							restoreExactPV(b->hs,
								b->key, b->norm,
								ply, tree);
							copyTree(tree, ply);
							b->stats->failnorm++;
							mb = &mt;
							goto ABFINISH2;
						} else {
							mt.real_score =
								mdum.real_score;
						}
					}
				} else {
// not enough depth
					if ((b->pers->NMP_allowed > 0)
						&& (hash.scoretype
							!= FAILHIGH_SC)
//						&& (hash.depth
//							>= (depth
//								- b->pers->NMP_reduction
//								- 1))
						&& (hash.value < beta))
						nulls = 0;
					mt.move = DRAW_M;
				}
			} else
				mt.move = DRAW_M;
		}
	}

	reduce_o = extend_o = 0;

	aftermovecheck = 0;
// null move PRUNING / REDUCING
	if ((nulls > 0) && (isPV == 0) && (b->pers->NMP_allowed > 0)
		&& (incheck == 0)
		&& (can_do_NullMove(b, att, talfa, tbeta, depth, ply, side) != 0)
		&& (depth >= b->pers->NMP_min_depth)) {
		tree->tree[ply][ply].move = NULL_MOVE;
		MakeNullMove(b, &u);

		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
		LOGGER_SE("%*d, +S , NULL, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, aftermovecheck, depth, talfa, tbeta, mb->real_score);
		
		b->stats->NMP_tries++;
// null move reduction
		reduce = b->pers->NMP_reduction + div(depth, b->pers->NMP_div).quot;
		ext = depth - reduce - 1;
// save stats, to get info how many nodes were visited due to NULL move...
		nodes_stat = b->stats->nodes;
		null_stat = b->stats->u_nullnodes;
		if (ext > 0) {
			LOGGER_SE("%*d, *S , NULL, AB, alfa %d, beta %d, ext %d, ply %d, nulls %d\n", 2+ply, ply, -tbeta, -tbeta+1, ext, ply+1, nulls-1);
			mt.real_score = -ABNew(b, -tbeta, -tbeta + 1, ext,
				ply + 1, opside, tree, nulls - 1, att);
		} else {
			LOGGER_SE("%*d, *S , NULL, Q, alfa %d, beta %d, ext %d, ply %d, checks %d\n", 2+ply, ply, -tbeta, -tbeta+1, ext, ply+1, b->pers->quiesce_check_depth_limit);
			mt.real_score = -QuiesceNew(b, -tbeta, -tbeta + 1, ext,
				ply + 1, opside, tree,
				b->pers->quiesce_check_depth_limit, att);
		}

// update null nodes statistics
		UnMakeNullMove(b, &u);
		LOGGER_SE("%*d, -S , NULL, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, aftermovecheck, depth, talfa, tbeta, mb->real_score, mt.real_score);

// engine stop protection?
		if (b->search_abort != 0)
			goto ABFINISH2;
		b->stats->u_nullnodes = null_stat
			+ (b->stats->nodes - nodes_stat);
		if (mt.real_score >= tbeta) {
			b->stats->NMP_cuts++;
			hash.key = b->key;
			hash.depth = (int16_t) depth;
			hash.value = mt.real_score;
			hash.bestmove = NULL_MOVE;
			hash.scoretype = FAILHIGH_SC;
			if ((b->hs != NULL) && (b->search_abort == 0))
				storeHash(b->hs, &hash, side, ply, ext, b->norm, 
					b->stats);
			if (b->pers->NMP_search_reduction == 0) {
				b->stats->failhigh++;
				mb = &mt;
				goto ABFINISH2;
			} else if (b->pers->NMP_search_reduction == -1) {
				reduce_o = 0;
				mt.move = DRAW_M;
			} else
				reduce_o = b->pers->NMP_search_reduction;
		}
	} else if ((nulls <= 0) && (b->pers->NMP_allowed > 0))
		nulls = b->pers->NMP_allowed;

#if 1
	if (mt.move == DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID
		if ((depth >= b->pers->IID_remain_depth) && (isPV)
			&& (b->hs != NULL)) {
			mt.real_score = ABNew(b, talfa, tbeta, depth - 2, ply,
				side, tree, nulls, att);
			if (b->search_abort != 0)
				goto ABFINISH2;
			if (mt.real_score < talfa) {
				mt.real_score = ABNew(b, -iINFINITY, tbeta,
					depth - 2, ply, side, tree, nulls, att);
				if (b->search_abort != 0)
					goto ABFINISH2;
			}
			if (retrieveHash(b->hs, &hash, side, ply, depth,
				b->pers->use_ttable_prev, b->norm, b->stats) != 0)
				mt.move = hash.bestmove;
			else
				mt.move = DRAW_M;
		}
	}

// try to judge on position and reduce / quit move searching
// sort of forward pruning / forward reducing

	if ((incheck != 1) && (b->pers->quality_search_reduction != 0)
		&& (ply > 4)) {
		qual = position_quality(b, att, talfa, tbeta, depth, ply, side);
		b->stats->position_quality_tests++;
		if (qual == 0) {
			b->stats->position_quality_cutoffs++;
			if (b->pers->quality_search_reduction == -1) {
				tree->tree[ply][ply].move = FAILLOW_SC;
				tree->tree[ply][ply].score = -iINFINITY;
				goto ABFINISH2;
			} else
				reduce_o += b->pers->quality_search_reduction;
		}
	}
#endif

// generate moves
	sortMoveListNew_Init(b, att, &mvs);
	if ((mt.move == DRAW_M) || (mt.move == NULL_MOVE))
		mvs.hash.move = DRAW_M;
	else
		mvs.hash.move = mt.move;
	b->stats->poswithmove++;

// main loop
	LOGGER_SE("%*d, *S , XXXX, amove ch:X, depth %d, talfa %d, tbeta %d,incheck %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, incheck, mb->real_score);

	while ((getNextMove(b, att, &mvs, ply, side, incheck, &m, tree) != 0)
		&& (b->search_abort == 0)) {
		extend = extend_o;
		reduce = reduce_o;
		tree->tree[ply][ply].move = m->move;
		MakeMove(b, m->move, &u);
// makemove switches board sides, b->side changes during makemove, now b->side==opside

// analyse attacks on king of side to move, incl PINs
// is side to move in check, remember it and extend depth by one
		eval_king_checks(b, &(att->ke[opside]), NULL, opside);
		if (isInCheck_Eval(b, att, opside)) {
// idea from Crafty - extend only SAFE moves
		if (b->pers->check_extension > 0) {
				pval =
					(u.captured < ER_PIECE) ? b->pers->Values[1][u.captured] :
						0;
				sval = SEE0(b, UnPackTo(m->move), side, pval);
				if (sval >= 0)
					extend += b->pers->check_extension;
			}
			tree->tree[ply][ply].move |= CHECKFLAG;
			aftermovecheck = 1;
		} else
			aftermovecheck = 0;

// setup window
		ttbeta =
			((mvs.count <= b->pers->PVS_full_moves) && isPV) ? tbeta :
				talfa + 1;

		DEB_SE(
				sprintfMoveSimple(m->move, b2);
				LOGGER_0("%*d, +S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, phase %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, mvs.actph);
		)
// check for Futility pruning conditions, based on depth
// !extended !incheck !isPV !first_move use_fprune
// getlazyEval + fprune_margin < alfa drop;

// check for LMP conditions based on depth
// !extended !incheck !isPV !first_move use_lmp move mvs.actph >= NORMAL !MATEd
// 


// setup LMR reductions
// reduce based on ply and movecount
		if (mvs.count > b->pers->LMR_start_move
			&& (b->pers->LMR_reduction > 0)
			&& (depth >= b->pers->LMR_remain_depth)
			&& (incheck == 0) && (aftermovecheck == 0)
			&& (extend != extend_o)
			&& mvs.actph>=NORMAL
			&& can_do_LMR(b, att, talfa, ttbeta, depth, ply, side, m)) {
			if (mvs.count > b->pers->LMR_prog_start_move)
				reduce += div(depth, b->pers->LMR_prog_mod).quot;
			reduce += b->pers->LMR_reduction + div(mvs.count,10).quot;
			b->stats->lmrtotal++;
		}

		m->real_score = SearchMoveNew(b, talfa, tbeta, ttbeta, depth,
			ply, extend, reduce, side, tree, nulls, att);
		UnMakeMove(b, &u);
		if (b->search_abort != 0)
			goto ABFINISH;

		LOGGER_SE("%*d, -S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, m->real_score);

		if (m->real_score >= tbeta) {
			if (mvs.count == 1)
				b->stats->firstcutoffs++;
			b->stats->cutoffs++;
			if ((b->pers->use_killer >= 1)
				&& (mvs.actph>=KILLER1 && mvs.actph<OTHER)) {
				update_killer_move(b->kmove, ply, m->move, b->stats);
				if(mvs.actph==NORMAL) {
					updateHHTableGood(b, b->hht, m, 0, side, depth, ply);
					if(mvs.quiet!=NULL) for(mn=m-1; mn>=mvs.quiet; mn--) updateHHTableBad(b, b->hht, mn, 0, side, depth, ply);
				}
			}
			mb = m;
			break;
		}
		if (m->real_score > mb->real_score) {
			mb = m;
			if (mb->real_score > talfa) {
				talfa = mb->real_score;
				copyTree(tree, ply);
			}
		}
	}
//	dump_moves(b, mvs.move, (mvs.lastp-mvs.move), ply, "abdump");

	if (mvs.count <= 0) {
		if (incheck == 0) {
// no moves found, not in check => draw, if incheck means mated - default setting for mb
			mb->real_score = 0;
			mb->move = DRAW_M;
		}
	}
	tree->tree[ply][ply].move = mb->move;
	tree->tree[ply][ply].score = mb->real_score;

	// update stats & store Hash

	hash.key = b->key;
	hash.depth = (int16_t) depth;
	hash.value = mb->real_score;
	hash.bestmove = mb->move;
	if (mb->real_score > alfa && mb->real_score < beta) {
		b->stats->failnorm++;
		hash.scoretype = EXACT_SC;
		if ((b->hs != NULL) && (b->pers->use_hash == 1) && (depth > 0)
			&& (b->search_abort == 0)) {
			storeHash(b->hs, &hash, side, ply, depth, b->norm, b->stats);
//!!!!		
			tree->tree[ply][ply].pld = hash.pld;
			tree->tree[ply][ply].key = b->key;
			tree->tree[ply][ply].ver = b->norm;
			storeExactPV(b->hs, b->key, b->norm, tree, ply);
			if (b->pers->use_killer >= 1) {
					if(mb->phase==NORMAL) updateHHTableGood(b, b->hht, mb, 0, side, depth, ply);
					if(mvs.quiet!=NULL)
						for(mn=mvs.lastp-1; mn>=mvs.quiet; mn--) 
							if(mn!=mb)
								updateHHTableBad(b, b->hht, mn, 0, side, depth, ply);
//								if(mn->phase==NORMAL)  updateHHTableBad(b, b->hht, mn, 0, side, depth, ply);
			}
		}
	} else {
		if (mb->real_score >= beta) {
			b->stats->failhigh++;
			hash.scoretype = FAILHIGH_SC;
		} else {
			b->stats->faillow++;
			hash.scoretype = FAILLOW_SC;
		}
		if ((b->hs != NULL) && (depth > 0))
			storeHash(b->hs, &hash, side, ply, depth, b->norm, b->stats);
	}
	ABFINISH:

	b->stats->movestested += mvs.count;
	b->stats->possiblemoves += ((mvs.lastp - mvs.move));
	ABFINISH2: LOGGER_4("count %d, score %d\n", mvs.count, mb->real_score);
	return mb->real_score;
}

#define MISn 575
#define MISc 120

int IterativeSearchN(board *b, int alfa, int beta, int depth, int side, int start_depth, tree_store *tree)
{
	int f;
	struct _statistics s, r, s2;

	int reduce, pval, sval;
	int asp_win = 0;
	int ply = 0;
	int changes;
	int alow, ahigh;
	int aspdiff[]={500, 1000, 10000, 100000, 1000000, iINFINITY};

	int cc, v, xcc, old_score, old_score_count;
	MOVESTORE bestmove, hashmove, i, t1pbestmove;
	move_entry tm;
	move_cont mvs;

	int opside;
	int legalmoves, incheck, best, talfa, tbeta, ttbeta, t1pbest,
			aftermovecheck, isPVcount;
	unsigned long long int nodes_bmove;
	int extend;
	hashEntry hash;
	char b2[256];

	UNDO u;
	attack_model *att, ATT;
	unsigned long long tstart, ebfnodesold, tnow;

	old_score = best = 0 - iINFINITY;
	old_score_count = 0;
	b->bestmove = NA_MOVE;
	b->bestscore = best;
	bestmove = hashmove = NA_MOVE;
	opside = (side == WHITE) ? BLACK : WHITE;
	copyBoard(b, &(tree->tree_board));

	b->run.iter_start = b->run.time_start;
	b->run.nodes_at_iter_start = b->stats->nodes;
	b->run.nodes_mask = (1ULL << b->pers->check_nodes_count) - 1;
	b->search_abort=0;

	DEB_1(if((b->uci_options->engine_verbose>=1)) printBoardNice(b);)

	// make current line end here
	tree->tree[ply][ply].move = NA_MOVE;
	tree->tree[ply + 1][ply + 1].move = NA_MOVE;
	tree->tree[ply][ply + 1].move = NA_MOVE;
	tree->tree[ply][ply].score = 0;
	b->p_pv.line[ply].move = NA_MOVE;  //???

	att = &ATT;
	// !!!!
//	setup_attack_index(b, att);
	// !!!!
	att->phase = eval_phase(b, b->pers);
	att->att_by_side[opside] = KingAvoidSQ(b, att, opside);
	eval_king_checks_all(b, att);

	// is opposite side in check ?
	if (isInCheck_Eval(b, att, opside) != 0) {
		DEB_1(printf("Opside in check4!\n");)
		tree->tree[ply][ply].move = MATE_M;
		return MATESCORE;
	}

	// is side to move in check ?
	incheck = (isInCheck_Eval(b, att, side) != 0);

	// check database of openings
	i = probe_book(b);
	if (i != NA_MOVE) {
		tree->tree[ply][ply].move = i;
		b->bestmove = tree->tree[ply][ply].move;
		b->bestscore = tree->tree[ply][ply].score;
		return 0;
	}

	sortMoveListNew_Init(b, att, &mvs);
	mvs.hash.move = DRAW_M;
	mvs.lastp = mvs.move;
	mvs.next = mvs.lastp;
	if (incheck == 1) {
		generateInCheckMovesN(b, att, &(mvs.lastp), 1);
	} else {
		generateCapturesN2(b, att, &(mvs.lastp), 1);
		generateMovesN2(b, att, &(mvs.lastp));
	}
	b->max_idx_root = mvs.lastp - mvs.move;
	if (b->max_idx_root == 1) {
		tree->tree[ply][ply].move = mvs.move[0].move;
		tree->tree[ply][ply + 1].move = NA_MOVE;
		tree->tree[ply][ply].score = 0;
		b->bestmove = tree->tree[ply][ply].move;
		b->bestscore = tree->tree[ply][ply].score;
		LOGGER_3("One move play hit\n");
		return 0;
	}

	// 0 - not age hash table
	// 1 - age with new game
	// 2 - age with new move / Iterative search entry
	// 3 - age with new interation 
	if (b->pers->ttable_clearing >= 2) {
		invalidateHash(b->hs);
		invalidatePawnHash(b->hps);
	}
	// iterate and increase depth gradually
	oldPVcheck = 0;
	clearSearchCnt(b->stats);

	// initial sort according
	cc = 0;
	talfa=alfa;
	tbeta=beta;
#if 1
	b->depth_run = 1;
	if (!incheck)
		while (cc < b->max_idx_root) {
			MakeMove(b, mvs.move[cc].move, &u);
			eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
			if (isInCheck_Eval(b, att, b->side)) {
				extend += b->pers->check_extension;
				mvs.move[cc].move |= CHECKFLAG;
			}
			tree->tree[ply][ply].move = mvs.move[cc].move;
			v = -QuiesceNew(b, -tbeta, -talfa, 0, 1, opside, tree,
				0, att);
			mvs.move[cc].qorder = v;
			UnMakeMove(b, &u);
			cc++;
		}
#endif

	/*
	 * b->stats, complete stats for all iterations
	 * s stats at beginning of iteration
	 */
	clearSearchCnt(&s);
	clearSearchCnt(&s2);
	clearSearchCnt(b->stats);

	b->stats->poswithmove++;
	ebfnodesold = 1;

	tstart = readClock();
	if (depth > MAXPLY)
		depth = MAXPLY;

	clearHHTable(b->hht);
	if (depth >= MAXPLY) depth = MAXPLY - 1;
	b->search_dif = (incheck) ? MISc : MISn;

// DEEPENING 
	for (f = start_depth; f <= depth; f++) {
		update_status(b);
		b->depth_run = f;
		changes = 0;

		if (b->pers->ttable_clearing >= 3) {
			invalidateHash(b->hs);
			invalidatePawnHash(b->hps);
		}
/*
		if (b->pers->negamax == 0) {
			talfa = alfa;
			tbeta = beta;
		}
*/
		CopySearchCnt(&s, b->stats);
		if (b->hs != NULL) installHashPV(&b->p_pv, b, f - 1, b->stats);
		clear_killer_moves(b->kmove);
		// (re)sort moves
		SelectBestO(&mvs);
		if (f > start_depth) {
			for (cc = 0; cc < b->max_idx_root; cc++)
				if (mvs.move[cc].move == b->p_pv.line[0].move) break;
			tm = mvs.move[cc];
			if (cc > 0) {
				for (; cc > 0; cc--) mvs.move[cc] = mvs.move[cc - 1];
				mvs.move[0] = tm;
			}
		}

		b->stats->positionsvisited++;
		b->stats->possiblemoves += (unsigned int) b->max_idx_root;
		b->stats->nodes++;
		
		xcc = -1;
		alow=ahigh=0;
		eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);

// aspiration entry point within depth
rerun:
		best = 0 - iINFINITY;
		isPVcount = 0;
		
		if((b->pers->use_aspiration!=0)&&(f>4)&&(!incheck)) {
			talfa=Max(alfa, old_score-aspdiff[alow]);
			tbeta=Min(beta, old_score+aspdiff[ahigh]);
		} else {
			talfa = alfa;
			tbeta = beta;
		}
//		L0("F %d, old %d, ta %d, tb %d, a %d, b %d, al %d, ah %d\n",f, old_score, talfa, tbeta, alfa, beta, alow, ahigh);

		
		cc = 0;
		// loop over all moves
		// inicializujeme line
		tree->tree[ply][ply].move = NA_MOVE;
		legalmoves = 0;
		
// looping moves for depth f
		while ((cc < b->max_idx_root) && (b->search_abort == 0)) {
			b->idx_root = cc;
			extend = 0;
			if (!(b->stats->nodes & b->run.nodes_mask)) {
				update_status(b);
			}
			nodes_bmove = b->stats->movestested + b->stats->qmovestested;
			b->stats->movestested++;
			tree->tree[ply][ply].move = mvs.move[cc].move;
			mvs.move[cc].real_score = 0;
			
			MakeMove(b, mvs.move[cc].move, &u);
			eval_king_checks(b, &(att->ke[b->side]), NULL, b->side);
			aftermovecheck = 0;
			if (isInCheck_Eval(b, att, b->side)) {
				tree->tree[ply][ply].move |= CHECKFLAG;
				if (b->pers->check_extension > 0) {
					pval = (u.captured < ER_PIECE) ? b->pers->Values[1][u.captured] : 0;
					sval = SEE0(b, UnPackTo(mvs.move[cc].move), side, pval);
					if (sval >= 0) extend += b->pers->check_extension;
				}
				aftermovecheck = 1;
			}
			DEB_SE(
					sprintfMoveSimple(mvs.move[cc].move, b2);
					LOGGER_0("%*d, +I , %s, amove ch:%d, depth %d, talfa %d, tbeta %d\n", 2+ply, ply, b2, aftermovecheck, f, talfa, tbeta);
			)
			reduce = 0;
			if (legalmoves >= b->pers->LMR_start_move
				&& (b->pers->LMR_reduction > 0)
				&& (depth >= b->pers->LMR_remain_depth)
				&& (incheck == 0) && (aftermovecheck == 0)
				&& mvs.actph>=NORMAL
				&& can_do_LMR(b, att, talfa, tbeta, depth, ply,
					side, &(mvs.move[cc]))) {
				reduce += b->pers->LMR_reduction;
				b->stats->lmrtotal++;
			}
			
			ttbeta = ((isPVcount < b->pers->PVS_root_full_moves)) ? tbeta : talfa + 1;
			v = SearchMoveNew(b, talfa, tbeta, ttbeta, f, 0, extend, reduce, side, tree, b->pers->NMP_allowed, att);
			mvs.move[cc].real_score = v;
			LOGGER_SE("%*d, -I , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, v);
			if (b->search_abort == 0) {
				unsigned long long tqorder =
					b->stats->movestested
						+ b->stats->qmovestested
						- nodes_bmove;
				mvs.move[cc].qorder =
					(tqorder >= (LONG_MAX / 2)) ? (LONG_MAX / 2) : (long int) tqorder;
				legalmoves++;
				if (v > best) {
					best = v;
					bestmove = mvs.move[cc].move;
					xcc = cc;
					isPVcount++;
					if (v > talfa) {
						talfa = v;
						if (v >= tbeta) {
							if (b->pers->use_aspiration == 0) {
								LOGGER_1("ERR: nemelo by jit pres TBETA v rootu\n");
								abort();
							}
							tree->tree[ply][ply + 1].move = BETA_CUT;
							xcc = -1;
							UnMakeMove(b, &u);
							break;
						} else {
							changes++;
							if(((changes >= 2)) 
							&& (f > (start_depth + 1))) {
								b->search_dif = Min(1000, Max((incheck) ? MISn : MISn, b->search_dif * (1+1/(7*changes))));
							}
							copyTree(tree, ply);
							tree->tree[ply][ply].score = best;
							// best line change
							if (b->uci_options->engine_verbose >= 1) printPV_simple(b, tree,f,b->side,&s,b->stats);
							DEB_SE(
								sprintfMoveSimple(mvs.move[cc].move, b2);
								LOGGER_0("%*d, *Iu, %s, amove ch:%d, depth %d, talfa %d, tbeta %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, best);
							)
						}
					} else if ((cc == 0)&&(b->pers->use_aspiration !=0)) {
						xcc = -1;
						UnMakeMove(b, &u);
						break;
					}
				}
				cc++;
			}
			UnMakeMove(b, &u);
		}
	tree->tree[ply][ply].move = bestmove;
	tree->tree[ply][ply].score = best;

	DEB_4(sprintfMoveSimple(bestmove, b2);)
	LOGGER_4("BESTMOVE %s\n", b2);

// search has finished
	if (b->search_abort == 0) {
// was not stopped during last iteration 

		b->stats->iterations++;
// clear qorder for moves not processed
		int li;
		for (li = cc; li < b->max_idx_root; li++)
			mvs.move[li].qorder = 0;

// handle aspiration if used
// check for problems
// over beta, not rising alfa at fist move or at all
//		L0("FFF %d\n", f);
		if ((b->pers->use_aspiration != 0) && (f > 4)) {
			if ((xcc == -1)) {
// handle anomalies
					b->stats->aspfailits++;
					if (tbeta <= best)
// we failed high 
						ahigh++;
					else {
// we failed low
//						if(cc==0) continue;
						alow++;
//						alow=7;
					}
					goto rerun;
			}
		} else {
		}
		// store proper bestmove & score
		// update stats & store Hash

		if (xcc != -1) {
			if(((changes != 1) || (tree->tree[ply][ply].move != b->p_pv.line[0].move)
			|| ((best+500) < old_score)) && (f > (start_depth + 1))) {
					b->search_dif =
						Min(1000,
							Max((incheck) ? MISn : MISn, b->search_dif*1.0)
							+((best+500) < old_score)*(old_score-best)/40
							+(tree->tree[ply][ply].move!= b->p_pv.line[0].move)*30
							);
			} else b->search_dif = Max(100, (best<0) ? b->search_dif -25 : b->search_dif * 0.85) ;

			hash.key = b->key;
			hash.depth = (int16_t) f;
			hash.value = best;
			hash.bestmove = bestmove;
			b->stats->failnorm++;
			hash.scoretype = EXACT_SC;
			if ((b->hs != NULL) && (f > 0) && (b->search_abort == 0)) {
				storeHash(b->hs, &hash, side, ply, f, b->norm, b->stats);
				tree->tree[ply][ply].pld = hash.pld;
				tree->tree[ply][ply].key = b->key;
				tree->tree[ply][ply].ver = b->norm;
				storeExactPV(b->hs, b->key, b->norm, tree, f);
			}
			store_PV_tree(tree, &b->p_pv);

			if (old_score == best) {
				old_score_count++;
				if ((old_score_count >= 3)
					&& (GetMATEDist(b->bestscore) <= (f - 1)))
					break;
			} else {
				old_score = best;
				old_score_count = 0;
			}
		}  // finished iteration
	} else {
// last iteration was not finished properly
		LOGGER_4("Interrupted in interation, move %d, processing %d, out of %d\n", xcc, b->idx_root, b->max_idx_root);
		if (xcc > -1) {
// move was found
			t1pbest = best;
			t1pbestmove = bestmove;
		} else if (f > start_depth) {
			t1pbestmove = b->p_pv.line[0].move;
			t1pbest = b->p_pv.line[0].score;
			restore_PV_tree(&b->p_pv, tree);
		} else {
			t1pbestmove = mvs.move[0].move;
			t1pbest = -MATEMAX;
		}
		tree->tree[ply][ply].move = t1pbestmove;
		tree->tree[ply][ply].score = t1pbest;
	}

	b->bestmove = tree->tree[ply][ply].move;
	b->bestscore = tree->tree[ply][ply].score;

	oldPVcheck = 1;

	tnow = readClock();
	b->stats->elaps += (tnow - tstart);

	if (b->search_abort == 0) {
		tstart = tnow;
		b->stats->ebfnodespri = ebfnodesold;
		ebfnodesold = (b->stats->nodes - s.nodes);
		b->stats->ebfnodes = ebfnodesold;
// calculate only finished iterations
		b->stats->depth = f;
	}
	DecSearchCnt(b->stats, &s, &r);

#pragma omp critical
{

// update stats how f-ply search has performed
	AddSearchCnt(&(STATS[f]), &r);
	AddSearchCnt(&(STATS[MAXPLY]), &r);
}
	// break only if mate is now - not in qsearch
	if ((b->search_abort != 0) || (search_finished(b) != 0))
		break;
	if ((b->uci_options->engine_verbose >= 1) && (xcc != -1))
		printPV_simple(b, tree, f, b->side, &s, b->stats);
	}  //deepening finished here
	b->stats->depth_sum += f;
	b->stats->depth_max_sum += b->stats->depth_max;

#pragma omp critical
{
// global stats
#if 1
	if (STATS[f].depth < b->stats->depth)
		STATS[f].depth = b->stats->depth;
	if (STATS[f].depth_max < b->stats->depth_max)
		STATS[f].depth_max = b->stats->depth_max;
	STATS[f].depth_sum += b->stats->depth;
	STATS[f].depth_max_sum += b->stats->depth_max;
	if (STATS[MAXPLY].depth < b->stats->depth)
		STATS[MAXPLY].depth = b->stats->depth;
	if (STATS[MAXPLY].depth_max < b->stats->depth_max)
		STATS[MAXPLY].depth_max = b->stats->depth_max;
	STATS[MAXPLY].depth_sum += b->stats->depth;
	STATS[MAXPLY].depth_max_sum += b->stats->depth_max;
#endif 
}

	DEB_1 (if((b->uci_options->engine_verbose>=1)) printSearchStat(b->stats);)
	return b->bestscore;
}
