#include "defines.h"
#include "ui.h"
#include "search.h"
#include "evaluate.h"
#include "movgen.h"
#include "generate.h"
#include "attacks.h"
#include "sys/time.h"
#include "hash.h"
#include "utils.h"
#include "pers.h"
#include "openings.h"
#include "globals.h"
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

#define MOVES_RET_MAX 64
#define moves_ret_update(x) if(x<MOVES_RET_MAX-1) moves_ret[x]++; else  moves_ret[MOVES_RET_MAX-2]++

tree_node prev_it_global[TREE_STORE_DEPTH+1];
tree_node o_pv_global[TREE_STORE_DEPTH+1];

int moves_ret[MOVES_RET_MAX];
attack_model ATT_A[TREE_STORE_DEPTH];

int DEPPLY=30;

int inPV;
unsigned long long COUNT;


#if 1
int TRIG;
#endif

void store_PV_tree(tree_store * tree, tree_node * pv )
{
	int f;
	for(f=0;f<=TREE_STORE_DEPTH;f++) {
		pv[f]=tree->tree[0][f];
		copyBoard(&(tree->tree[0][f]).tree_board, &(pv[f]).tree_board);
		copyAttModel(&(tree->tree[0][f]).att, &(pv[f]).att);
	}
}

void copyTree(tree_store * tree, int level)
{
	int f;
	if(level>TREE_STORE_DEPTH) {
		printf("Error Depth: %d\n", level);
		abort();
	}
	for(f=level+1;f<=TREE_STORE_DEPTH;f++) {
		tree->tree[level][f]=tree->tree[level+1][f];
		copyBoard(&(tree->tree[level+1][f]).tree_board, &(tree->tree[level][f]).tree_board);
		copyAttModel (&(tree->tree[level+1][f]).att, &(tree->tree[level][f]).att);
	}
}

void clearSearchCnt(struct _statistics * s)
{
	s->faillow=0;
	s->failhigh=0;
	s->failnorm=0;
// doopravdy otestovanych tahu	
	s->movestested=0;
	s->qmovestested=0;
// all possible moves from visited position
	s->possiblemoves=0;
	s->qpossiblemoves=1;
// zero window run	
	s->zerototal=0;
// zero window rerun/ alpha improved in zero	
	s->zerorerun=0;
	s->lmrtotal=0;
	s->lmrrerun=0;
// quiesce zero rerun
	s->quiesceoverrun=0;
	s->positionsvisited=0;
	s->qposvisited=0;
	s->fhflcount=0;
	s->firstcutoffs=0;
	s->cutoffs=0;
	s->NMP_tries=0;
	s->NMP_cuts=0;

// hash info
	posBPV=0;
	tempposBPV=0;
	nodeprintcount=NODE_PRINTCOUNT;
}

void clearALLSearchCnt(struct _statistics * s) {
int f;
	for(f=TREE_STORE_DEPTH;f>=0;f--) {
		clearSearchCnt(&(s[f]));
	}
}

void AddSearchCnt(struct _statistics * s, struct _statistics * b)
{
	s->faillow+=b->faillow;
	s->failhigh+=b->failhigh;
	s->failnorm+=b->failnorm;
	s->movestested+=b->movestested;
	s->qmovestested+=b->qmovestested;
	s->possiblemoves+=b->possiblemoves;
	s->qpossiblemoves+=b->qpossiblemoves;
	s->zerototal+=b->zerototal;
	s->zerorerun+=b->zerorerun;
	s->lmrtotal+=b->lmrtotal;
	s->lmrrerun+=b->lmrrerun;
	s->quiesceoverrun+=b->quiesceoverrun;
	s->positionsvisited+=b->positionsvisited;
	s->qposvisited+=b->qposvisited;
	s->fhflcount+=b->fhflcount;
	s->firstcutoffs+=b->firstcutoffs;
	s->cutoffs+=b->cutoffs;
	s->NMP_tries+=b->NMP_tries;
	s->NMP_cuts+=b->NMP_cuts;
}

void DecSearchCnt(struct _statistics * s, struct _statistics * b, struct _statistics * r)
{
	r->faillow=	s->faillow-b->faillow;
	r->failhigh= s->failhigh-b->failhigh;
	r->failnorm= s->failnorm-b->failnorm;
	r->movestested= s->movestested-b->movestested;
	r->qmovestested= s->qmovestested-b->qmovestested;
	r->possiblemoves= s->possiblemoves-b->possiblemoves;
	r->qpossiblemoves= s->qpossiblemoves-b->qpossiblemoves;
	r->zerototal= s->zerototal-b->zerototal;
	r->zerorerun= s->zerorerun-b->zerorerun;
	r->lmrtotal= s->lmrtotal-b->lmrtotal;
	r->lmrrerun= s->lmrrerun-b->lmrrerun;
	r->quiesceoverrun= s->quiesceoverrun-b->quiesceoverrun;
	r->positionsvisited= s->positionsvisited-b->positionsvisited;
	r->qposvisited= s->qposvisited-b->qposvisited;
	r->fhflcount= s->fhflcount-b->fhflcount;
	r->firstcutoffs= s->firstcutoffs-b->firstcutoffs;
	r->cutoffs= s->cutoffs-b->cutoffs;
	r->NMP_tries= s->NMP_tries-b->NMP_tries;
	r->NMP_cuts= s->NMP_cuts-b->NMP_cuts;
}

void printSearchStat(struct _statistics *s)
{
char buff[1024];
	sprintf(buff, "Low %lld, High %lld, Normal %lld, Positions %lld, MovesSearched %lld (%lld%%) of %lld TotalMovesAvail. Branching %lld, %lld\n", s->faillow, s->failhigh, s->failnorm, s->positionsvisited, s->movestested, (s->movestested*100/(s->possiblemoves+1)), s->possiblemoves, (s->movestested/(s->positionsvisited+1)), (s->possiblemoves/(s->positionsvisited+1)));
	LOGGER_0("Info:",buff,"");
	sprintf(buff, "QPositions %lld, QMovesSearched %lld,(%lld%%) of %lld QTotalMovesAvail\n", s->qposvisited, s->qmovestested, (s->qmovestested*100/s->qpossiblemoves), s->qpossiblemoves);
	LOGGER_0("Info:",buff,"");
	sprintf(buff, "ZeroN %lld, ZeroRerun %lld, QZoverRun %lld, LmrN %lld, LmrRerun %lld, FhFlCount: %lld\n", s->zerototal, s->zerorerun, s->quiesceoverrun, s->lmrtotal, s->lmrrerun, s->fhflcount);
	LOGGER_0("Info:",buff,"");
	sprintf(buff, "Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%, \n",s->firstcutoffs, s->cutoffs,100*s->firstcutoffs/(s->cutoffs+1));
	LOGGER_0("Info:",buff,"");
	sprintf(buff, "NULL MOVE: Tries %lld, Cuts %lld, Ratio %lld%%, \n",s->NMP_tries, s->NMP_cuts,100*s->NMP_cuts/(s->NMP_tries+1));
	LOGGER_0("Info:",buff,"");
}

void printALLSearchCnt(struct _statistics * s) {
int f;
char buff[1024];
	for(f=0;f<=30+1;f++) {
		sprintf(buff, "Level %d", f);
		LOGGER_1("Stats:",buff,"\n");
		printSearchStat(&(s[f]));
	}
	LOGGER_1("Stats:","Konec","\n");
}

void installHashPV(tree_node * pv, int depth)
{
hashEntry h;
	int f, s, mi, ply;
	// !!!!
//	depth=999;
// neulozime uplne posledni pozici ???
	for(f=0; f<depth; f++) {
		switch(pv[f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
			break;
		default:
//			sprintfMove(&(tree->tree[0][f].tree_board), tree->tree[0][f].move, b2);
			h.key=pv[f].tree_board.key;
			h.map=pv[f].tree_board.norm;
			h.value=pv[f].score;
			h.bestmove=pv[f].move;
			storePVHash(&h,f);
			break;
		}
	}
}



void clearPV(tree_store * tree) {
	int f;
	for(f=0;f<=TREE_STORE_DEPTH;f++) {
		tree->tree[0][f].move=NA_MOVE;
		tree->tree[f][f].move=NA_MOVE;
	}
}

void sprintfPV(tree_store * tree, int depth, char *buff)
{
	int f, s, mi, ply;
	char b2[1024];

	buff[0]='\0';
	// !!!!
	depth=999;
	for(f=0; f<=depth; f++) {
		switch(tree->tree[0][f].move) {
		case DRAW_M:
		case NA_MOVE:
		case WAS_HASH_MOVE:
		case ALL_NODE:
		case BETA_CUT:
		case MATE_M:
			//				mi=GetMATEDist(tree->tree[0][0].score);
			sprintfMove(&(tree->tree[0][f].tree_board), tree->tree[0][f].move, b2);
			strcat(buff, b2);
//			strcat(buff," ");
			f=depth+1;
			break;
		default:
			sprintfMove(&(tree->tree[0][f].tree_board), tree->tree[0][f].move, b2);
			strcat(buff, b2);
//			strcat(buff," ");
			if(tree->tree[0][f+1].move!=MATE_M) strcat(buff," ");
			break;
		}
	}
	if(isMATE(tree->tree[0][0].score))  {
		ply=GetMATEDist(tree->tree[0][0].score);
		if (ply==0) mi=1;
		else {
			mi= tree->tree[0][0].tree_board.side==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	if(mi==-1) sprintf(b2,"EVAL:%d", tree->tree[0][0].score); else sprintf (b2,"MATE in:%d", mi);
	strcat(buff, b2);
}

void printPV(tree_store * tree, int depth)
{
char buff[1024];

	buff[0]='\0';
// !!!!
	sprintfPV(tree, depth, buff);
	LOGGER_1("BeLine:", buff, "\n");

}

void printPV_simple(board *b, tree_store * tree, int depth, struct _statistics * s, struct _statistics * s2)
{
int f, mi, xdepth, ply;
char buff[1024], b2[1024];
unsigned long long int tno;

	buff[0]='\0';
	xdepth=depth;
// !!!!
	xdepth=999;
	for(f=0; f<=xdepth; f++) {
		switch(tree->tree[0][f].move) {
			case DRAW_M:
			case MATE_M:
			case NA_MOVE:
			case WAS_HASH_MOVE:
			case ALL_NODE:
			case BETA_CUT:
				f=xdepth+1;
				break;
			default:
				sprintfMoveSimple(tree->tree[0][f].move, b2);
				strcat(buff, b2);
				strcat(buff," ");
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

	if(isMATE(tree->tree[0][0].score))  {
		ply=GetMATEDist(tree->tree[0][0].score);
		if (ply==0) mi=1;
		else {
			mi= tree->tree[0][0].tree_board.side==WHITE ? (ply+1)/2 : (ply/2)+1;
		}
	} else mi=-1;

	tno=readClock()-b->time_start;
	
	if(mi==-1) sprintf(b2,"info score cp %d depth %d nodes %lld time %d pv %s\n", tree->tree[0][0].score/10, depth, s->movestested+s2->movestested+s->qmovestested+s2->qmovestested, tno, buff);
	else sprintf (b2,"info score mate %d depth %d nodes %lld time %d pv %s\n", mi, depth, s->movestested+s2->movestested+s->qmovestested+s2->qmovestested, tno, buff);
	tell_to_engine(b2);
	// LOGGER!!!
}


// called inside search
int update_status(board *b){
char buf[512];
	unsigned long long int tnow;
	sprintf(buf, "Node test: %d,", b->stats.movestested);
	LOGGER_1("UPDT:",buf,"\n");
	if(b->uci_options.nodes>0) {
		if (b->stats.positionsvisited >= b->uci_options.nodes) engine_stop=1;
		return 0;
	}
//tnow milisekundy
// movetime je v milisekundach
//
	tnow=readClock();
		if ((b->time_crit + b->time_start) < tnow){
			sprintf(buf, "Time out loop - time_move_u, %d, %llu, %llu, %lld", b->time_move, b->time_start, tnow, (tnow-b->time_start));
			LOGGER_1("INFO:",buf,"\n");
			engine_stop=1;
		}
	return 0;
}

// called after iteration
int search_finished(board *b){

unsigned long long tnow, slack;
char buf[512];


// moznosti ukonceni hledani
// pokud ponder nebo infinite, tak hledame dal
	if((b->uci_options.infinite==1)||(b->uci_options.ponder==1)) {
		return 0;
	}
// mate in X
// depth
// nodes

	if (engine_stop) {
		LOGGER_1("INFO:","Engine stop called","\n");
		return 9999;
	}

	if(b->uci_options.nodes>0) {
		if (b->stats.positionsvisited >= b->uci_options.nodes) return 1;
		else return 0;
	}

// time per move
	if(b->time_move==0) {
		return 0;
	}

	tnow=readClock();
	slack=tnow-b->iter_start;

	if(b->uci_options.movetime>0) {
		if ((b->time_crit + b->time_start) < tnow) {
			sprintf(buf, "Time out - movetime, %d, %llu, %llu, %lld", b->uci_options.movetime, b->time_start, tnow, (tnow-b->time_start));
			LOGGER_1("INFO:",buf,"\n");
			return 2;
		}
	} else if ((tnow - b->time_start) > b->time_crit){
		sprintf(buf, "Time out - time_move, %d, %llu, %llu, %lld", b->time_crit, b->time_start, tnow, (tnow-b->time_start));
		LOGGER_1("INFO:",buf,"\n");
		return 3;
	} else {
// konzerva
		if(b->uci_options.movestogo==1) return 0;
//		if((3.5*slack)>(b->time_crit-slack)) {
		if((((tnow-b->time_start+5)*2)>b->time_crit)||(((tnow-b->time_start+5)*1.5)>b->time_move)) {
			sprintf(buf, "Time out run - time_move, %d, %llu, %llu, %lld", b->time_move, b->time_start, tnow, (tnow-b->time_start));
			LOGGER_1("INFO:",buf,"\n");
			return 33;
		}
	}
	

	b->iter_start=tnow;
	return 0;
}

int can_do_NullMove(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side){
int pieces;

	if((depth<b->pers->NMP_min_depth) || (alfa != (beta-1))) return 0;

	pieces=BitCount((b->norm^b->maps[PAWN])&b->colormaps[b->side]);
// potreba dodelat evaluaci a podminky
// ne sach, ne PV
// mam tam vic nez jen krale a pesce, resp alespon 2 dalsi figury
// score je vetsi rovno beta
// je prostor pro redukci? - To mozna dam do search jako prechod do quiescence
	return (pieces>3) && (a->sc.complete >= beta);
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
 
int can_do_LMR(board *b, attack_model *a, int alfa, int beta, int depth, int ply, int side, move_entry *move)
{
int inch2;
// zakazani LMR - 9999
	if((depth<b->pers->LMR_remain_depth) || (alfa != beta-1)) return 0;
	if( move->qorder>KILLER_OR-10) return 0;

// utoci neco na krale strany na tahu?
	inch2=AttackedTo_A(b, b->king[b->side], b->side);
	if(inch2!=0) return 0;

return 1;
}

/*
 * Quiescence looks for quiet positions, ie where no checks, no captures etc take place
 *
 */

int Quiesce(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, search_history *hist, int phase)
{
	attack_model *att, ATT;
	move_entry move[300];
//	char bb[2048];
	int hashmove, bestmove, cc, xcc;
	int val;

	move_entry *m, *n;
	int opside;
	int legalmoves, incheck, talfa, tbeta, gmr;
	int best, scr;
	int tc;
	hashEntry hash;

	int psort;
	UNDO u;
//	char b2[2048];

	
	copyBoard(b, &(tree->tree[ply][ply].tree_board));
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=NA_MOVE;
	
	xcc=-1;
//	att=&(ATT_A[ply]);
	att=&ATT; 
	att->phase=phase;
	eval_king_checks_all(b, att);
	eval(b, att, b->pers);

	if(side==WHITE) scr=att->sc.complete;
	else scr=0-att->sc.complete;

	if(b->pers->use_quiesce==0) return scr;

	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0;
	}

	best=scr;

	b->stats.qposvisited++;
//	tree->tree[ply][ply].move=NA_MOVE;
	
	if(engine_stop!=0) {
		return scr;
	}

	opside = (side == WHITE) ? BLACK : WHITE;
//	copyBoard(b, &(tree->tree[ply][ply].tree_board));
	gmr=GenerateMATESCORE(ply);
	
	// is opposite side in check ?
	if(isInCheck_Eval(b, att, opside)!=0) {
		tree->tree[ply][ply].move=MATE_M;
		LOGGER_1("ERR:","Opside in check!","\n");
		printBoardNice(b);
		printPV(tree,ply);
		return gmr;
	}

	// mate distance pruning
/*
		if((gmr) <= alfa) {
			return alfa;
		}
		if(-gmr >= beta) {
			return beta;
		}
 */

	talfa=alfa;
	tbeta=beta;

	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0) {
		incheck=1;
//		talfa=-iINFINITY;
	}	else {
		incheck=0;
		if(scr>=beta) {
			return scr;
		}
		if(scr>talfa) talfa=scr;
	}

	bestmove=NA_MOVE;

	hashmove=DRAW_M;
	legalmoves=0;
// time to check hash table

	hash.key=b->key;
	hash.map=b->norm;

/*	if(retrieveHash(&hash, side, ply)!=0) {
//		hashmove=hash.bestmove;
		if(hash.depth>=depth) {
			if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=beta)) {
			}
			if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=alfa)){
			}
			if(hash.scoretype==EXACT_SC) {
			}
		}
	} else hashmove=DRAW_M;
*/
	m = move;
	n = move;

/*
 * Quiescence should consider
 * hash move
 * good / neutral captures
 * promotions
 * checking moves - generate quiet moves giving check
 * all moves when in check
 *
 */
 
 // FIXME QuietCheck moves se negeneruji pokud je strana v DEPTH==0 v sachu!!!
	if(incheck==1){
		generateInCheckMoves(b, att, &m);
		tc=sortMoveList_Init(b, att, hashmove, move, m-n, depth, 1 );
		getNSorted(move, tc, 0, 1);
//		best = AlphaBeta(b, talfa, tbeta, 1,  ply, side, tree, hist, phase);
//		return best;
	}
	else {
		generateCaptures(b, att, &m, 0);
		if(depth==0) generateQuietCheckMoves(b, att, &m);
		tc=sortMoveList_QInit(b, att, hashmove, move, m-n, depth, 1 );
		getNSorted(move, tc, 0, 1);
	}

//	dump_moves(b, n, m-n, ply);

	if(tc<1) psort=tc;
	else psort=1;

	cc = 0;
	b->stats.qpossiblemoves+=tc;

	while ((cc<tc)&&(engine_stop==0)) {
		if(psort==0) {
			psort=1;
			getNSorted(move, tc, cc, psort);
		}
		if(!(b->stats.movestested & b->nodes_mask)){
			update_status(b);
		}

//		printBoardNice(b);
//		printfMove(b, move[cc].move);
		if(move[cc].qorder>=(A_OR2+32*P_OR-K_OR)&&(move[cc].qorder<=(A_OR2+32*Q_OR))) {
		}
		else {
			u=MakeMove(b, move[cc].move);
			b->stats.qmovestested++;
			b->stats.movestested++;
			{
				tree->tree[ply][ply].move=move[cc].move;
				val = -Quiesce(b, -tbeta, -talfa, depth-1,  ply+1, opside, tree, hist, phase);
			}

			//		UnMakeMove(b, u);
			move[cc].real_score=val;
			legalmoves++;

			if(val>best) {
				best=val;
				xcc=cc;
				bestmove=move[cc].move;
				if(val > talfa) {
					talfa=val;
					if(val >= tbeta) {
						//					tree->tree[ply][ply].move=bestmove;
						//					tree->tree[ply][ply].score=best;
						tree->tree[ply][ply+1].move=BETA_CUT;
						UnMakeMove(b, u);
						break;
					}
					else {
						//					tree->tree[ply][ply].move=bestmove;
						//					tree->tree[ply][ply].score=best;
						//					copyBoard(b, &(tree->tree[ply][ply].after_board));
						copyTree(tree, ply);
					}
				}
			}
			UnMakeMove(b, u);
			psort--;
		}
		cc++;
	}

	if(legalmoves==0) {
		if(incheck==0) {
			// FIXME -- DRAW score, hack - PAWN is 1000
//			best=-200;
// kdo vi, jen tu neni zadne brani
			best=talfa;
			bestmove=NA_MOVE;
		}	else 	{
// I was mated! So best is big negative number...
			best=0-gmr;
			bestmove=MATE_M;
		}
	}
// restore best
	tree->tree[ply][ply].move=bestmove;
	tree->tree[ply][ply].score=best;

	// update stats & store Hash

	hash.key=b->key;
	hash.depth=depth;
	hash.map=b->norm;
	hash.value=best;
	hash.bestmove=bestmove;
	if(best>beta) {
		b->stats.failhigh++;
//		hash.scoretype=FAILHIGH_SC;
	} else {
		if(best<alfa){
			b->stats.faillow++;
			tree->tree[ply][ply+1].move=ALL_NODE;
		} else {
			b->stats.failnorm++;
//			hash.scoretype=EXACT_SC;
		}
	}
	return best;
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
 *
 * takze FailSoft!!!
 * upravovat Alfa - hodnota ktere urcite mohu dosahnout
 * upravovat Beta - hodnota kterou kdyz prekrocim tak si o uroven vyse tah vedouci do teto pozice nevyberou
 * udrzovat aktualni hodnotu nezavisle na A a B
 
 * - best - zatim nejvyssi hodnota
 * - bestmove - odpovidajici tah
 * - val - hodnota prave spocitaneho tahu
 */

int AlphaBeta(board *b, int alfa, int beta, int depth, int ply, int side, tree_store * tree, search_history *hist, int phase, int nulls)
// depth - jak hluboko mam jit, 0 znamena pouze evaluaci pozice, zadne dalsi pultahy
// ply - jak jsem hluboko, 0 jsem v root pozici
{
	int tc,cc, xcc;
	move_entry move[300];
	int bestmove, hashmove;
	move_entry *m, *n;
	int opside, isPV;
	int val, legalmoves, incheck, best, talfa, tbeta, gmr;
	int reduce, extend;
	struct _statistics s, r;

//	char b2[2048], b3[256];
	hashEntry hash;

	int psort;

	int ddd=0;

	UNDO u;
	attack_model *att;

	if(b->pers->negamax==0) {
	// nechceme AB search, ale klasicky minimax
		alfa=0-iINFINITY;
		beta=iINFINITY;
	}

	isPV= alfa != beta-1;
	best=0-iINFINITY;
	bestmove=NA_MOVE;

	opside = (side == WHITE) ? BLACK : WHITE;
	copyBoard(b, &(tree->tree[ply][ply].tree_board));
	
// inicializuj zvazovany tah na NA
	tree->tree[ply][ply].move=NA_MOVE;
	tree->tree[ply+1][ply+1].move=NA_MOVE;
	tree->tree[ply][ply+1].move=WAS_HASH_MOVE;
	
	att=&(tree->tree[ply][ply].att);
	att->phase=phase;
	
//	eval(b, att, b->pers);
	eval_king_checks_all(b, att);
	
	if (is_draw(b, att, b->pers)>0) {
		tree->tree[ply][ply].move=DRAW_M;
		return 0; //!!!
	}

	gmr=GenerateMATESCORE(ply);

	// is opposite side in check ?
	if(isInCheck_Eval(b, att, opside)!=0) {
		tree->tree[ply][ply].move=MATE_M;
		LOGGER_1("ERR:","Opside in check!","\n");
		printBoardNice(b);
		printPV(tree,ply);
		return gmr; //!!!
	}
// mate distance pruning

	talfa=alfa;
	tbeta=beta;
	switch(isMATE(alfa)) {
	case -1:
		if((0-gmr)>=tbeta) return 0-gmr;
		if(talfa<(0-gmr)) talfa=0-gmr;
		break;
	case 1:
		if((gmr-1)<=talfa) return gmr-1;
		if((gmr)<tbeta) tbeta=gmr;
		break;
	default:
		break;
	}

	clearSearchCnt(&s);
	AddSearchCnt(&s, &(b->stats));
//	clearSearchCnt(&(b->stats));
	b->stats.positionsvisited++;
	b->stats.possiblemoves++;

	// is side to move in check ?
	if(isInCheck_Eval(b, att, side)!=0) {
		incheck=1;
	}	else incheck=0;

	{
		b->stats.positionsvisited++;
// time to check hash table
		hash.key=b->key;
		hash.map=b->norm;
		if(b->pers->use_ttable==1 && (retrieveHash(&hash, side, ply)!=0)) {
			hashmove=hash.bestmove;
//FIXME je potreba nejak ukoncit PATH??
			if(hash.depth>=depth) {
				if((hash.scoretype!=FAILLOW_SC)&&(hash.value>=tbeta)) {
					b->stats.failhigh++;
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					AddSearchCnt(&(b->stats), &s);
					return hash.value; //!!!
				}
				if((hash.scoretype!=FAILHIGH_SC)&&(hash.value<=talfa)){
					b->stats.faillow++;
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					AddSearchCnt(&(b->stats), &s);
					return hash.value; //!!!
				}
				if(hash.scoretype==EXACT_SC) {
					tree->tree[ply][ply].move=hash.bestmove;
					tree->tree[ply][ply].score=hash.value;
					if(b->pers->use_hash) return hash.value; //!!!
// !!!!
//					talfa= hash.value-1;
				}
			}
		} else {
			hashmove=DRAW_M;
		}
		
// null move
		if(nulls && b->pers->NMP_allowed && (incheck==0) && can_do_NullMove(b, att, talfa, tbeta, depth, ply, side)) {
			u=MakeNullMove(b);
			b->stats.NMP_tries++;
			extend=0;
			reduce=b->pers->NMP_reduction;
// zero window (with reductions)
			if((depth-reduce+extend-1)>0) {
				val = -AlphaBeta(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase, nulls-1);
			} else {
//				tree->tree[ply][ply+1].move=NA_MOVE; //???
//				tree->tree[ply+1][ply+1].move=NA_MOVE; //???
				val = -Quiesce(b, -(talfa+1), -talfa, depth-1,  ply+1, opside, tree, hist, phase);
			}
			UnMakeNullMove(b, u);
			if(val>=tbeta) {
				tree->tree[ply][ply].move=NULL_MOVE;
				tree->tree[ply][ply].score=val;
				b->stats.NMP_cuts++;
				AddSearchCnt(&(b->stats), &s);
				
				hash.key=b->key;
				hash.depth=depth;
				hash.map=b->norm;
				hash.value=val;
				hash.bestmove=NULL_MOVE;
				hash.scoretype=FAILHIGH_SC;
				if(b->pers->use_ttable==1) storeHash(&hash, side, ply, depth);
				return val; //!!!
			}
		}
		
		if(hashmove==DRAW_M) {
// no hash, if we are deep enough and not in zero window, try IID
// IID, vypnout - 9999
			if((depth>b->pers->IID_remain_depth) && (isPV)) {
				val = AlphaBeta(b, talfa, tbeta, depth-b->pers->IID_remain_depth,  ply, side, tree, hist, phase, 0);
				// still no hash?, try everything!
				if(val < talfa) val = AlphaBeta(b, -iINFINITY, tbeta, depth-b->pers->IID_remain_depth,  ply, side, tree, hist, phase, 0);
				if((b->pers->use_ttable==1) && retrieveHash(&hash, side, ply)!=0) {
					hashmove=hash.bestmove;
				} else {
					hashmove=DRAW_M;
				}
			}
		}

// generate bitmaps for movegen
		simple_pre_movegen(b, att, b->side);
// to get attacked bitmap of other side. 
// optimalizace: mozno ziskat z predchoziho pul tahy
		simple_pre_movegen(b, att, opside);
		
		tree->tree[ply+1][ply+1].move=NA_MOVE;
		tree->tree[ply][ply+1].move=WAS_HASH_MOVE;

		legalmoves=0;
		m = move;

		if(incheck==1) {
			generateInCheckMoves(b, att, &m);
// vypnuti nastavenim check_extension na 0
//			extend+=b->pers->check_extension;
		} else {
			generateCaptures(b, att, &m, 1);
			generateMoves(b, att, &m);
		}
		
		n = move;
		tc=sortMoveList_Init(b, att, hashmove, move, m-n, depth, 1 );

//		log_divider(NULL);
//		printPV(tree, ply-1);
//		dump_moves(b, n, m-n, ply);

		if(tc<1) psort=tc;
		else {
			psort=1;
		}

		cc = 0;
		getNSorted(move, tc, cc, psort);
		b->stats.possiblemoves+=tc;

		// main loop
		while ((cc<tc)&&(engine_stop==0)) {
			extend=0;
			reduce=0;
			if(psort==0) {
				psort=1;
				getNSorted(move, tc, cc, psort);
			}
			if(!(b->stats.movestested & b->nodes_mask)){
				update_status(b);
			}

//			printBoardNice(b);
//			printfMove(b, move[cc].move);
			u=MakeMove(b, move[cc].move);
			b->stats.movestested++;

// vloz tah ktery aktualne zvazujeme - na vystupu z funkce je potreba nastavit na BESTMOVE!!!
			tree->tree[ply][ply].move=move[cc].move;
//			tree->tree[ply+1][ply+1].move=NA_MOVE;

// is side to move in check
// the same check is duplicated one ply down in eval
			eval_king_checks_all(b, att);
			if(isInCheck_Eval(b ,att, b->side)) {
				extend+=b->pers->check_extension;
			}


// debug check
			compareDBoards(b, DBOARDS);
			compareDPaths(tree,DPATHS,ply);

// vypnuti ZERO window - 9999
			if(cc<b->pers->PVS_full_moves) {
				// full window
				if(depth+extend-1 > 0) val = -AlphaBeta(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, 0);
				else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
			} else {
// vypnuti LMR - LMR_start_move - 9999
				if(cc>=b->pers->LMR_start_move && (incheck==0) && can_do_LMR(b, att, talfa, tbeta, depth, ply, side, &(move[cc]))) {
					reduce=b->pers->LMR_reduction;
					b->stats.lmrtotal++;
					b->stats.zerototal++;
// zero window (with reductions)
					if(depth-reduce+extend-1 > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
					else val = -Quiesce(b, -(talfa+1), -talfa, depth-reduce+extend-1,  ply+1, opside, tree, hist, phase);
// if alpha raised rerun without reductions, zero window
					if(val>talfa) {
						b->stats.lmrrerun++;
						if(depth+extend-1 > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
						else val = -Quiesce(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
						if(val<=talfa) b->stats.fhflcount++;
//alpha raised, full window search
						if(val>talfa && val < tbeta) {
							b->stats.zerorerun++;
							if(depth+extend-1 > 0) val = -AlphaBeta(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
							else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
							if(val<=talfa) b->stats.fhflcount++;
						}
					}
				} else {
// zero window without reductions
					if(depth+extend-1 > 0) val = -AlphaBeta(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
					else val = -Quiesce(b, -(talfa+1), -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
					b->stats.zerototal++;
//alpha raised, full window search
					if(val>talfa && val < tbeta) {
						b->stats.zerorerun++;
						if(depth+extend-1 > 0) val = -AlphaBeta(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase, b->pers->NMP_allowed);
						else val = -Quiesce(b, -tbeta, -talfa, depth+extend-1,  ply+1, opside, tree, hist, phase);
						if(val<=talfa) b->stats.fhflcount++;
					}
				}
			}
//			sprintf(b3,"**** Back at LEVEL %u ****\n", ply);
//			log_divider(b3);
//			UnMakeMove(b, u);
			move[cc].real_score=val;
			
			legalmoves++;

			if(val>best) {
				xcc=cc;
				best=val;
				bestmove=move[cc].move;
				if(val > talfa) {
					talfa=val;
					if(val >= tbeta) {
// cutoff
						if(cc==0) b->stats.firstcutoffs++;
						b->stats.cutoffs++;
// record killer
						if((b->pers->use_killer>=1)&&(is_quiet_move(b, att, &(move[cc])))) {
							update_killer_move(ply, move[cc].move);
						}
// prepinani AB
//						if(b->pers->negamax) {
//							tree->tree[ply][ply].move=bestmove;
//							tree->tree[ply][ply].score=best;
//							copyTree(tree, ply);
							tree->tree[ply][ply+1].move=BETA_CUT;
							UnMakeMove(b, u);
							break;
//						}
					} else {
//						tree->tree[ply][ply].move=bestmove;
//						tree->tree[ply][ply].score=best;
//						copyBoard(b, &(tree->tree[ply][ply].after_board));
						copyTree(tree, ply);
					}
				}
			}
			UnMakeMove(b, u);
//			printBoardNice(b);
			psort--;
			cc++;
		}
		if(legalmoves==0) {
			if(incheck==0) {
// FIXME -- DRAW score, hack - PAWN is 1000
				best=-200;
				bestmove=DRAW_M;
			}	else 	{
				// I was mated! So best is big negative number...
				best=0-gmr;
				bestmove=MATE_M;
			}
		}
		tree->tree[ply][ply].move=bestmove;
		tree->tree[ply][ply].score=best;

		// update stats & store Hash

		hash.key=b->key;
		hash.depth=depth;
		hash.map=b->norm;
		hash.value=best;
		hash.bestmove=bestmove;
//!!!!
		if(best>=tbeta) {
			b->stats.failhigh++;
			hash.scoretype=FAILHIGH_SC;
			if((b->pers->use_ttable==1)&&(depth>0)) storeHash(&hash, side, ply, depth);
		} else {
			if(best<talfa){
				b->stats.faillow++;
				hash.scoretype=FAILLOW_SC;
				if((b->pers->use_ttable==1)&&(depth>0)) storeHash(&hash, side, ply, depth);
//				copyTree(tree, ply);
				tree->tree[ply][ply+1].move=ALL_NODE;
			} else {
				b->stats.failnorm++;
				hash.scoretype=EXACT_SC;
				if((b->pers->use_ttable==1)&&(depth>0)) storeHash(&hash, side, ply, depth);
			}
		}
	}
//	dump_moves(b, n, m-n, ply);
//	printfMove(b, bestmove);
	
	DecSearchCnt(&(b->stats), &s, &r);
	AddSearchCnt(&(STATS[ply]), &r);
	return best; //!!!
}

int IterativeSearch(board *b, int alfa, int beta, const int ply, int depth, int side, int start_depth, tree_store * tree)
{
int f, i;
char buff[1024], b2[2048], bx[2048];
search_history hist;
struct _statistics s, r;

int reduce;

int tc,cc, v, xcc ;
move_entry move[300];
int bestmove, hashmove;
move_entry *m, *n;
int opside;
int legalmoves, incheck, best, talfa, tbeta, nodes_bmove;
int extend;

UNDO u;
attack_model *att, ATT;

tree_node *prev_it;
tree_node *o_pv;
// neni thread safe!!!
		prev_it=prev_it_global;
		o_pv=o_pv_global;


		o_pv[0].move=NA_MOVE;
		initDBoards(DBOARDS);
		initDPATHS(b, DPATHS);

//		att=&(ATT_A[0]);
//		att=&(ATT);
//		sprintf(bx, "Line %d in file %s\n", __LINE__, __FILE__);
//		log_divider(bx);
		xDEBUG=0;
		b->bestmove=NA_MOVE;
		b->bestscore=0;
		bestmove=hashmove=NA_MOVE;
		clearSearchCnt(&s);
		clearSearchCnt(&(b->stats));
		clearALLSearchCnt(STATS);
		
		b->nodes_mask=0x07FF;
		b->iter_start=b->time_start;

//		sprintf(bx, "Line %d in file %s\n", __LINE__, __FILE__);
//		log_divider(bx);

		opside = (side == WHITE) ? BLACK : WHITE;
		copyBoard(b, &(tree->tree[ply][ply].tree_board));

// make current line end here
		tree->tree[ply][ply].move=NA_MOVE;

		att=&(tree->tree[ply][ply].att);
		att->phase = eval_phase(b);
//		eval(b, att, b->pers);
//		copyAttModel(att, &(tree->tree[0][0].att));

//		sprintf(bx, "Line %d in file %s\n", __LINE__, __FILE__);
//		log_divider(bx);

		eval_king_checks_all(b, att);

		// is opposite side in check ?
		if(isInCheck_Eval(b, att, opside)!=0) {
			DEB_2(printf("Opside in check!\n"));
			tree->tree[ply][ply].move=MATE_M;
//????			
			return MATESCORE;
		}

		// is side to move in check ?
		if(isInCheck_Eval(b, att, side)!=0) {
			incheck=1;
		}	else incheck=0;

//		if(isDrawBy50(b)==1) {
//			tree->tree[ply][ply].move=DRAW_M;
//			return 0;
//		}

		if(side==WHITE) best=att->sc.complete;
		else best=0-att->sc.complete;

//		sprintf(bx, "Line %d in file %s\n", __LINE__, __FILE__);
//		log_divider(bx);


// check database of openings
		i=probe_book(b);
		if(i!=NA_MOVE) {
//			printfMove(b, i);
			tree->tree[ply][ply].move=i;
			tree->tree[ply][ply+1].move=NA_MOVE;
			
			tree->tree[ply][ply].score=0;
			b->bestmove=tree->tree[ply][ply].move;
			b->bestscore=tree->tree[ply][ply].score;
			return 0;
		}

//		sprintf(bx, "Line %d in file %s\n", __LINE__, __FILE__);
//		log_divider(bx);

		simple_pre_movegen(b, att, b->side);
		simple_pre_movegen(b, att, opside);
//!!! optimalizace
		o_pv[ply].move=NA_MOVE; //???
		legalmoves=0;
		m = move;
		if(incheck==1) {
			generateInCheckMoves(b, att, &m);
		} else {
			generateCaptures(b, att, &m, 1);
			generateMoves(b, att, &m);
		}
		n = move;

//		sprintf(bx, "Line %d in file %s\n", __LINE__, __FILE__);
//		log_divider(bx);

		alfa=0-iINFINITY;
		beta=iINFINITY;
		talfa=alfa;
		tbeta=beta;
// make hash age by new search not each iteration
		invalidateHash();
		// iterate and increase depth gradually
		for(f=start_depth;f<=depth;f++) {
			if(b->pers->negamax==0) {
				alfa=0-iINFINITY;
				beta=iINFINITY;
				talfa=alfa;
				tbeta=beta;
			}
			clearSearchCnt(&s);
			AddSearchCnt(&s, &(b->stats));
			hashmove=o_pv[ply].move;
			hashmove=NA_MOVE;
			installHashPV(o_pv, f-1);
			clear_killer_moves();
			xcc=-1;
// (re)sort moves
//			boardCheck(b);
			tc=sortMoveList_Init(b, att, hashmove, move, m-n, ply, m-n );
			getNSorted(move, tc, 0, tc);
//			log_divider("**** ROOOT ****\n");
//			dump_moves(b, n, m-n, ply);
			assert(m!=0);
//			boardCheck(b);

			b->stats.positionsvisited++;
			b->stats.possiblemoves+=tc;
			b->stats.depth=f-1;

			/*
			 * **********************************************************************************
			 */
			{
				best=0-iINFINITY;
				inPV=1;

// hack
				cc = 0;
// loop over all moves
// inicializujeme line
				tree->tree[ply][ply].move=NA_MOVE;
				tree->tree[ply][ply+1].move=NA_MOVE;
//				tree->tree[ply+1][ply+1].move=NA_MOVE;
				while ((cc<tc)&&(engine_stop==0)) {
					extend=0;
//					reduce=0;
					if(!(b->stats.movestested & b->nodes_mask)){
						update_status(b);
					}
					nodes_bmove=b->stats.possiblemoves+b->stats.qpossiblemoves;
//					log_divider("**** To LEVEL 1 ****\n");
//					printfMove(b, move[cc].move);
					u=MakeMove(b, move[cc].move);
// aktualni zvazovany tah					
					tree->tree[ply][ply].move=move[cc].move;
//					tree->tree[0][0].score=0;
					b->stats.movestested++;

// is side to move in check
// the same check is duplicated one ply down in eval
					eval_king_checks_all(b, att);
					if(isInCheck_Eval(b ,att, b->side)) {
						extend+=b->pers->check_extension;
					}

					compareDBoards(b, DBOARDS);
					compareDPaths(tree,DPATHS,ply);

// vypnuti ZERO window - 9999
					if(legalmoves<b->pers->PVS_root_full_moves) {
						// full window
						if((f-1+extend)>0) v = -AlphaBeta(b, -tbeta, -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
						else v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase);
					} else {
						if((f-1+extend)>0) v = -AlphaBeta(b, -(talfa+1), -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
						else v = -Quiesce(b, -(talfa+1), -talfa, 0,  1, opside, tree, &hist, att->phase);
						b->stats.zerototal++;
		//alpha raised, full window search
						if(v>talfa && v < tbeta) {
							b->stats.zerorerun++;
							if((f+extend)>0) v = -AlphaBeta(b, -tbeta, -talfa, f-1+extend, 1, opside, tree, &hist, att->phase, b->pers->NMP_allowed);
							else v = -Quiesce(b, -tbeta, -talfa, 0,  1, opside, tree, &hist, att->phase);
							if(v<=talfa) b->stats.fhflcount++;
						}
					}
//					log_divider("**** Back at ROOOOT ****\n");
//					UnMakeMove(b, u);
					move[cc].real_score=v;

					inPV=0;
					move[cc].qorder=b->stats.possiblemoves+b->stats.qpossiblemoves-nodes_bmove;
					legalmoves++;
					if(v>best) {
						best=v;
						bestmove=move[cc].move;
						xcc=cc;
						if(v > talfa) {
//							printf("Eval1: v:%d, best:%d, talfa:%d, tbeta:%d\n", v, best, talfa, tbeta);
							talfa=v;
							if(v >= tbeta) {
								if(b->pers->use_aspiration==0) {
									LOGGER_1("ERR:","nemelo by jit pres TBETA v rootu","\n");
//									tree->tree[ply][ply].move=bestmove;
//									tree->tree[ply][ply].score=best;
//									copyTree(tree, ply);
								}
								tree->tree[ply][ply+1].move=BETA_CUT;
								UnMakeMove(b, u);
								xcc=-1;
								break;
							}
							else {
//								copyBoard(b, &(tree->tree[0][0].tree_board));
//								copyBoard(b, &(tree->tree[ply][ply].after_board));
								tree->tree[ply][ply].move=bestmove;
								tree->tree[ply][ply].score=best;
								copyTree(tree, ply);
// best line change								
//								printPV(tree, depth);
								if(b->uci_options.engine_verbose>=1) printPV_simple(b, tree, f, &s, &(b->stats));
//								printf("Eval2: v:%d, best:%d, talfa:%d, tbeta:%d\n", v, best, talfa, tbeta);
							}
						}
					}
					UnMakeMove(b, u);
					cc++;
				}
				if(legalmoves==0) {
					if(incheck==0) {
						best=0;
//						tree->tree[ply][ply].move=DRAW_M;
						bestmove=DRAW_M;
					}	else 	{
//????
						best=0-MATESCORE;
//						tree->tree[ply][ply].move=MATE_M;
						bestmove=MATE_M;
					}
				}
				if(best>beta) {
					b->stats.failhigh++;
				} else {
					if(best<alfa){
						b->stats.faillow++;
					} else {
						b->stats.failnorm++;
					}
				}
			}
//			dump_moves(b, n, m-n, ply);
//			printfMove(b, bestmove);
// store proper bestmove & score
			tree->tree[ply][ply].move=bestmove;
			tree->tree[ply][ply].score=best;

			
// hack
			store_PV_tree(tree, o_pv);
			/*
			 * **********************************************************************************
			 * must handle unfinished iteration
			 */
			if((engine_stop!=0)&&(f>start_depth)) {
				for(i=0;i<(f-1);i++) tree->tree[ply][i]=prev_it[i];
			} else {
				for(i=0;i<f;i++) prev_it[i]=tree->tree[ply][i];
			}
			
			b->bestmove=tree->tree[ply][ply].move;
			b->bestscore=tree->tree[ply][ply].score;

//			log_divider(NULL);
//			DEB_2(sprintf(buff,"***** Result from LEVEL: %d *****\n",f));
//			LOGGER_2("",buff,"");

			DEB_3 (printPV(tree, f));
//			DEB_1 (printScoreExt(att));
//			DEB_1 (printSearchStat(&(b->stats)));
//			DEB_1 (printHashStats());
			DecSearchCnt(&(b->stats),&s,&r);
			AddSearchCnt(&(STATS[0]), &r);
//			AddSearchCnt(&s, &(b->stats));
//			DEB_2(log_divider("=============================================="));
//			DEB_1 (printSearchStat(&s));
//			log_divider(NULL);
//			printALLSearchCnt(STATS);
//			DEB_2(log_divider("LEVEL info END\n"));

// break only if mate is now - not in qsearch
			if(GetMATEDist(b->bestscore)<f) {
				break;
			}
			if((engine_stop!=0)||(search_finished(b)!=0)) break;
			if((b->pers->use_aspiration>0) && (xcc!=-1)) {
// mame realny tah, pro dalsi iteraci pripravime okno okolo bestscore
				talfa=b->bestscore-b->pers->Values[0][ROOK];
				tbeta=b->bestscore+b->pers->Values[0][ROOK];
			} else
			 {
// faillow, nemame tah vratime okno, jak patri
				talfa=alfa;
				tbeta=beta;
// a provedeme tutez iteraci jeste jednou
// nicmene neni vyreseno, co kdyz bez aspiration window nenajdu tah?
				if(xcc==-1) f--;
			}
// time keeping
		}
		if(b->uci_options.engine_verbose>=1) printPV_simple(b, tree, f, &s, &(b->stats));
//		clearSearchCnt(&(b->stats));
		DEB_1 (printSearchStat(&r));
		DEB_1 (printHashStats());
		clearSearchCnt(&s);
		AddSearchCnt(&s, &(b->stats));
		return b->bestscore;
}
