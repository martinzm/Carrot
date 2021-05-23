
#include "ui.h"
#include "bitmap.h"
#include "generate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tests.h"
#include "hash.h"
#include "pers.h"
#include "utils.h"
#include "evaluate.h"
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

//#define NUM_THREADS 1
#define INPUT_BUFFER_SIZE 16384

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

char eVERS[]="0.30.1";
char eREL[]="Develop";

void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}


//board bs[NUM_THREADS];
// komunikace mezi enginem a uci rozhranim
// bs je nastavovano uci rozhranim a je pouzivano enginem na vypocty
//board bs;

// engine state rika
// uci state dtto pro uci stav
// engine_stop - 0 ok, 1 zastav! pouziti pokud je engine "THINKING"
int engine_state;
int uci_state;

int tell_to_engine(char *s){
	fprintf(stdout, "%s\n", s);
	LOGGER_4("TO_E: %s\n",s);
	return 0;
}

int uci_send_bestmove(MOVESTORE b){
	char buf[50], b2[100];
//	if(b!=0){
	LOGGER_4("INFO: bestmove sending\n");
	sprintfMoveSimple(b, buf);
	sprintf(b2,"bestmove %s\n", buf);
	tell_to_engine(b2);
	LOGGER_3("INFO: Bestmove %s\n", buf);
//	}
	LOGGER_4("INFO: bestmove sent");
	return 0;
}

/*
 * engine states
 * 0 - make_quit
 * 4 - stop_thinking
 * 1 - stopped
 * 3 - start_thinking
 * 2 - thinking
 */


void *engine_thread(void *arg){
	tree_store * moves;
	struct _statistics *stat;
	board *b;
	int f,n,i;

	moves = (tree_store *) malloc(sizeof(tree_store));
	stat = allocate_stats(1);
	moves->tree_board.stats=(stat);
	b=(board *)arg;
	engine_stop=1;
	LOGGER_4("THREAD: started\n");
	while (engine_state!=MAKE_QUIT){
		switch (engine_state) {
		case START_THINKING:
			LOGGER_4("THREAD: Thinking\n");
			engine_stop=0;
			engine_state=THINKING;
			IterativeSearch(b, 0-iINFINITY, iINFINITY ,0 , b->uci_options->depth, b->side,b->pers->start_depth, moves);
			LOGGER_4("THREAD: Iter Left\n");
			engine_stop=4;
			engine_state=STOPPED;
			uci_state=2;
			if(b->bestmove!=0) uci_send_bestmove(b->bestmove);
			else {
				LOGGER_3("INFO: no bestmove!\n");
				uci_send_bestmove(moves->tree[0][0].move);
			}
			break;
		case STOPPED:
			sleep_ms(1);
			LOGGER_4("THREAD: Stopped\n");
			break;
		}
	}
	deallocate_stats(stat);
	free(moves);
	LOGGER_4("THREAD: quit\n");
//	pthread_exit(NULL);
	return arg;
}

int handle_uci(){
	char buff[1024];
	sprintf(buff,"id name ENGINE v%s, REL %s, %s %s\n",eVERS,eREL, __DATE__,__TIME__);
	tell_to_engine(buff);
	sprintf(buff,"id author Martin Zampach\n");
	tell_to_engine(buff);
	sprintf(buff,"uciok\n");
	tell_to_engine(buff);
	return 0;
}

int handle_newgame(board *bs){
	setup_normal_board(bs);
	LOGGER_4("INFO: newgame\n");
	return 0;
}

// potrebuje specialni fixy pro
// promotion - spec flag
// ep - spec flag
// castle - spec flag, fix dest pole
int move_filter_build(char *str, MOVESTORE *m){
	char *tok, *b2;
	int i,a,b,c,d,q,spec;
	MOVESTORE v;
	size_t l;
	// replay moves
	i=0;
	m[0]=0;
	if(!str) return 0;
	tok = tokenizer(str," \n\r\t", &b2);
	if(!tok) return 0;
	do {
		// tah je ve tvaru AlphaNumAlphaNum(Alpha*)

		l=strlen(tok);
		a=b=c=d=q=spec=0;
		if(l>=4 && l<=5){
			if(isalpha(tok[0])) if((tolower(tok[0])>='a') && (tolower(tok[0])<='h')) {
				a=tolower(tok[0])-'a';
			}
			if(isdigit(tok[1])) if((tok[1]>='0') && (tok[1]<='9')) {
				b=tok[1]-'0'-1;
			}
			if(isalpha(tok[2])) if((tolower(tok[2])>='a') && (tolower(tok[2])<='h')) {
				c=tolower(tok[2])-'a';
			}
			if(isdigit(tok[3])) if((tok[3]>='0') && (tok[3]<='9')) {
				d=tok[3]-'0'-1;
			}
// promotion
			q=ER_PIECE;
			if(l==5) {
				if(isalpha(tok[4])) switch (tolower(tok[4])) {
				case 'b':
					q=BISHOP;
					break;
				case 'n':
					q=KNIGHT;
					break;
				case 'r':
					q=ROOK;
					break;
				case 'q':
					q=QUEEN;
					break;
				default:
					spec=0;
					break;
				}
			}
			// ep from, to, PAWN
			// castling e1g1 atd
			v=PackMove(b*8+a, d*8+c,q, 0);
			m[i++]=v;
		}
		if(!b2) break;
		tok = tokenizer(b2," \n\r\t", &b2);
	} while(tok);

	m[i++]=0;
	return i;
}

int handle_position(board *bs, char *str){

char *tok, *b2, bb[100];
int  i, a;
MOVESTORE m[MAXPLYHIST],mm[MAXPLYHIST];

	if(engine_state!=STOPPED) {
		LOGGER_3("UCI: INFO: Not stopped!, E:%d U:%d\n", engine_state, uci_state);
//		engine_stop=1;
		engine_state=STOP_THINKING;

		sleep(1);
		while(engine_state!=STOPPED) {
			LOGGER_3("UCI: INFO: Stopping!, E:%d U:%d\n", engine_state, uci_state);
//			engine_stop=1;
			engine_state=STOP_THINKING;
			sleep_ms(1);
		}
	}

	tok = tokenizer(str," \n\r\t",&b2);
	while(tok){
		LOGGER_4("PARSE: %s\n",tok);

		if(!strcasecmp(tok,"fen")) {
			LOGGER_4("INFO: FEN+moves %s\n",b2);
			setup_FEN_board(bs,b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
			tok = tokenizer(b2," \n\r\t", &b2);
//			break;
		} else if (!strcasecmp(tok,"startpos")) {
			LOGGER_4("INFO: startpos %s\n",b2);
			setup_normal_board(bs);
//			DEB_2(printBoardNice(bs));
//			break;
		} else if (!strcasecmp(tok,"moves")) {
// build filter moves
			move_filter_build(b2,m);
			a=0;
			mm[1]=0;
			DEB_4(printBoardNice(bs));
			while(m[a]!=0) {
				mm[0]=m[a];
//				eval(bs, &att, bs->pers);
				i=alternateMovGen(bs, mm);
				if(i!=1) {
//				printBoardNice(bs);
					LOGGER_0("%s\n",b2);
					LOGGER_0("INFO3: move problem!\n");
					abort();
// abort
				}
//				DEB_3(sprintfMove(bs, mm[0], bb));
				DEB_4(sprintfMoveSimple(mm[0], bb));
				LOGGER_4("MOVES parse: %s\n",bb);

int from;
int oldp;
		from=UnPackFrom(mm[0]);
		oldp=bs->pieces[from]&PIECEMASK;
		
		if((oldp>KING)||(oldp<PAWN)) {
	char buf[256];
			LOGGER_0("Step4 error\n");
			printBoardNice(bs);
			printboard(bs);
			sprintfMoveSimple(mm[0], buf);
			LOGGER_0("while making move %s\n", buf);
			LOGGER_0("From %d, old %d\n", from, oldp );
			abort();
		}

				MakeMove(bs, mm[0]);
				a++;
			}
//			printBoardNice(bs);

//play moves
			break;
		}
		tok = tokenizer(b2," \n\r\t", &b2);
	}
return 0;
}

int ttest_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=-1;
	timed2_def(i, 24, 100);
	return 0;
}

int ttest_remis(char *str){
int i;
	i=atoi(str);
	if(i==0) i=10000;
	timed2_remis(i, 24, 100);
	return 0;
}

int ttest_def2(char *str){
int i;
	i=atoi(str);
	if(i==0) i=10000;
	timed2Test("../tests/test_a.epd", i,99, 9999);
	return 0;
}

int thash_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=90000;
	timed2Test("../tests/test_hash.epd", i, 200, 100);
	return 0;
}

int tpawn_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=90000;
	pawnEvalTest("../tests/test_pawn.epd", i);
	return 0;
}


int thash_def_comp(char *str){
int i;
	i=atoi(str);
	if(i==0) i=90000;
	timed2Test_comp("../tests/test_hash.epd", i, 200, 999);
	return 0;
}

int twac_def_comp(char *str){
int i;
	i=atoi(str);
	if(i==0) i=90000;
	timed2Test_comp("../tests/test_suite_bk.epd", i, 200, 999);
	return 0;
}

int ttsts_def(char *str){
int i;
	i=atoi(str);
	if(i==0) i=1000;
	timed2STS(i, 200, 10, "pers.xml", "pers2.xml");
	return 0;
}

int ttest_wac(char *str){
int i;
	i=atoi(str);
	if(i==0) i=10000;
	timed2Test("../tests/test_wac.epd", i,90, 100);
	return 0;
}

int ttest_wac2(char *str){
int i;
	i=atoi(str);
	if(i==0) i=60000;
	timed2Test("../tests/test_a.epd", i,90, 200);
	return 0;
}

int ttest_null(char *str){
int i;
	i=atoi(str);
	if(i==0) i=60000;
	timed2Test("../tests/test_suite_nullmove.epd", i,90, 100);
	return 0;
}

int ttest_swap_eval(char *str){
int i;
	i=atoi(str);
	if(i==0) i=4;
//	timed2Test_x("../tests/test_eval.pgn", 999,90, i);
	timed2Test_x("../texel/1-0.txt", 999,90, i);
	return 0;
}

int mtest_def(){
	movegenTest("../tests/test_pozice.epd");
	return 0;
}

int handle_go(board *bs, char *str){
	int n, moves, time, inc, basetime, cm, lag;

	char *i[100];

	if(engine_state!=STOPPED) {
		LOGGER_4("UCI: INFO: Not stopped!, E:%d U:%d\n", engine_state, uci_state);
		engine_stop=1;
//		engine_state=STOP_THINKING;

		sleep_ms(1000);
		while(engine_state!=STOPPED) {
			LOGGER_4("UCI: INFO: Stopping!, E:%d U:%d\n", engine_state, uci_state);
			engine_stop=1;
//			engine_state=STOP_THINKING;
			sleep_ms(1000);
		}
	}

	basetime=0;

// ulozime si aktualni cas co nejdrive...
	bs->run.time_start=readClock();

	lag=10; //miliseconds
	//	initialize ui go options

	bs->uci_options->engine_verbose=1;

	bs->uci_options->binc=0;
	bs->uci_options->btime=0;
	bs->uci_options->depth=999999;
	bs->uci_options->infinite=0;
	bs->uci_options->mate=0;
	bs->uci_options->movestogo=0;
	bs->uci_options->movetime=0;
	bs->uci_options->ponder=0;
	bs->uci_options->winc=0;
	bs->uci_options->wtime=0;
	bs->uci_options->search_moves[0]=0;

	bs->uci_options->nodes=0;

	bs->run.time_move=0;
	bs->run.time_crit=0;

// if option is not sent, such option should not affect/limit search

//	LOGGER_4("PARSEx: %s\n",str);
	n=indexer(str, " \n\r\t",i);
//	LOGGER_4("PARSE: indexer %i\n",n);

	if((n=indexof(i,"wtime"))!=-1) {
// this time is left on white clock
		bs->uci_options->wtime=atoi(i[n+1]);
		LOGGER_4("PARSE: wtime %s\n",i[n+1]);
	}
	if((n=indexof(i,"btime"))!=-1) {
		bs->uci_options->btime=atoi(i[n+1]);
		LOGGER_4("PARSE: btime %s\n",i[n+1]);
	}
	if((n=indexof(i,"winc"))!=-1) {
		bs->uci_options->winc=atoi(i[n+1]);
		LOGGER_4("PARSE: winc %s\n",i[n+1]);
	}
	if((n=indexof(i,"binc"))!=-1) {
		bs->uci_options->binc=atoi(i[n+1]);
		LOGGER_4("PARSE: binc %s\n",i[n+1]);
	}
	if((n=indexof(i,"movestogo"))!=-1) {
// this number of moves till next time control
		bs->uci_options->movestogo=atoi(i[n+1]);
		LOGGER_4("PARSE: movestogo %s\n",i[n+1]);
	}
	if((n=indexof(i,"depth"))!=-1) {
// limit search do this depth
		bs->uci_options->depth=atoi(i[n+1]);
		LOGGER_4("PARSE: depth %s\n",i[n+1]);
	}
	if((n=indexof(i,"nodes"))!=-1) {
// limit search to this number of nodes
		bs->uci_options->nodes=atoi(i[n+1]);
		LOGGER_4("PARSE: nodes %s\n",i[n+1]);
	}
	if((n=indexof(i,"mate"))!=-1) {
// search for mate this deep
		bs->uci_options->mate=atoi(i[n+1]);
		LOGGER_4("PARSE: mate %s\n",i[n+1]);
	}
	if((n=indexof(i,"movetime"))!=-1) {
// search exactly for this long
		bs->uci_options->movetime=atoi(i[n+1]);
		LOGGER_4("PARSE: movetime %s\n",i[n+1]);
	}
	if((n=indexof(i,"infinite"))!=-1) {
// search forever
		bs->uci_options->infinite=1;
		LOGGER_4("PARSE: infinite\n");
	}
	if((n=indexof(i,"ponder"))!=-1) {
		bs->uci_options->ponder=1;
		LOGGER_4("PARSE: ponder\n");
	}
	if((n=indexof(i,"searchmoves"))!=-1) {
//		uci_options.searchmoves=atoi(i[n+1]);
		LOGGER_4("PARSE: searchmoves %s IGNORED",i[n+1]);
	}

	// pred spustenim vypoctu jeste nastavime limity casu
	if(bs->uci_options->infinite!=1) {
		if(bs->uci_options->movetime!=0) {
// pres time_crit nejede vlak a okamzite konec
// time_move je cil kam bychom meli idealne mirit a nemel by byt prekrocen pokud neni program v problemech
// time_move - target time
			bs->run.time_move=bs->uci_options->movetime*2;
			bs->run.time_crit=bs->uci_options->movetime-lag;
		} else {
			if(bs->uci_options->movestogo==0){
// sudden death
				moves=50; //fixme
			} else moves=bs->uci_options->movestogo;
			if((bs->side==0)) {
				time=bs->uci_options->wtime;
				inc=bs->uci_options->winc;
				cm=bs->uci_options->btime-bs->uci_options->wtime;
			} else {
				time=bs->uci_options->btime;
				inc=bs->uci_options->binc;
				cm=bs->uci_options->wtime-bs->uci_options->btime;
			}
			if(time>0) {
				basetime=((inc-lag)*moves+time)/(moves+2);
				if(basetime>time) basetime=time;
// booster between 20 - 40 ply
				if((bs->move>=40)&&(bs->move<=80)) {
					basetime*=150;
					basetime/=100;
				} else if(cm>0) {
					basetime*=90; //!!!
					basetime/=100;
				}
				bs->run.time_crit=basetime*2;
				bs->run.time_move=basetime;
				if(bs->run.time_crit>=time) {
					bs->run.time_crit=time;
					bs->run.time_move=time/2;
				}
			}
		}
	}
	DEB_2(printBoardNice(bs));
	LOGGER_2("TIME: wtime: %llu, btime: %llu, time_crit %llu, time_move %llu, basetime %llu\n", bs->uci_options->wtime, bs->uci_options->btime, bs->run.time_crit, bs->run.time_move, basetime );
//	engine_stop=0;
	//invalidateHash(bs->hs);

//	bs->time_start=readClock();

	bs->move_ply_start=bs->move;
	bs->pers->start_depth=1;
	uci_state=4;
	engine_state=START_THINKING;

	LOGGER_4("UCI: go activated\n");
	sleep_ms(1);

	return 0;
}

int handle_stop(){
	LOGGER_4("UCI: INFO: STOP has been received from UI\n");
	while(engine_state!=STOPPED) {
		LOGGER_4("UCI: INFO: running, E:%d U:%d\n", engine_state, uci_state);
		engine_stop=1;
		sleep_ms(1);
	}
	LOGGER_4("UCI: INFO: stopped, E:%d U:%d", engine_state, uci_state);
	return 0;
}

board * start_threads(){
	board *b;
	pthread_attr_t attr;
	b=malloc(sizeof(board)*1);
	b->stats=allocate_stats(1);
	clearALLSearchCnt(STATS);
	b->uci_options=malloc(sizeof(struct _ui_opt));
	b->hs=allocateHashStore(HASHSIZE, 2048);
	b->hps=allocateHashPawnStore(HASHPAWNSIZE);
	engine_state=STOPPED;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&(b->run.engine_thread),&attr, engine_thread, (void *) b);
	pthread_attr_destroy(&attr);
return b;
}

int stop_threads(board *b){
void *status;
	engine_state=MAKE_QUIT;
	sleep_ms(1);
	pthread_join(b->run.engine_thread, &status);
	DEB_2(printALLSearchCnt(STATS));
	freeHashPawnStore(b->hps);
	freeHashStore(b->hs);
	deallocate_stats(b->stats);
	free(b->uci_options);
	free(b);
return 0;
}

int uci_loop(int second){
	char *buff, *tok, *b2;

	int16_t move_o;
	size_t inp_len;
	int bytes_read;
	int position_setup=0;
	board *b;
	
	clear_killer_moves();
	b=start_threads();
	uci_state=1;

	buff = (char *) malloc(INPUT_BUFFER_SIZE+1);
	inp_len=INPUT_BUFFER_SIZE;

	LOGGER_4("INFO: UCI started\n");

/*
 * 	setup personality
 */

	if(second) {
		b->pers=(personality *) init_personality("pers2.xml");
	} else {
		b->pers=(personality *) init_personality("pers.xml");
	}

	move_o=-1;
	/*
	 * parse and dispatch UCI protocol/commands
	 * uci_states
	 * 0 - quit
	 * 1 - non uci/waiting for uci
	 * 2 - uci handled, idle
	 * 3 - computing
	 */

	while(uci_state!=0){

		/*
		 * wait & get line from standard input
		 */
		bytes_read=(int) getline(&buff, (&inp_len), stdin);
		if(bytes_read==-1){
			LOGGER_1("INFO: input read error!\n");
			break;
		}
		else{
reentry:
			LOGGER_4("FROM:%s",buff);
			tok = tokenizer(buff," \n\r\t",&b2);
			while(tok){
				LOGGER_4("PARSE: %d %s\n",uci_state,tok);
				if(!strcasecmp(tok,"quit")) {
					handle_stop();
					uci_state=0;
					engine_stop=1;
					break;
				} else if (!strcasecmp(tok,"isready")) {
					tell_to_engine("readyok\n");
					break;
				}
				if(uci_state==1) {
					if(!strcmp(tok,"uci")) {
						handle_uci();
						uci_state=2;
						break;
					}
					if(!strcmp(tok,"perft")) {
						perft2_def(1,7,0);
						break;
					}
					if(!strcmp(tok,"perft1")) {
						perft2("../tests/test_perft.epd",1, 7, 1);
					}
					if(!strcmp(tok,"perft2")) {
						perft2("../tests/test_perft.epd",1, 11, 1);
					}
					if(!strcmp(tok,"perft3")) {
						perft2("../tests/test_perftsuite.epd",2, 14, 0);
					}
					if(!strcmp(tok, "testsee")) {
						see_test();
					}
					if(!strcmp(tok, "testev")) {
						eeval_test();
					}
					if(!strcmp(tok, "testsee0")) {
						see0_test();
					}
					if(!strcmp(tok, "ttswap")) {
						ttest_swap_eval(b2);
					}
					if(!strcmp(tok,"ttdef")) {
						ttest_def(b2);
						break;
					}
					if(!strcmp(tok,"ttremis")) {
						ttest_remis(b2);
						break;
					}
					if(!strcmp(tok,"ttfile")) {
						ttest_def2(b2);
						break;
					}
					if(!strcmp(tok,"wac")) {
						ttest_wac(b2);
						break;
					}
					if(!strcmp(tok,"wac2")) {
						ttest_wac2(b2);
						break;
					}
					if(!strcmp(tok,"ttnull")) {
						ttest_null(b2);
						break;
					}
					if(!strcmp(tok,"mts")) {
						mtest_def();
						break;
					}
					if(!strcmp(tok,"tthash")) {
						thash_def(b2);
						break;
					}
					if(!strcmp(tok,"ttpawn")) {
						tpawn_def(b2);
						break;
					}
					if(!strcmp(tok,"tthashc")) {
						thash_def_comp(b2);
						break;
					}
					if(!strcmp(tok,"ttcc")) {
						twac_def_comp(b2);
						break;
					}
					if(!strcmp(tok,"ttsts")) {
						ttsts_def("10000");
						break;
					}
					if(!strcmp(tok,"texel")) {
						texel_test();
						break;
					}
					if(!strcmp(tok,"wpers")) {
						write_personality(b->pers, "pers_test.xml");
						break;
					}
					if(!strcmp(tok, "mtst")) {
//						strcpy(buff, "position startpos moves e2e4 g8f6 e4e5 f6g8 d2d4 b8c6 g1f3 d7d6 f1b5 a7a6 b5c6 b7c6 e1g1 f7f6 h2h3 d6e5 d4e5 d8d1 f1d1 c8d7 a2a3 e8c8 b1c3 e7e6 e5f6 g7f6 c1e3 g8e7 e3c5 e7d5 c3e4 f6f5");
//						strcpy(buff, "position startpos moves e2e4 c7c6 g1f3 d7d5 b1c3 g8f6 e4d5 c6d5 f1b5 b8c6 b5c6 b7c6 f3e5 d8b6 e1g1");
						strcpy(buff, "position startpos moves d2d3 d7d5 c1f4");
						uci_state=2;
						goto reentry;
					} else if(!strcasecmp(tok,"my")){
//hack
//						strcpy(buff,"startpos moves e2e3 g8f6 d1f3 d7d5 f1b5 c7c6 b5d3 b8d7 f3f5 d7c5 f5f4 c5d3 c2d3 d8d6 f4f3 g7g6 b1c3 c8f5 d3d4 f8h6 g1e2 e8g8\n");
//						strcpy(buff,"startpos moves e2e3 g8f6 d1f3\n");
//						strcpy(buff,"fen r2qk2r/p2nbppp/bpp1p3/3p4/2PP4/1PB3P/P2NPPBP/R2QK2R b KQkq - 2 22 moves e8h8 e1g1\n");
//						strcpy(buff, "position fen r1b1k2r/1p1p1ppp/2q1pn2/4P3/p1Pn1P2/2N3P1/PP1N3P/R2QKB1R w KQkq - 3 16");
//						strcpy(buff, "position startpos moves d2d4 d7d5 g1f3 c7c6 e2e3 e7e6 c2c4 f8d6 b2b3 g8f6 b1c3 b8d7 f1d3 e6e5 c4d5 d6b4 d1c2 e5e4 d5c6 d7b8");
						strcpy(buff,"position startpos moves e2e4 c7c5 g1f3 b8c6 f1b5 e7e6 e1g1 g8e7 f1e1 a7a6 b5c6 e7c6 b2b3 f8e7 c1b2 e8g8 d2d4 c5d4 f3d4 d7d6 b1d2 c8d7 d4c6 d7c6 e1e3 e7g5 d1g4 g5h6 e3h3 d8g5 g4g5 h6g5 h3d3 g5d2 d3d2 c6e4 d2d6 f8d8 a1d1 d8d6 d1d6 e4c2 d6d7 c2e4 a2a4 a8c8 a4a5 e4c6 d7d1 c6d5 d1d3 c8c2 d3c3 c2c3 b2c3 d5b3 g1f1 b3d5 g2g3 f7f6 f1e1 g8f7 e1d2 e6e5 c3b4 e5e4 d2c3 f7e6 b4c5 e6f5 c3d4 f5e6 d4c3 g7g6 c3c2 d5c4 c2c3 c4d3 c3b3 e6d5 c5b6 d5e6 b6e3 g6g5 e3d4 g5g4 d4e3 d3e2 b3c3 e2b5 c3b2 b5c6 b2b3 c6d5 b3a3 e6f7 a3b2 f7e6 e3c5 e6f5 c5b6 f5e5 b6e3 e5e6 e3c5 e6e5 c5e3 e5f5 e3b6 d5c4 b2c3 c4f7 b6e3 f7a2 e3d4 a2d5 d4c5 f5e5 c5e3 e5e6 e3d4 e6f5 d4c5 f5e5 c5e3 e5e6 e3d4 d5c6 d4b6 c6b5 b6e3 e6d5 c3b3 b5c4 b3b2 c4b5 b2c2 b5d3 c2b3 d3b5 b3a3 b5f1 e3d2 d5c4 a3b2 c4d5 d2c3 d5e6 c3b4 e6e5 b4c3 e5e6 c3b4 e6e5 b2b3 f1d3 b4c3 e5e6 c3d4 d3f1 d4e3 f1b5 b3b2 e6e5 e3f4 e5f5 f4c7 b5c6 b2c3 f5e6 c3d4 e6f5 d4c3 f5e6 c3d4 e4e3 f2e3 e6f5 c7d8 f5e6 d8c7 e6f5 c7d6 c6f3 d6b4 f5e6 b4c5 f3d5 e3e4 d5c6 c5a7 c6a4 a7c5 a4e8 d4e3 e8a4 e3d4 a4e8 d4e3 e8g6 e3d4 g6f7 c5a3 f7e8 d4e3 e8a4 a3b2 a4d1 b2d4 d1c2 e3f4 c2d3 f4g4 d3e4 g4f4 f6f5 f4e3 e6d6 e3f4 d6e6 f4e3 e4d5 d4c5 d5c6 c5a7 c6b5 a7c5 b5c6 c5a7 c6b5 a7c5 b5c4 c5b4 e6e5 b4c3 e5e6 c3b2 c4b3 b2a3 e6e5 a3b4 b3f7 b4c3 e5e6 e3d4 f7e8 d4c5 e8c6 h2h4 c6f3 c3d2 e6e7 d2g5 e7e6 c5d4 f3d1 g5h6 d1c2 h6g5 c2d1 g5h6 d1c2 h6e3 h7h5 e3c1 c2e4 c1a3 e4f3 a3b4 f3c6 b4d2 c6g2 d4c4 g2c6 d2f4 c6d5 c4c5 d5g2 f4d2 g2h1 d2c3 h1d5 c3b2 d5e4 c5d4 e6f6 d4c4 f6e6 c4d4 e6f6 b2c3 e4h1 c3b4 f6e6 b4f8 h1f3 f8b4 f3g2 b4c5 g2c6 c5b6 c6f3 d4c4 f3g2");

//						handle_position(b, buff);
//						position_setup=1;
						uci_state=2;
						goto reentry;
					}
				} else if(uci_state==2){
					if(!strcasecmp(tok,"ucinewgame")) {
						handle_newgame(b);
						position_setup=1;
						break;
					} else if(!strcasecmp(tok,"position")){
						handle_position(b, b2);
						position_setup=1;
						break;
					} else if(!strcasecmp(tok,"go")){
						if(!position_setup) {
							handle_newgame(b);
							position_setup=1;
						}
						if((b->pers->ttable_clearing>=1)&&(b->move!=(move_o+2))) {
						LOGGER_1("INFO: UCI hash reset\n");
							invalidateHash(b->hs);
						}
						move_o=b->move;
						handle_go(b, b2);
						break;
					} else if(!strcasecmp(tok,"gox")){
						strcpy(buff,"go movetime 1000");
						goto reentry;
					}
				} else if(uci_state==4){
					if(!strcasecmp(tok,"stop")){
						handle_stop();
						uci_state=2;
						break;
					} else {
					}
				}
				tok = tokenizer(b2," \n\r\t", &b2);
			}
			//
		}
	}
	LOGGER_4("INFO: exiting...\n");
	free(b->pers);
	stop_threads(b);
//	deallocate_stats(b->stats);
//	free(b->uci_options);
	LOGGER_2("INFO: UCI stopped\n");
	free(buff);
	return 0;
}

int uci_loop2(int second){
	return 0;
}
