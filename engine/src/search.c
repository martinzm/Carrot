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
	int f, mi, ply, l, pos[4];
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
			MakeMoveNew(&(tree->tree_board),
				tree->tree[0][f].move, pos, u+f);
			l = 1;
			break;
		}
		f++;
	}
	if (l == 0)
		f--;
	f--;
	while (f >= 0) {
		UnMakeMoveNew(&(tree->tree_board), u+f, pos);
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
			"info depth %d seldepth %lld nodes %lld score cp %d time %lld pv ",
			depth, s2->s[S_depth_max],
			s2->s[S_positionsvisited] + s2->s[S_qposvisited],
			tree->tree[0][0].score / 10, tno);
		strcat(b2, buff);
	} else {
		if (isMATE(tree->tree[0][0].score) < 0)
			mi = 0 - mi;
		sprintf(b2,
			"info depth %d seldepth %lld nodes %lld score mate %d time %lld pv ",
			depth, s2->s[S_depth_max],
			s2->s[S_positionsvisited] + s2->s[S_qposvisited], mi, tno);
		strcat(b2, buff);
	}
	tell_to_engine(b2);
	LOGGER_1("%s\n",b2);
}

void printPV_simple_act(board *b, tree_store *tree, int depth, int side, struct _statistics *s)
{
	int f, xdepth;
	char buff[1024], b2[1024];

	strcpy(buff, "Line: ");
	xdepth = depth;
	xdepth = MAXPLY + 1;
	for (f = 0; f < depth; f++) {
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
	LOGGER_4("Nodes at check %d, mask %d, crit %d\n",b->stats->s[S_nodes], b->run.nodes_mask, b->run.time_crit);
	b->search_abort=engine_stop;
	if (b->run.time_crit == 0) return 0;
	if (b->uci_options->nodes > 0) {
		if (b->stats->s[S_positionsvisited] >= b->uci_options->nodes) {
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
	npsd = b->stats->s[S_nodes] - b->run.nodes_at_iter_start + 1;
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
		&& (b->stats->s[S_positionsvisited] >= b->uci_options->nodes)){
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
		LOGGER_4("INFO: Time out movetime - RUN1, plan: %lld, crit: %lld, iter: %lld, left: %llu, elaps: %lld, frem %lld: %lld < %lld\n", b->run.time_move, b->run.time_crit, tpsd, remain, (tnow-b->run.time_start), frem, frem*b->stats->s[S_ebfnodespri], tpsd*b->stats->s[S_ebfnodes]);
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
	b->run.nodes_at_iter_start = b->stats->s[S_nodes];
	return 0;
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

int QuiesceCheckN(board *b, int talfa, int tbeta, int depth, int ply, int side, tree_store *tree, int checks, attack_model *att)
{
	move_cont mvs;

	move_entry *m, mdum = { MATE_M, 0, 0 - GenerateMATESCORE(ply) }, *mb;
	int opside = Flip(side);
	int pos[4];
	BITVAR rr;

	UNDO u;
	DEB_SE(char b2[256];)
//	char b3[256];
	
	int aftermovecheck = 0;

	b->stats->s[S_nodes]++;
	b->stats->s[S_qposvisited]++;

	if (!(b->stats->s[S_nodes] & b->run.nodes_mask)) {
		if(update_status(b)!=0) {
//			tree->tree[ply][ply].move = NA_MOVE;
			return 0;
		}
	}

	if (b->stats->s[S_depth_max] < ply) {
		b->stats->s[S_depth_max] = ply;
		if (ply >= MAXPLY - 1) {
			tree->tree[ply][ply].move = NA_MOVE;
			return tbeta;
		}
	}
	
	mb = &mdum;

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
		MakeMoveNew(b, m->move, pos, &u);
		rr = ChangesToMove(b, att, &u);
		generateBitmaps(b, att, rr, side);
		generateBitmaps(b, att, rr, opside);
		att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
		att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
		mvsfromk22(b, att, side);
		mvsfromk22(b, att, opside);
#if 1
		if (isInCheck_Eval(b, att, opside)) {
			tree->tree[ply][ply].move |= CHECKFLAG;
			aftermovecheck = 1;
		}
		DEB_SE(
				sprintfMoveSimple(m->move, b2);
				LOGGER_0("%*d, +C , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score);
		)
		if (((checks > 0)) && (aftermovecheck != 0))
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa,
				depth - 1, ply + 1, opside, tree, checks - 1,
				att);
		else
#endif
			if(checks > 0)
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, checks - 1, att);
			else 
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, 0, att);

		UnMakeMoveNew(b, &u, pos);
		rr = ChangesToMove(b, att, &u);
		generateBitmaps(b, att, rr, side);
		generateBitmaps(b, att, rr, opside);
		att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
		att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
		mvsfromk22(b, att, side);
		mvsfromk22(b, att, opside);

		LOGGER_SE("%*d, -C , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, m->real_score);
		if (m->real_score >= tbeta) {
			if (m == mvs.move)
				b->stats->s[S_qfirstcutoffs]++;
			b->stats->s[S_qcutoffs]++;
			b->stats->s[S_Qfailhigh]++;
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
		b->stats->s[S_Qfaillow]++;
	} else
		b->stats->s[S_Qfailnorm]++;

ESTOP:
	b->stats->s[S_qmovestested] += mvs.count;
	b->stats->s[S_qpossiblemoves] += ((mvs.lastp - mvs.move));

	return mb->real_score;
}

int QuiesceNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store *tree, int checks, attack_model *att)
{
	move_cont mvs;
	move_entry *m, mdum = { MATE_M, 0, 0 - GenerateMATESCORE(ply) }, *mb;

	int opside, scr, fullrun;
	int incheck, talfa, tbeta, gmr, aftermcheck;
	int pos[4];
	BITVAR rr;
	UNDO u;
	DEB_SE( char b2[256]; )
//	char b3[256];

	scr = gmr = -mdum.real_score;
	tree->tree[ply][ply].move = tree->tree[ply+1][ply+1].move = tree->tree[ply][ply+1].move = NA_MOVE;
	
	// mate distance pruning
	if (((gmr) <= alfa)||(-gmr >= beta)||(ply >= MAXPLY - 1)) {
//		tree->tree[ply][ply].move = NA_MOVE;
		if (gmr <= alfa) return alfa;
		return beta;
	}
	b->stats->s[S_nodes]++;
	b->stats->s[S_qposvisited]++;
	
	LOGGER_SE("%*d, *Q , EEEE, amove ch:X, depth %d, alfa %d, beta %d\n", 2+ply, ply, depth, alfa, beta);
	
	if (!(b->stats->s[S_nodes] & b->run.nodes_mask)) {
		if(update_status(b)!=0){
//			tree->tree[ply][ply].move = NA_MOVE;
			return 0;
		}
	}

	if (b->stats->s[S_depth_max] < ply) {
		b->stats->s[S_depth_max] = ply;
	}

	opside = Flip(side);
	mb = &mdum;

//	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move) != 0);
	incheck = (isInCheck_Eval(b, att, side)!=0);

	if ((checks > 0) && (is_draw(b, att, b->pers) > 0) && (!incheck)) {
//	if ((is_draw(b, att, b->pers) > 0) && (!incheck)) {
			tree->tree[ply][ply].move = DRAW_M;
			return 0;
	}

	scr = lazyEval(b, att, alfa, beta, side, ply, depth, b->pers, &fullrun);
	if ((scr >= beta)) {
//		tree->tree[ply][ply].move = NA_MOVE;
		return scr;
	}
	if(!incheck)
	{
		talfa = scr > alfa ? scr : alfa;
		mb->real_score = scr;
	}
	else talfa=alfa;

	int ta_re= scr - talfa;
	int tb_re= tbeta - scr;
	

#if 1
	if ((b->pers->use_quiesce == 0) || (ply >= MAXPLY)
	|| (ply > ((b->depth_run * (b->pers->quiesce_depth_limit_multi + 10)) / 10))) {
#else
	if ((b->pers->use_quiesce == 0) || (ply >= MAXPLY)){
#endif
//		tree->tree[ply][ply].move = tree->tree[ply+1][ply+1].move = tree->tree[ply][ply+1].move NA_MOVE;
		return scr;
	}
	tbeta = beta;

// check for king capture & for incheck solution
// find if any move hits other king
//	if (fullrun == 0)
	if (att->att_by_side[side] & normmark[b->king[opside]])
// i have captured king!
//		tree->tree[ply][ply].move = NA_MOVE;
		return beta;

	LOGGER_SE("%*d, *Q , QQQQ, amove ch:X, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);
	
	sortMoveListNew_Init(b, att, &mvs);
	LOGGER_SE("%*d, *Q , SORT, amove ch:X, dpth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, mb->real_score);
	while ((getNextCap(b, att, &mvs, ply, side, incheck, &m, tree) != 0)
		&& (b->search_abort == 0)) {

		tree->tree[ply][ply].move = m->move;
/* 
 * filter out captures not improving our situation
 */
// check if capture can help us at all
		
		MakeMoveNew(b, m->move, pos, &u);
		rr = ChangesToMove(b, att, &u);
		generateBitmaps(b, att, rr, BLACK);
		generateBitmaps(b, att, rr, WHITE);
		att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
		att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
		mvsfromk22(b, att, BLACK);
		mvsfromk22(b, att, WHITE);

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
			incheck2 = att->ke[side].attackers != 0;
			if ((incheck2 != 0)) {
				UnMakeMoveNew(b, &u, pos);
				rr = ChangesToMove(b, att, &u);
				generateBitmaps(b, att, rr, BLACK);
				generateBitmaps(b, att, rr, WHITE);
				att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
				att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
				mvsfromk22(b, att, BLACK);
				mvsfromk22(b, att, WHITE);
				LOGGER_SE("%*d, -Q2 , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score, m->real_score);
//				tree->tree[ply][ply].move = NA_MOVE;
				continue;
			}
		}
#endif
#if 0
//		if (((checks > 0) || ((checks <= 0) && (mb == &mdum)))
		if (((checks > 0) )
			&& (aftermcheck))
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa,
				depth - 1, ply + 1, opside, tree, checks - 1,
				att);
		else
#endif
#if 0
			if(checks > 0)
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, checks - 1, att);
			else
				m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, 0, att);
#endif
		m->real_score = -QuiesceNew(b, -tbeta, -talfa, depth - 1, ply + 1, opside, tree, Max(checks - 1, 0), att);
		UnMakeMoveNew(b, &u, pos);
		rr = ChangesToMove(b, att, &u);
		generateBitmaps(b, att, rr, BLACK);
		generateBitmaps(b, att, rr, WHITE);
		att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
		att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
		mvsfromk22(b, att, BLACK);
		mvsfromk22(b, att, WHITE);

		LOGGER_SE("%*d, -Q , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermcheck, depth, talfa, tbeta, mb->real_score, m->real_score);
		if (m->real_score >= tbeta) {
			if (m == mvs.move)
				b->stats->s[S_qfirstcutoffs]++;
			b->stats->s[S_qcutoffs]++;
			b->stats->s[S_Qfailhigh]++;
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
//	tree->tree[ply][ply+1].move=NA_MOVE;

		b->stats->s[S_qmovestested]+=mvs.count;
		sortMoveListNew_Init(b, att, &mvs);
		while ((getNextCheckin(b, att, &mvs, ply, side, incheck, &m, tree)!=0)&&(b->search_abort==0)) {
//			tree->tree[ply][ply].move=m->move;
//			L0("---\n");
//			sprintfMoveSimple(m->move, b3);
//			L0("Qcheck MOVE %s\n", b3);
			MakeMoveNew(b, m->move, pos, &u);
			rr = ChangesToMove(b, att, &u);
			generateBitmaps(b, att, rr, BLACK);
			generateBitmaps(b, att, rr, WHITE);
			att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
			att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
			mvsfromk22(b, att, BLACK);
			mvsfromk22(b, att, WHITE);

DEB_SE(
			sprintfMoveSimple(m->move, b2);
		LOGGER_0("%*d, +G , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, 1, depth, talfa, tbeta, mb->real_score);
)

			tree->tree[ply][ply].move|=CHECKFLAG;
			tree->tree[ply][ply+1].move=NA_MOVE;
			m->real_score = -QuiesceCheckN(b, -tbeta, -talfa, depth-1, ply+1, opside, tree, checks-1, att);
//			L0("+++\n");
			UnMakeMoveNew(b, &u, pos);
			rr = ChangesToMove(b, att, &u);
			generateBitmaps(b, att, rr, BLACK);
			generateBitmaps(b, att, rr, WHITE);
			att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
			att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
			mvsfromk22(b, att, BLACK);
			mvsfromk22(b, att, WHITE);

			LOGGER_SE("%*d, -G , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, 1, depth, talfa, tbeta, mb->real_score, m->real_score);
			if(m->real_score>=tbeta) {
				b->stats->s[S_qcutoffs]++;
				b->stats->s[S_Qfailhigh]++;
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
		b->stats->s[S_Qfaillow]++;
		tree->tree[ply][ply + 1].move = ALL_NODE;
	} else b->stats->s[S_failnorm]++;

ESTOP:
	b->stats->s[S_qmovestested] += mvs.count;
	b->stats->s[S_qpossiblemoves] += ((mvs.lastp - mvs.move));
	return mb->real_score;
}

// ttbeta 
int SearchMoveNew(board *b, int talfa, int tbeta, int ttbeta, int depth, int ply, int extend, int reduce, int check, int side, tree_store *tree, int nulls, const attack_model *att)
{
	int val, ext;
	int isPV;
	int opside = Flip(side);
	long long int lmrrmoves, zerormoves;

//		if(reduce>0) LOGGER_0("XXX depth %d, extend %d, reduce %d, talfa %d, tbeta %d\n", depth, extend, reduce, talfa, tbeta);

	isPV = (talfa != (tbeta - 1));
	b->stats->s[S_zerototal] += (1 - isPV);
	ext = depth - reduce + extend - 1;
	val = talfa;
	int check_depth = isPV ? b->pers->quiesce_check_depth_limit : 0;
//	int check_depth = b->pers->quiesce_check_depth_limit;
	if (((ext > 0) && (ply < MAXPLY))||(check!=0)) {
		val = -ABNew(b, -ttbeta, -talfa, ext, ply + 1, opside, tree,
			nulls, att);
//	unexpected over alpha? - rerun as it might be because of reduced depth
		if ((val > talfa) && (reduce>0)) {
			lmrrmoves=b->stats->s[S_movestested];
			val = -ABNew(b, -ttbeta, -talfa, depth - 1,
				ply + 1, opside, tree, nulls, att);
				b->stats->s[S_lmrrerun]++;
				if (val <= talfa)
					b->stats->s[S_fhflcount]++;
				b->stats->s[S_lmrrerunnodes] += b->stats->s[S_movestested] - lmrrmoves;
		}
	} else {
		val = -QuiesceNew(b, -ttbeta, -talfa, ext, ply + 1, opside,
			tree, check_depth, att);
		}
// over talfa, open closed zero window
// always talfa < ttbeta, and ttbeta should be talfa+1 or tbeta
	if (((val > talfa && val < tbeta && ttbeta < tbeta))
	&& (b->search_abort == 0)) {
		ext = depth + extend - 1;
		b->stats->s[S_zerorerun]++;
		zerormoves=b->stats->s[S_movestested];
		if (ext > 0 || (check!=0))
			val = -ABNew(b, -tbeta, -talfa, ext, ply + 1, opside, tree, nulls, att);
		else
			val = -QuiesceNew(b, -tbeta, -talfa, ext, ply + 1,
				opside, tree,
				b->pers->quiesce_check_depth_limit, att);
		if (val <= talfa)
			b->stats->s[S_fhflcount]++;
		b->stats->s[S_zerorerunnodes] += b->stats->s[S_movestested] - zerormoves;
//		if (reduce > 0)
//			b->stats->s[S_lmrrerun]++;
	}
	return val;
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
		if ((GT_M0(b, p, b->side, PIECES) == 0))
//			&& (GT_M0(b, p, b->side, PAWN) < 6))
			return 0;
	} else return 1;
	return 1;
}

int can_do_LMP(board *b, attack_model *a, int alfa, int beta, int depth, move_entry *m, int ply, int side, uint8_t phase)
{
	int8_t from, to, rank;
	int prio, reduce;
	
	if(CheckingMove(b, a, side, m)) return 0;
	int isPV = (alfa != (beta - 1));

	to = UnPackTo(m->move);
	from = UnPackFrom(m->move);
//	prom = UnPackProm(m->move);

// promotion
	rank=getRank(from);
	if ((b->pieces[from]&PIECEMASK) == PAWN) {
		if((((side==WHITE)&&(rank==RANKi7||rank==RANKi6))
		||((side==BLACK)&&(rank==RANKi2||rank==RANKi3)))) return 0;
	}
	if ((b->pieces[to]) != ER_PIECE) return 0;

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
int can_do_LMR(board *b, attack_model *a, int alfa, int beta, int depth, move_entry *move, int ply, int side, uint8_t phase, UNDO *u)
{

	int8_t from, movp, ToPos, rank;
	int prio, reduce, sval;
	int isPV = (alfa != (beta - 1));

// promotion
	rank=getRank(u->from);
	if (u->old == PAWN) {
		if((((u->side==WHITE)&&(rank==RANKi7))||((u->side==BLACK)&&(rank==RANKi2)))&& (move->phase<OTHER)) return 0;
	}

// king, stm low on material
#if 1
	if (u->old == KING) {
		if ((GT_M0(b, b->pers, b->side, PIECES) <= 2)) return 0;
	}
#endif

	prio = checkHHTable(b->hht, side, u->old, u->to);

	reduce = b->pers->lmr_table[Min(64,depth)][Min(64,move->ord)];
//	if(move->phase>=OTHER) reduce++;

#if 1
	if (prio > (HHScale/4)) reduce--;
	if (prio > (2*HHScale/3)) reduce--;
	if (prio < -(HHScale/4)) reduce++;
#endif

#if 1
	if(u->whereCa != -1) {
		sval = SEE0(b, u->to, side, u->captured);
		if(sval<0) reduce+=2;
	}
#endif 

#if 0
// alternativa
	if(prio > 5*HHScale/8) reduce = 0;
	else 
	  if (prio > (HHScale/8)) reduce--;

	else 
#endif
#if 0
	  if (prio < -(HHScale/8)) reduce++;
#endif

#if 0
	if(phase<128) reduce--;
#endif

#if 0
	if(phase<=64) reduce--;
#endif

	if(depth>=4 && move->ord > 10 && !isPV) reduce++;

	return CLAMP(reduce, 0, depth-2);
//	return reduce;
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

// when in check is can be entered with depth <= 0
int ABNew(board *b, int alfa, int beta, int depth, int ply, int side, tree_store *tree, int nulls, attack_model *att)
// depth - jak hluboko mam jeste jit, 0 znamena pouze evaluaci pozice, zadne dalsi pultahy
// ply - jak jsem hluboko, 0 jsem v root pozici
{
	int qual;
	int pos[4];
	move_entry *m, mdum = { MATE_M, 0, 0 - GenerateMATESCORE(ply) }, *mb, *mn,
			mt;
	move_cont mvs, *MVS;
	int opside;
	int isPV = (alfa != (beta - 1));
	int pval, sval;
	int incheck, talfa, tbeta, ttbeta, gmr, aftermovecheck;
	int reduce, extend, ext;
	int reduce_o, extend_o;
	unsigned long long nodes_stat, null_stat;
	hashEntry hash;
	BITVAR pld, rr;
	char b2[256];

	UNDO u;
	int futility_sim_flag=0;

	MVS = ply== 0 ? &tree->root_c : &mvs;
	
	b->stats->s[S_nodes]++;
	b->stats->s[S_positionsvisited]++;
	b->stats->s[S_PV_positions]+=isPV;
	
	tree->tree[ply][ply].move = NA_MOVE;
	
	mb = &mdum;
	if (!(b->stats->s[S_nodes] & b->run.nodes_mask)) {
		if(update_status(b)!=0) {
			return 0;
		}
	} 
	LOGGER_SE("%*d, *S , EEEE, amove ch:X, depth %d, talfa %d, tbeta %d,incheck ?\n", 2+ply, ply, depth, alfa, beta);
	DEB_SE(printBoardNice(b);)

	DEB_S2( MVS->alfa=alfa; MVS->beta=beta; MVS->def.state=0; MVS->def.real_score=-mb->real_score; MVS->def.move=NULL_MOVE; )

// mate distance pruning
	gmr = -mb->real_score;

	if (gmr <= alfa) {
		b->stats->s[S_faillow]++;
		DEB_S2(MVS->def.state|=r_ALFA;)
		return alfa;
	}
	if (-gmr >= beta) {
		b->stats->s[S_failhigh]++;
		DEB_S2(MVS->def.state|=r_BETA;)
		return beta;
	}

//!!!
	incheck = (isInCheck_Eval(b, att, side)!=0);
	assert(depth>0||incheck!=0);
//	incheck = (UnPackCheck(tree->tree[ply-1][ply-1].move) != 0);
	DEB_S2(if (incheck) MVS->def.state|=r_CHECK;)
	opside = Flip(side);

	int drw=is_draw(b, att, b->pers);
	if ((( drw > 0)&&(!incheck))||(drw>=3)) {
		mb->move = tree->tree[ply][ply].move = DRAW_M;
		mb->real_score = 0;
		if (mb->real_score <= alfa)
			b->stats->s[S_faillow]++;
		else if (mb->real_score >= beta)
			b->stats->s[S_failhigh]++;
		else
			b->stats->s[S_failnorm]++;
		DEB_S2(MVS->def.state|=r_DRAW;)
		goto ABFINISH2;
	}

// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move = tree->tree[ply+1][ply+1].move = tree->tree[ply][ply+1].move = NA_MOVE;

	talfa = alfa;
	ttbeta = tbeta = beta;
	if (tbeta > gmr)
		tbeta = gmr;
	if (talfa < -gmr)
		talfa = -gmr;

	mt.move = DRAW_M;
	DEB_S2( MVS->def.a=talfa; MVS->def.b=tbeta; )
	
	int hresult=0;
// time to check hash table
// TT CUT off?
	if (b->hs != NULL) {
		hash.key = b->key;
		hash.scoretype = NO_NULL;
		hresult = retrieveHash(b->hs, &hash, side, ply, depth,
			b->pers->use_ttable_prev, b->norm, b->stats);
		if (hresult != 0) {
		// hash hit
			DEB_S2( MVS->def.real_score=hash.value; MVS->def.move=hash.bestmove; MVS->def.state|=r_HASH;)
			mt.real_score = hash.value;
			mt.move = hash.bestmove;
			if ((mt.move == NULL_MOVE)
				|| (isMoveValid(b, mt.move, att, side, tree))) {
				if ((hash.depth >= depth)) {
					tree->tree[ply][ply].move =
						mt.move;
					tree->tree[ply][ply].score = mt.real_score;
					if ((hash.scoretype != FAILHIGH_SC) && (mt.move != NULL_MOVE)
						&& (mt.real_score <= talfa)) {
						b->stats->s[S_faillow]++;
						b->stats->s[S_failhashlow]++;
						mb = &mt;
						goto ABFINISH2;
					} else if ((hash.scoretype != FAILLOW_SC)
						&& (mt.real_score >= tbeta)) {
						b->stats->s[S_failhigh]++;
						b->stats->s[S_failhashhigh]++;
						b->stats->s[S_cutoffs]++;
//						b->stats->s[S_firstcutoffs]++;
						mb = &mt;
						goto ABFINISH2;
					} else if (hash.scoretype == EXACT_SC) {
						b->stats->s[S_failhashnorm]++;
						if ((b->pers->use_hash)) {
// fix situation where exactPV contains repetition and shows PV, beyond 3rd repetition
							if((mt.real_score>talfa) && (mt.real_score<tbeta)) {
							restoreExactPV(b->hs,
								b->key, b->norm,
								ply, tree);
							copyTree(tree, ply);
							}
							b->stats->s[S_failnorm]++;
							mb = &mt;
							goto ABFINISH2;
						} else {
							mt.real_score =
								mdum.real_score;
						}
					} else {
					// hash hit, depth
					}
				} else {
// TT hit, not enough depth
					if ((b->pers->NMP_allowed > 0)
						&& (hash.scoretype
							== FAILLOW_SC)
						&& (hash.depth
							>= (depth
								- b->pers->NMP_reduction
								- 1))
						&& (hash.value < beta))
						nulls = 0;
//					mt.move = DRAW_M;
				}
			} else
				mt.move = DRAW_M;
		}
	}

int sco;
int pvalue;
uint8_t phase=eval_phase(b, b->pers);
// get value of position from hash or lazyEval (mat + psq, phase scaled)

	sco= (hresult!=0) ? hash.value : (side==WHITE) ? getlazyEval(b, b->pers): -getlazyEval(b, b->pers);
// scaled value of PAWN
	pvalue = PVAL(b->pers->Values[0][PAWN], b->pers->Values[1][PAWN], phase, 255);
	
	reduce_o = extend_o = 0;
// reverse futility pruning
// asi necutovat kdyz mam jen pesce
	int pstatef = ((GT_M0(b, b->pers, side, PIECES) == 0) && (GT_M0(b, b->pers, side, PAWN) > 0));

		if ((depth <= b->pers->futility_depth && b->pers->futility_depth>0)
			&& (incheck == 0)
			&& !isPV 
			&& !pstatef
//			&& (hresult==0)
			&& (isMATE2(tbeta) == 0)
			&& (depth>0)
			)

			{
//				if(sco >= ( tbeta + b->pers->futility_cut[depth] )) {
				if(sco >= ( tbeta + (int)(depth*pvalue * 1.2) )) {
					b->stats->s[S_FUT_cuts]++;
					if(b->pers->futility_sim==1) {
						L0("alfa %d, beta %d, talfa %d, tbeta %d, mat_eval %d, depth %d, fcut value %d, est score %d\n", alfa, beta, talfa, tbeta, sco, depth,b->pers->futility_cut[depth], sco - b->pers->futility_cut[depth]);
						futility_sim_flag=1;
					} else {
//						mb->real_score=sco - b->pers->futility_cut[depth];
						mb->real_score=sco - (int)(depth*pvalue * 1.2);
						DEB_S2( MVS->def.real_score=sco; MVS->def.state|=r_FUT;)
						goto ABFINISH2;
					}
				}
//				if(sco+2000 < tbeta) reduce_o=depth; 
		}
		

	aftermovecheck = 0;
// null move PRUNING / REDUCING
	if ((nulls > 0) && (isPV == 0) && (b->pers->NMP_allowed > 0)
		&& (incheck == 0)
		&& (can_do_NullMove(b, att, talfa, tbeta, depth, ply, side) != 0)
		&& (depth > b->pers->NMP_min_depth)
		&& sco >= (tbeta + 500)
		) {
		tree->tree[ply][ply].move = NULL_MOVE;
		MakeNullMove(b, &u);

		LOGGER_SE("%*d, +S , NULL, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, aftermovecheck, depth, talfa, tbeta, mb->real_score);
		
		b->stats->s[S_NMP_tries]++;
// null move reduction, divisor interaction not working

//		reduce = b->pers->NMP_reduction;
//		reduce = b->pers->NMP_reduction + div(depth, b->pers->NMP_div).quot;

		reduce = depth >=5 ? 3:1;

		ext = depth - reduce - 1; //!!!
// save stats, to get info how many nodes were visited due to NULL move...
		nodes_stat = b->stats->s[S_nodes];
		null_stat = b->stats->s[S_u_nullnodes];

		eval_king_checks_extU(b, &(att->ke[WHITE]), 0, b->king[WHITE]);
		eval_king_checks_extU(b, &(att->ke[BLACK]), 1, b->king[BLACK]);

		if (ext > 0) {
			LOGGER_SE("%*d, *S , NULL, AB, alfa %d, beta %d, ext %d, ply %d, nulls %d\n", 2+ply, ply, -tbeta, -tbeta+1, ext, ply+1, nulls-1);
			mt.real_score = -ABNew(b, -tbeta, -tbeta + 1, ext,
				ply + 1, opside, tree, nulls - 1, att);
		} else {
			LOGGER_SE("%*d, *S , NULL, Q, alfa %d, beta %d, ext %d, ply %d, checks %d\n", 2+ply, ply, -tbeta, -tbeta+1, ext, ply+1, b->pers->quiesce_check_depth_limit);
			mt.real_score = -QuiesceNew(b, -tbeta, -tbeta + 1, ext,
			ply + 1, opside, tree,
//				b->pers->quiesce_check_depth_limit, att);
				0, att);
		}

// update null nodes statistics
		UnMakeNullMove(b, &u);
		LOGGER_SE("%*d, -S , NULL, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, aftermovecheck, depth, talfa, tbeta, mb->real_score, mt.real_score);

	generateBitmaps(b, att, b->colormaps[b->side], b->side);
		eval_king_checks_extU(b, &(att->ke[WHITE]), 0, b->king[WHITE]);
		eval_king_checks_extU(b, &(att->ke[BLACK]), 1, b->king[BLACK]);
	mvsfromk22(b, att, b->side);

// engine stop protection?
		if (b->search_abort != 0)
			goto ABFINISH2;
		b->stats->s[S_u_nullnodes] = null_stat
			+ (b->stats->s[S_nodes] - nodes_stat);
		if ((mt.real_score >= tbeta)
		&& (isMATE(mt.real_score)==0)) { //!!!!
			b->stats->s[S_NMP_cuts]++;
			hash.key = b->key;
			hash.depth = (int16_t) depth;
			hash.value = mt.real_score;
			hash.bestmove = NULL_MOVE;
			hash.scoretype = FAILHIGH_SC;
			if ((b->hs != NULL) && (b->search_abort == 0))
				storeHash(b->hs, &hash, side, ply, ext, b->norm, 
					b->stats);
			if (b->pers->NMP_search_reduction == 0) {
				b->stats->s[S_failhigh]++;
				mb = &mt;
				DEB_S2( MVS->def.real_score=mt.real_score; MVS->def.state|=r_NULL; MVS->def.re=ext; )
				goto ABFINISH2;
			} else if (b->pers->NMP_search_reduction == -1) {
				reduce_o = 0;
				mt.move = DRAW_M;
			} else
				reduce_o = b->pers->NMP_search_reduction;
		}
	} else if ((nulls <= 0) && (b->pers->NMP_allowed > 0))
		nulls = b->pers->NMP_allowed;


#if 0
	if (mt.move == DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID ????
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
		b->stats->s[S_position_quality_tests]++;
		if (qual == 0) {
			b->stats->s[S_position_quality_cutoffs]++;
			if (b->pers->quality_search_reduction == -1) {
				tree->tree[ply][ply].move = FAILLOW_SC;
				tree->tree[ply][ply].score = -iINFINITY;
				goto ABFINISH2;
			} else
				reduce_o += b->pers->quality_search_reduction;
		}
	}
#endif

// init moves loop
	sortMoveListNew_Init(b, att, MVS);
	if ((mt.move == DRAW_M) || (mt.move == NULL_MOVE))
		MVS->hash.move = DRAW_M;
	else
		MVS->hash.move = mt.move;
	b->stats->s[S_poswithmove]++;

	int lmr_sim_flag=0;

// main loop
	LOGGER_SE("%*d, *S , XXXX, amove ch:X, depth %d, talfa %d, tbeta %d,incheck %d, best %d\n", 2+ply, ply, depth, talfa, tbeta, incheck, mb->real_score);

	while (((ply==0 ? getNextRootMove(b, att, MVS, ply, side, incheck, &m, tree) : getNextMove(b, att, MVS, ply, side, incheck, &m, tree)) != 0)
		&& (b->search_abort == 0)) {

		m->real_score=-iINFINITY;
		if(!isMoveValid(b, m->move, att, side, tree)) {
			printBoardNice(b);
			L0("invalid move!\n");
			sprintfMoveSimple(m->move, b2);
			LOGGER_0("%*d, +S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score);
			L0("phase:%d\n", m->phase);
			BITVAR pins = ((att->ke[side].cr_pins | att->ke[side].di_pins));
			printmask(pins, "pins");
			int fr = UnPackFrom(m->move);
			printmask(att->mvs[fr], "moves");
			printmask(att->mvk[fr], "masked");
			assert(0);
		}
		
		DEB_S2(m->state=r_NOR; )
		extend = extend_o;
		reduce = reduce_o;
		tree->tree[ply][ply].move = m->move;

// check for LMP conditions based on depth
// !extended !incheck !isPV !first_move use_lmp move mvs.actph >= NORMAL !MATEd
// check after move is done in can_do_LMP as well as promotion

//		if ((MVS->count > (b->pers->LMP_start_move + 2*depth*depth))
		if (((MVS->quiet_pr) > b->pers->LMP_start_move + 2*depth)
			&& (b->pers->LMP_enable > 0)
			&& (depth <= b->pers->LMP_depth)
			&& (depth > 0) // depth <= 0 is happenning only when incheck
			&& (incheck == 0) 
//			&& (extend == extend_o)
			&& (m->phase>KILLER4)
			&& !isPV
//			&& (u.whereCa == -1)
			){
			int lmp_red = can_do_LMP(b, att, talfa, ttbeta, depth, m, ply, side, phase);
				if(lmp_red>0) {
					b->stats->s[S_lmpcount]++;
					goto bypass2;
				}
		}

#if 0
// check for Futility pruning conditions, based on depth
// !extended !incheck !isPV !first_move use_fprune
// getlazyEval + fprune_margin < alfa drop;

		if ((MVS->count > 0)
			&& (depth < b->pers->futility_depth)&&(depth > 0)
			&& (incheck == 0) && (aftermovecheck == 0)
			&& (extend == extend_o)
			&& !(ttbeta== talfa+1)
			&& mb != &mdum
			&& (tbeta<MATEMIN) && (tbeta> -MATEMIN)
			&& (talfa<MATEMIN) && (talfa> -MATEMIN)
			&& (m->phase>=NORMAL)) {
			sco=(side==WHITE) ? get_material_eval_f(b, b->pers): -get_material_eval_f(b, b->pers);
				if((sco+b->pers->futility_cut[depth]) <= talfa) {
					m->state=1;
					if(b->pers->futility_sim==1) {
						L0("alfa %d, beta %d, talfa %d, tbeta %d, sco %d, depth %d, fcut %d\n", alfa, beta, talfa, tbeta, sco, depth,b->pers->futility_cut[depth]);
					} else 
						goto bypass2;
				}
		}
#endif

// perform move & setup node counting

		MakeMoveNew(b, m->move, pos, &u);
		m->nodes = -(b->stats->s[S_movestested]+b->stats->s[S_qmovestested]);
		
// makemove switches board sides, b->side changes during makemove, now b->side==opside

// analyse attacks on king of side to move, incl PINs
// is side to move in check, remember it and extend depth by one

		rr = ChangesToMove(b, att, &u);
		generateBitmaps(b, att, rr, WHITE);
		generateBitmaps(b, att, rr, BLACK);
		att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
		att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
		mvsfromk22(b, att, WHITE);
		mvsfromk22(b, att, BLACK);

		if (isInCheck_Eval(b, att, opside)) {
// idea from Crafty - extend only SAFE moves
#if 1
		if (b->pers->check_extension > 0 && depth>1) {
				pval =
					(u.captured < ER_PIECE) ? u.captured : 0;
				sval = SEE0(b, UnPackTo(m->move), side, pval);
				if (sval >= 0)
					extend += b->pers->check_extension;
			}
#endif 
//		extend++;
			tree->tree[ply][ply].move |= CHECKFLAG;
			aftermovecheck = 1;
			DEB_S2(m->state|=r_CHECK; )
		} else
			aftermovecheck = 0;

// setup window
		ttbeta =
			((MVS->count <= b->pers->PVS_full_moves) && isPV) ? tbeta : talfa + 1;

		DEB_SE(
				sprintfMoveSimple(m->move, b2);
				LOGGER_0("%*d, +S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score);
		)


/*
// check for LMP conditions based on depth
// !extended !incheck !isPV !first_move use_lmp move mvs.actph >= NORMAL !MATEd

//		if ((MVS->count > (b->pers->LMP_start_move + 2*depth*depth))
		if (((MVS->quiet_pr) > b->pers->LMP_start_move + 2*depth*depth)
			&& (b->pers->LMP_enable > 0)
			&& (depth <= b->pers->LMP_depth)
			&& (incheck == 0) 
			&& (aftermovecheck == 0)
//			&& (extend == extend_o)
			&& (m->phase>KILLER4)
			&& !isPV
			&& (u.whereCa == -1)
			){
			int lmp_red = can_do_LMP(b, att, talfa, ttbeta, depth, m, ply, side, phase, &u);
				if(lmp_red>0) {
					b->stats->s[S_lmpcount]++;
					goto bypass;
				}
		}
*/

int lmr_a=talfa;
int lmr_b=tbeta;
int lmr_s=m->real_score;
// setup LMR reductions, not extended, normal moves, not in check, no PV, not giving check, no captures except those ordered behind killers (bad SEE)
// reduce based 
		if ((MVS->count > b->pers->LMR_start_move)
			&& (b->pers->LMR_reduction > 0)
			&& (depth > b->pers->LMR_remain_depth)
			&& (incheck == 0) 
			&& (aftermovecheck == 0)
//			&& (extend == extend_o)
			&& !isPV
//			&& phase>=64
			&& ((u.whereCa == -1)
				|| (m->phase>KILLER4))
			){
			int lmr_red = can_do_LMR(b, att, talfa, tbeta, depth, m, ply, side, phase, &u);
				if(lmr_red!=0) {
				L4("depth %d, move %d, red %d\n", depth, m->order, lmr_red);
				DEB_S2(m->state|=r_LMR;)
					if(b->pers->LMR_sim==0) {
						reduce += lmr_red;
//						reduce += b->pers->LMR_reduction + div(MVS->count,b->pers->LMR_prog_mod).quot;
					} else {
						lmr_sim_flag=1;
					}
					b->stats->s[S_lmrtotal]++;
				}
		}

// ttbeta - temporary beta, either talfa+1 or tbeta !!!!
		b->stats->s[S_movestested]++;
		m->real_score = SearchMoveNew(b, talfa, tbeta, ttbeta, depth,
			ply, extend, reduce, aftermovecheck, side, tree, nulls, att);
bypass:
		DEB_S2(m->a=talfa; m->b=ttbeta; m->re=extend-reduce; m->depth=depth; )

		UnMakeMoveNew(b, &u, pos);
		rr = ChangesToMove(b, att, &u);
		generateBitmaps(b, att, rr, side);
		generateBitmaps(b, att, rr, opside);
		att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
		att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
		mvsfromk22(b, att, side);
		mvsfromk22(b, att, opside);
		m->nodes+=(b->stats->s[S_movestested]+b->stats->s[S_qmovestested]);

bypass2:
		if (b->search_abort != 0)
			goto ABFINISH;

		LOGGER_SE("%*d, -S , %s, amove ch:%d, depth %d, talfa %d, tbeta %d, best %d, val %d\n", 2+ply, ply, b2, aftermovecheck, depth, talfa, tbeta, mb->real_score, m->real_score);

		if (m->real_score >= tbeta) {
// cutoff
			DEB_S2(m->state|=r_BETA; )

			if((b->pers->LMR_sim!=0) && (lmr_sim_flag>0)){
				sprintfMoveSimple(m->move, b2);
				L0("Invalid MOVE %s, score %d, talfa %d, tbeta %d\n",b2, m->real_score, talfa, tbeta);
				printBoardNice(b);
			}

			b->stats->s[S_cutoffs]++;
			if ((m->ord == 0))
				b->stats->s[S_firstcutoffs]++;
			if (is_quiet_move(b, att, m)) {
				b->stats->s[S_quiet_cuts]++;
				if(MVS->cap_pr!=0) b->stats->s[S_quiet_cuts_cap]++;
				if((MVS->quiet_pr==1)||(m->ord==0)) b->stats->s[S_first_quiet_cuts]++;
				
				if ((b->pers->use_killer >= 1)) {
					if ((m->phase>=KILLER1 && m->phase<OTHER)) {
							update_killer_move(b->kmove, ply, m->move, b->stats);
					}
// update history when over beta
					if ((m->phase>=KILLER1 && m->phase<OTHER)||(m->phase==HASHMOVE)) {
						updateHHTableGood(b, b->hht, m, 0, side, depth, ply);
						if(MVS->quiet!=NULL) for(mn=m-1; mn>=MVS->quiet; mn--) updateHHTableBad(b, b->hht, mn, 0, side, depth, ply);
					}
				}
			}
			
			mb = m;
//				copyTree(tree, ply);
			b->stats->s[S_moves_to_cutoff]+=m->ord;
			break;
		}
		DEB_S2({ if (m->real_score <= talfa) m->state|=r_ALFA; else if(m->real_score < tbeta) m->state|=r_IWIN;} )
		if (m->real_score > mb->real_score) {
			mb = m;
			if (mb->real_score > talfa) {
				if((b->pers->LMR_sim!=0) && (lmr_sim_flag>0)){
					lmr_sim_flag=0;
					sprintfMoveSimple(m->move, b2);
					L0("Invalid MOVE %s, score %d, talfa %d, tbeta %d\n",b2, m->real_score, talfa, tbeta);
					printBoardNice(b);
				}
				talfa = mb->real_score;
				copyTree(tree, ply);
			}
		} else if ((ply==1) && (MVS->count==1) && (b->pers->use_aspiration!=0)) {
			goto ABFINISH;
		}
	}

	if (MVS->count <= 0) {
		if (incheck == 0) {
// no moves found, not in check => draw, if incheck means mated - default setting for mb
			mb->real_score = 0;
			mb->move = DRAW_M;
		}
	}
	// update stats & store Hash

	hash.key = b->key;
	hash.depth = (int16_t) depth;
	hash.value = mb->real_score;
	hash.bestmove = mb->move;
	if (mb->real_score > alfa && mb->real_score < beta) {
		b->stats->s[S_failnorm]++;
		hash.scoretype = EXACT_SC;
		if ((b->hs != NULL) && (b->pers->use_hash == 1) && (depth > 0)
			&& (b->search_abort == 0)) {
			storeHash(b->hs, &hash, side, ply, depth, b->norm, b->stats);
//!!!!		
			tree->tree[ply][ply].pld = hash.pld;
			tree->tree[ply][ply].key = b->key;
			tree->tree[ply][ply].ver = b->norm;
			storeExactPV(b->hs, b->key, b->norm, tree, ply);
		}
	} else {
		if (mb->real_score >= beta) {
			b->stats->s[S_failhigh]++;
			hash.scoretype = FAILHIGH_SC;
		} else {
			b->stats->s[S_faillow]++;
			hash.scoretype = FAILLOW_SC;
// poresit statistiku
			b->stats->s[S_non_cutoff_moves]+=MVS->count;
		}
		if ((b->hs != NULL) && (depth > 0))
			storeHash(b->hs, &hash, side, ply, depth, b->norm, b->stats);
	}
ABFINISH:
	tree->tree[ply][ply].move = mb->move;
	tree->tree[ply][ply].score = mb->real_score;

//	b->stats->s[S_movestested] += MVS->count;
	b->stats->s[S_possiblemoves] += ((MVS->lastp - MVS->move));
ABFINISH2:

	if((b->pers->futility_sim!=0) && (futility_sim_flag>0)) {
		if (mb->real_score < beta) {
			L0("Fcut wrong cut %d\n", mb->real_score);
			printBoardNice(b);
		}
		
	}
DEB_4(	if(ply==1) {
		printBoardNice(b);
		printPV_simple_act(b, tree, ply, b->side, b->stats);
		dump_moves(b, MVS, (MVS->lastp-MVS->move), ply, "abdump");
		LOGGER_0("count %d, score %d\n", MVS->count, mb->real_score);
	} )
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
int pos[4];
int aspdiff[]={50, 100, 200, 400, 800, iINFINITY};

int cc, v, xcc, old_score, old_score_count;
MOVESTORE bestmove, hashmove, i, t1pbestmove;
move_entry tm;
move_cont *mvs;

int opside;
int legalmoves, incheck, best, talfa, tbeta, ttbeta, t1pbest, aftermovecheck, isPVcount;
unsigned long long int nodes_bmove;
int extend;
hashEntry hash;
char b2[256];

BITVAR rr;
UNDO u;
attack_model *att, ATT;
unsigned long long tstart, ebfnodesold, tnow;

	old_score = best = 0 - iINFINITY;
	old_score_count = 0;
	b->bestscore = best;
	bestmove = hashmove = NA_MOVE;
	opside = Flip(side);
	copyBoard(b, &(tree->tree_board));

	b->run.iter_start = b->run.time_start;
	b->run.nodes_at_iter_start = b->stats->s[S_nodes];
	b->run.nodes_mask = (1ULL << b->pers->check_nodes_count) - 1;
	b->search_abort=0;

	DEB_1(if((b->uci_options->engine_verbose>=1)) printBoardNice(b);)
	DEB_S2(printBoardNice(b);)

	ply=0;
	b->p_pv.line[ply].move = NA_MOVE;  //???

	att = &ATT;
	att->phase = eval_phase(b, b->pers);

	eval_king_checks_extU(b, &(att->ke[WHITE]), 0, b->king[WHITE]);
	eval_king_checks_extU(b, &(att->ke[BLACK]), 1, b->king[BLACK]);

	generateBitmaps(b, att, FULLBITMAP, BLACK);
	generateBitmaps(b, att, FULLBITMAP, WHITE);
	att->att_by_side[BLACK] = regenerateSQAttacked(b, att, BLACK);
	att->att_by_side[WHITE] = regenerateSQAttacked(b, att, WHITE);
	mvsfromk22(b, att, BLACK);
	mvsfromk22(b, att, WHITE);

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

	setup_root_moves(b, att, side, incheck, tree);
	b->bestmove =  tree->root_c.move[0].move;
	
	if (tree->root_c.tgen == 1) {
		tree->tree[ply][ply].move = tree->root_c.move[0].move;
		tree->tree[ply][ply + 1].move = NA_MOVE;
		tree->tree[ply][ply].score = 0;
		b->bestmove = tree->tree[ply][ply].move;
		b->bestscore = tree->tree[ply][ply].score;
		LOGGER_3("One move play hit\n");
		printPV_simple(b, tree,1,b->side,&s,b->stats);
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
//	clearSearchCnt(b->stats);

	talfa=alfa;
	tbeta=beta;

	/*
	 * b->stats, complete stats for all iterations
	 * s stats at beginning of iteration
	 */
//	clearSearchCnt(&s);
//	clearSearchCnt(&s2);
	clearSearchCnt(b->stats);

	ebfnodesold = 1;

	tstart = readClock();
	if (depth > MAXPLY)
		depth = MAXPLY;

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
		
		b->stats->s[S_ebfnodes]=0;
		b->stats->s[S_ebfnodespri]=0;
		CopySearchCnt(&s, b->stats);
		if (b->hs != NULL) installHashPV(&b->p_pv, b, f - 1, b->stats);
		clear_killer_moves(b->kmove);

		alow=ahigh=0;

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
// search
		best = ABNew(b, talfa, tbeta, f, ply, side, tree, b->pers->NMP_allowed, att);
		
// search has finished
		DEB_S2( move_cont_dump(b, att, &(tree->root_c)); )
	
	if (b->search_abort == 0) {
// was not stopped during last iteration 

		b->stats->s[S_iterations]++;

// handle aspiration if used
// check for problems
// over beta, not rising alfa at fist move or at all
		if (b->pers->use_aspiration != 0) {
			if (tbeta <= best){
// gat failed move and move it to the front
				int i;
				for(i=0; i< tree->root_c.tgen; i++){
					if(tree->root_c.move[i].move == tree->tree[ply][ply].move){
						move_entry tmp =tree->root_c.move[i];
						for(;i>ply;i--) tree->root_c.move[i] = tree->root_c.move[i-1];
						tree->root_c.move[ply] = tmp;
					}
				}
				b->stats->s[S_aspfailits]++;
				ahigh++;
				goto rerun;
			}
			else if(best<=talfa) {
					b->stats->s[S_aspfailits]++;
					alow++;
				goto rerun;
			}
		}
		
		// store proper bestmove & score
		// update stats & store Hash

		{
			if(((tree->tree[ply][ply].move != b->p_pv.line[0].move)
			|| ((best+500) < old_score)) && (f > (start_depth + 1))) {
					b->search_dif =
						Min(1000,
							Max((incheck) ? MISn : MISn, b->search_dif*1.0)
							+((best+500) < old_score)*(old_score-best)/40
							+(tree->tree[ply][ply].move!= b->p_pv.line[0].move)*30);
			} else b->search_dif = Max(100, (best<0) ? b->search_dif -25 : b->search_dif * 0.85) ;

			store_PV_tree(tree, &b->p_pv);
			
			bestmove=tree->tree[ply][ply].move;

			if (old_score == best) {
				old_score_count++;
				if ((old_score_count >= 3)
					&& (GetMATEDist(b->bestscore) <= (f - 1)))
					break;
			} else {
				old_score = best;
				old_score_count = 0;
			}
			
			// re sort moves, setup sort keys
			tree->root_c.next=tree->root_c.move;
			for(int i=0; i<tree->root_c.tgen; i++) tree->root_c.move[i].qorder=tree->root_c.move[i].real_score+tree->root_c.move[i].nodes/f/4;
//			for(int i=0; i<tree->root_c.tgen; i++) tree->root_c.move[i].qorder=tree->root_c.move[i].real_score;
//			for(int i=0; i<tree->root_c.tgen; i++) tree->root_c.move[i].qorder=tree->root_c.move[i].nodes;
			for(i=0; i< tree->root_c.tgen; i++){
				if(tree->root_c.move[i].move == tree->tree[ply][ply].move){
					tree->root_c.move[i].qorder += 699999;
					break;
				}
			}
			SelectBestO(&(tree->root_c));
		}  // finished iteration
   DEB_1 (if(b->uci_options->engine_verbose>=2) dumpHHTable(b->hht);)
	} else {
// last iteration was not finished properly
		DEB_S2( move_cont_dump(b, att, &(tree->root_c)); )
//		dumpHHTable(b->hht);
		
		if ((((&tree->root_c)->count)>1) && (tree->tree[ply][ply].move!=NA_MOVE)) {
			old_score = best;
			bestmove = tree->tree[ply][ply].move;
		} else if (f > start_depth) {
			restore_PV_tree(&b->p_pv, tree);
		} else {
			bestmove = tree->root_c.move[0].move;
			old_score = -MATEMAX;
		}
	  
		tree->tree[ply][ply].move = bestmove;
		tree->tree[ply][ply].score = old_score;
	}

	b->bestmove = tree->tree[ply][ply].move;
	b->bestscore = tree->tree[ply][ply].score;

	oldPVcheck = 1;

	tnow = readClock();
	b->stats->s[S_elaps] += (tnow - tstart);

	if (b->search_abort == 0) {
		b->stats->s[S_ebfnodespri] = ebfnodesold;
//		ebfnodesold = (b->stats->s[S_nodes] - s.nodes);
//		ebfnodesold = (b->stats->s[S_positionsvisited] - s.s[S_positionsvisited]);
		ebfnodesold = (b->stats->s[S_positionsvisited]);
		b->stats->s[S_ebfnodes] = ebfnodesold;
// calculate only finished iterations
		b->stats->s[S_depth] = f;
//		L0("Iter %d, time %d, nodes %lld, prev it nodes %lld, EBF=%f, speed=%f\n", f, tnow-tstart, b->stats->s[S_ebfnodes], b->stats->s[S_ebfnodespri], 
//		(float)b->stats->s[S_ebfnodes]/(float)(b->stats->s[S_ebfnodespri]+1), (float) b->stats->s[S_ebfnodes]/(float)(tnow-tstart));
		tstart = tnow;
	}
// compute difference betweem start and end of an iteration
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
	if (b->uci_options->engine_verbose >= 1)
#pragma omp critical
		printPV_simple(b, tree, f, b->side, &s, b->stats);
//		printSearchStat(&STATS[MAXPLY]);
	}  //deepening finished here


//   DEB_1 (if(b->uci_options->engine_verbose>=2) dumpHHTable(b->hht);)
	
// only finished depths
	if ((b->search_abort != 0)) f--;
// accumulate depths over runs
	b->stats->s[S_depth_sum] += f;
	b->stats->s[S_depth_max_sum] += b->stats->s[S_depth_max];

#pragma omp critical
{
// global stats - update move related
#if 0
	if (STATS[f].s[S_depth] < b->stats->s[S_depth])
		STATS[f].s[S_depth] = b->stats->s[S_depth];
	if (STATS[f].s[S_depth_max] < b->stats->s[S_depth_max])
		STATS[f].s[S_depth_max] = b->stats->s[S_depth_max];
	STATS[f].s[S_depth_sum] += b->stats->s[S_depth];
	STATS[f].s[S_depth_max_sum] += b->stats->s[S_depth_max];
#endif

	if (STATS[MAXPLY].s[S_depth] < b->stats->s[S_depth])
		STATS[MAXPLY].s[S_depth] = b->stats->s[S_depth];
	if (STATS[MAXPLY].s[S_depth_max] < b->stats->s[S_depth_max])
		STATS[MAXPLY].s[S_depth_max] = b->stats->s[S_depth_max];
	STATS[MAXPLY].s[S_depth_sum] += b->stats->s[S_depth];
	STATS[MAXPLY].s[S_depth_max_sum] += b->stats->s[S_depth_max];
	STATS[MAXPLY].s[S_ITsearch]++;

	printPV_simple(b, tree,f,b->side,&s,b->stats);

//	DEB_1 (if((b->uci_options->engine_verbose>=1)) printSearchStat(b->stats);)
	
}
	return b->bestscore;
}
