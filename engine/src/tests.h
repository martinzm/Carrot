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

#ifndef TESTS_H
#define TESTS_H
#include "bitmap.h"
#define CMTLEN 256

struct _results {
	struct _statistics stats;
	unsigned long long int time;
	int passed;
	int bestscore;
	int dm;
	char move[10];
	char fen[1024];
};

void perft2(char*, int, int, int);
void perft2x(char*, int, int, int, int);
void perft2_def(int, int, int);

void timed2Test(char*, int, int, int);
void timed2Test_IQ(char*, int, int, int);
void timed2Test_comp(char*, int, int, int);
void timed2STS(int, int, int, char*, char*);
void timed2STSn(int, int, int, int, char*, char*);
void timed2STSex(char**, int*, int, int, int, int, char*, char*);
void keyTest_def(void);
void timed2_def(int time, int depth, int max);
void timed2_remis(int time, int depth, int max);
void see_test();
void see0_test();

//void texel_test();
void timed2Test_x(char*, int, int, int);

int computeMATIdx(board *b);
void fill_test();

void pawnEvalTest(char*, int);
void EvalCompare(char*[], int, char*[], int, int);
void eval_checker(char*, int);
void eval_checker2(char*, int);
void eval_checker3(char*, int);
int parseEPD(char *buffer, char FEN[100], char (*am)[CMTLEN], char (*bm)[CMTLEN], char (*pv)[CMTLEN], char (*cm)[CMTLEN], char *cm9, int *matec, char **name);
void analyzer_1(char *, int, int, int, int, int, char*);
void eval_qui_checker(char *filein, char *fileout, int max_positions);

int movegensort_test(char *, char *);
int test_hh(char*, char*);



#endif
