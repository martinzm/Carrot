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

#ifndef STATS_H
#define STATS_H
#include "defines.h"

struct _statistics {
#if 0
	long long failnorm;  // node normalni
	long long faillow;  // node neprekonal alfa
	long long failhigh;  // node prekonal beta
	long long failhashnorm;  // node normalni dle TT
	long long failhashlow;  // node neprekonal alfa, dle TT
	long long failhashhigh;  // node prekonal beta, dle TT
	long long nodes;  // mel by byt souctem positionsvisited a qposvisited, je to pro time management
	long long positionsvisited;  // kolik pozic jsme navstivili? Tj kolikrat jsme vstoupili do Search
	long long movestested;  //kolik TAHU bylo opravdu testovanych - tj kazde MakeMove, ne kazde MakeMove vede do rekurze Search
	long long possiblemoves;  //kolik bylo moznych TAHU
	long long zerototal;
	long long zerorerun;
	long long zerorerunnodes;
	long long quiesceoverrun;
	long long qposvisited;  // stejne jako non q verze
	long long qmovestested;  // stejne jako non q verze
	long long qpossiblemoves;  // stejne jako non q verze
	long long lmrtotal;
	long long lmrrerun;
	long long lmrrerunnodes;
	long long lmpcount;
	long long fhflcount;
	long long firstcutoffs;
	long long cutoffs;
	long long moves_to_cutoff;
	long long non_cutoff_moves;
	long long first_quiet_cuts;
	long long quiet_cuts;
	long long quiet_cuts_cap;
	long long qfirstcutoffs;
	long long qcutoffs;
	long long FUT_cuts;
	long long NMP_cuts;
	long long NMP_tries;
	long long qSEE_tests;
	long long qSEE_cuts;
	long long poswithmove;  //num of positions for movegen ran
	long long ebfnodes;
	long long ebfnodespri;
	long long elaps;
	long long u_nullnodes;
	long long iterations;
	long long aspfailits;

// hash
	long long hashStores;
	long long hashStoreColl;
	long long hashAttempts;
	long long hashHits;
	long long hashColls;
	long long hashMiss;
	long long hashStoreMiss;
	long long hashStoreInPlace;
	long long hashStoreHits;

//	long long hashPawnStores;
	long long hashPawnStoreColl;
	long long hashPawnAttempts;
	long long hashPawnHits;
	long long hashPawnColls;
	long long hashPawnMiss;
	long long hashPawnStoreMiss;
	long long hashPawnStoreInPlace;
	long long hashPawnStoreHits;

	long long position_quality_tests;
	long long position_quality_cutoffs;

	long long depth;
	long long depth_max;
	long long depth_sum;
	long long depth_max_sum;
#endif
	long long s[S_MAX_COLL];
};

void clearSearchCnt(struct _statistics*);
void AddSearchCnt(struct _statistics*, struct _statistics*);
void CopySearchCnt(struct _statistics*, struct _statistics*);
void DecSearchCnt(struct _statistics*, struct _statistics*, struct _statistics*);
void printSearchStat(struct _statistics*);
void printSearchStat2(struct _statistics*, char*);
void clearALLSearchCnt(struct _statistics*);
void printALLSearchCnt(struct _statistics*);
struct _statistics* allocate_stats(int);
void deallocate_stats(struct _statistics*);

#endif
