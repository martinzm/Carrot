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

#include <stdlib.h>
#include <string.h>
#include "stats.h"
#include "bitmap.h"
#include "utils.h"

void clearSearchCnt(struct _statistics *s)
{
	s->faillow = 0;
	s->failhigh = 0;
	s->failnorm = 0;
	s->failhashlow = 0;
	s->failhashhigh = 0;
	s->failhashnorm = 0;
	s->nodes = 0;
// doopravdy otestovanych tahu
	s->movestested = 0;
	s->qmovestested = 0;
// all possible moves from visited position
	s->possiblemoves = 0;
	s->qpossiblemoves = 1;
// zero window run
	s->zerototal = 0;
// zero window rerun/ alpha improved in zero
	s->zerorerun = 0;
	s->lmrtotal = 0;
	s->lmrrerun = 0;
	s->lmpcount = 0;
// quiesce zero rerun
	s->quiesceoverrun = 0;
	s->positionsvisited = 0;
	s->qposvisited = 0;
	s->fhflcount = 0;
	s->firstcutoffs = 0;
	s->cutoffs = 0;
	s->moves_to_cutoff = 0;
	s->non_cutoff_moves = 0;
	s->first_quiet_cuts = 0;
	s->quiet_cuts = 0;
	s->quiet_cuts_cap = 0;

	s->qfirstcutoffs = 0;
	s->qcutoffs = 0;
	s->FUT_cuts = 0;
	s->NMP_tries = 0;
	s->NMP_cuts = 0;
	s->qSEE_tests = 0;
	s->qSEE_cuts = 0;
	s->hashStores = 0;
	s->hashStoreColl = 0;
	s->hashAttempts = 0;
	s->hashHits = 0;
	s->hashColls = 0;
	s->hashMiss = 0;
	s->hashStoreMiss = 0;
	s->hashStoreInPlace = 0;
	s->hashStoreHits = 0;

	s->hashPawnStores = 0;
	s->hashPawnStoreColl = 0;
	s->hashPawnAttempts = 0;
	s->hashPawnHits = 0;
	s->hashPawnColls = 0;
	s->hashPawnMiss = 0;
	s->hashPawnStoreMiss = 0;
	s->hashPawnStoreInPlace = 0;
	s->hashPawnStoreHits = 0;

	s->poswithmove = 0;
	s->ebfnodes = 0;
	s->ebfnodespri = 0;
	s->elaps = 0;
	s->u_nullnodes = 0;
	s->iterations = 0;
	s->aspfailits = 0;

	s->depth = 0;
	s->depth_max = 0;
	s->depth_sum = 0;
	s->depth_max_sum = 0;

	s->position_quality_tests = 0;
	s->position_quality_cutoffs = 0;
	
}

// do prvniho parametru je pricten druhy
void AddSearchCnt(struct _statistics *s, struct _statistics *b)
{

	s->faillow += b->faillow;
	s->failhigh += b->failhigh;
	s->failnorm += b->failnorm;
	s->failhashlow += b->failhashlow;
	s->failhashhigh += b->failhashhigh;
	s->failhashnorm += b->failhashnorm;
	s->nodes += b->nodes;
	s->movestested += b->movestested;
	s->qmovestested += b->qmovestested;
	s->possiblemoves += b->possiblemoves;
	s->qpossiblemoves += b->qpossiblemoves;
	s->zerototal += b->zerototal;
	s->zerorerun += b->zerorerun;
	s->lmrtotal += b->lmrtotal;
	s->lmrrerun += b->lmrrerun;
	s->lmpcount += b->lmpcount;
	s->quiesceoverrun += b->quiesceoverrun;
	s->positionsvisited += b->positionsvisited;
	s->qposvisited += b->qposvisited;
	s->fhflcount += b->fhflcount;
	s->firstcutoffs += b->firstcutoffs;
	s->cutoffs += b->cutoffs;
	s->moves_to_cutoff += b->moves_to_cutoff;
	s->non_cutoff_moves += b->non_cutoff_moves;
	s->first_quiet_cuts += b->first_quiet_cuts;
	s->quiet_cuts += b->quiet_cuts;
	s->quiet_cuts_cap += b->quiet_cuts_cap;

	s->qfirstcutoffs += b->qfirstcutoffs;
	s->qcutoffs += b->qcutoffs;
	s->NMP_tries += b->NMP_tries;
	s->NMP_cuts += b->NMP_cuts;
	s->FUT_cuts += b->FUT_cuts;
	s->qSEE_tests += b->qSEE_tests;
	s->qSEE_cuts += b->qSEE_cuts;
	s->hashStores += b->hashStores;
	s->hashStoreColl += b->hashStoreColl;
	s->hashAttempts += b->hashAttempts;
	s->hashHits += b->hashHits;
	s->hashColls += b->hashColls;
	s->hashMiss += b->hashMiss;
	s->hashStoreMiss += b->hashStoreMiss;
	s->hashStoreInPlace += b->hashStoreInPlace;
	s->hashStoreHits += b->hashStoreHits;

	s->hashPawnStores += b->hashPawnStores;
	s->hashPawnStoreColl += b->hashPawnStoreColl;
	s->hashPawnAttempts += b->hashPawnAttempts;
	s->hashPawnHits += b->hashPawnHits;
	s->hashPawnColls += b->hashPawnColls;
	s->hashPawnMiss += b->hashPawnMiss;
	s->hashPawnStoreMiss += b->hashPawnStoreMiss;
	s->hashPawnStoreInPlace += b->hashPawnStoreInPlace;
	s->hashPawnStoreHits += b->hashPawnStoreHits;

	s->poswithmove += b->poswithmove;
	s->ebfnodes += b->ebfnodes;
	s->ebfnodespri += b->ebfnodespri;
	s->elaps += b->elaps;
	s->u_nullnodes += b->u_nullnodes;
	s->iterations += b->iterations;
	s->aspfailits += b->aspfailits;

	s->position_quality_tests += b->position_quality_tests;
	s->position_quality_cutoffs += b->position_quality_cutoffs;

	s->depth_sum += b->depth_sum;
	s->depth_max_sum += b->depth_max_sum;

}

// do prvniho parametru je skopirovan druhy
void CopySearchCnt(struct _statistics *s, struct _statistics *b)
{
	s->faillow = b->faillow;
	s->failhigh = b->failhigh;
	s->failnorm = b->failnorm;
	s->failhashlow = b->failhashlow;
	s->failhashhigh = b->failhashhigh;
	s->failhashnorm = b->failhashnorm;
	s->nodes = b->nodes;
	s->movestested = b->movestested;
	s->qmovestested = b->qmovestested;
	s->possiblemoves = b->possiblemoves;
	s->qpossiblemoves = b->qpossiblemoves;
	s->zerototal = b->zerototal;
	s->zerorerun = b->zerorerun;
	s->lmrtotal = b->lmrtotal;
	s->lmrrerun = b->lmrrerun;
	s->lmpcount = b->lmpcount;
	s->quiesceoverrun = b->quiesceoverrun;
	s->positionsvisited = b->positionsvisited;
	s->qposvisited = b->qposvisited;
	s->fhflcount = b->fhflcount;
	s->firstcutoffs = b->firstcutoffs;
	s->cutoffs = b->cutoffs;
	s->moves_to_cutoff = b->moves_to_cutoff;
	s->non_cutoff_moves = b->non_cutoff_moves;
	s->first_quiet_cuts = b->first_quiet_cuts;
	s->quiet_cuts = b->quiet_cuts;
	s->quiet_cuts_cap = b->quiet_cuts_cap;

	s->qfirstcutoffs = b->qfirstcutoffs;
	s->qcutoffs = b->qcutoffs;
	s->FUT_cuts = b->FUT_cuts;
	s->NMP_tries = b->NMP_tries;
	s->NMP_cuts = b->NMP_cuts;
	s->qSEE_tests = b->qSEE_tests;
	s->qSEE_cuts = b->qSEE_cuts;
	s->hashStores = b->hashStores;
	s->hashStoreColl = b->hashStoreColl;
	s->hashAttempts = b->hashAttempts;
	s->hashHits = b->hashHits;
	s->hashColls = b->hashColls;
	s->hashMiss = b->hashMiss;
	s->hashStoreMiss = b->hashStoreMiss;
	s->hashStoreInPlace = b->hashStoreInPlace;
	s->hashStoreHits = b->hashStoreHits;

	s->hashPawnStores = b->hashPawnStores;
	s->hashPawnStoreColl = b->hashPawnStoreColl;
	s->hashPawnAttempts = b->hashPawnAttempts;
	s->hashPawnHits = b->hashPawnHits;
	s->hashPawnColls = b->hashPawnColls;
	s->hashPawnMiss = b->hashPawnMiss;
	s->hashPawnStoreMiss = b->hashPawnStoreMiss;
	s->hashPawnStoreInPlace = b->hashPawnStoreInPlace;
	s->hashPawnStoreHits = b->hashPawnStoreHits;

	s->poswithmove = b->poswithmove;
	s->ebfnodes = b->ebfnodes;
	s->ebfnodespri = b->ebfnodespri;
	s->elaps = b->elaps;
	s->u_nullnodes = b->u_nullnodes;
	s->iterations = b->iterations;
	s->aspfailits = b->aspfailits;

	s->position_quality_tests = b->position_quality_tests;
	s->position_quality_cutoffs = b->position_quality_cutoffs;

#if 1
	s->depth = b->depth;
	s->depth_max = b->depth_max;
	s->depth_sum = b->depth_sum;
	s->depth_max_sum = b->depth_max_sum;
#endif
}

// od prvniho je odecten druhy a vlozen do tretiho
void DecSearchCnt(struct _statistics *s, struct _statistics *b, struct _statistics *r)
{
	r->faillow = s->faillow - b->faillow;
	r->failhigh = s->failhigh - b->failhigh;
	r->failnorm = s->failnorm - b->failnorm;
	r->failhashlow = s->failhashlow - b->failhashlow;
	r->failhashhigh = s->failhashhigh - b->failhashhigh;
	r->failhashnorm = s->failhashnorm - b->failhashnorm;
	r->nodes = s->nodes - b->nodes;
	r->movestested = s->movestested - b->movestested;
	r->qmovestested = s->qmovestested - b->qmovestested;
	r->possiblemoves = s->possiblemoves - b->possiblemoves;
	r->qpossiblemoves = s->qpossiblemoves - b->qpossiblemoves;
	r->zerototal = s->zerototal - b->zerototal;
	r->zerorerun = s->zerorerun - b->zerorerun;
	r->lmrtotal = s->lmrtotal - b->lmrtotal;
	r->lmrrerun = s->lmrrerun - b->lmrrerun;
	r->lmpcount = s->lmpcount - b->lmpcount;
	r->quiesceoverrun = s->quiesceoverrun - b->quiesceoverrun;
	r->positionsvisited = s->positionsvisited - b->positionsvisited;
	r->qposvisited = s->qposvisited - b->qposvisited;
	r->fhflcount = s->fhflcount - b->fhflcount;
	r->firstcutoffs = s->firstcutoffs - b->firstcutoffs;
	r->cutoffs = s->cutoffs - b->cutoffs;
	r->moves_to_cutoff = s->moves_to_cutoff - b->moves_to_cutoff;
	r->non_cutoff_moves = s->non_cutoff_moves - b->non_cutoff_moves;
	r->first_quiet_cuts = s->first_quiet_cuts - b->first_quiet_cuts;
	r->quiet_cuts = s->quiet_cuts - b->quiet_cuts;
	r->quiet_cuts_cap = s->quiet_cuts_cap - b->quiet_cuts_cap;
	r->qfirstcutoffs = s->qfirstcutoffs - b->qfirstcutoffs;
	r->qcutoffs = s->qcutoffs - b->qcutoffs;
	r->NMP_tries = s->NMP_tries - b->NMP_tries;
	r->NMP_cuts = s->NMP_cuts - b->NMP_cuts;
	r->FUT_cuts = s->FUT_cuts - b->FUT_cuts;

	r->qSEE_tests = s->qSEE_tests - b->qSEE_tests;
	r->qSEE_cuts = s->qSEE_cuts - b->qSEE_cuts;
	r->hashStores = s->hashStores - b->hashStores;
	r->hashStoreColl = s->hashStoreColl - b->hashStoreColl;
	r->hashAttempts = s->hashAttempts - b->hashAttempts;
	r->hashHits = s->hashHits - b->hashHits;
	r->hashColls = s->hashColls - b->hashColls;
	r->hashMiss = s->hashMiss - b->hashMiss;
	r->hashStoreMiss = s->hashStoreMiss - b->hashStoreMiss;
	r->hashStoreInPlace = s->hashStoreInPlace - b->hashStoreInPlace;
	r->hashStoreHits = s->hashStoreHits - b->hashStoreHits;

	r->hashPawnStores = s->hashPawnStores - b->hashPawnStores;
	r->hashPawnStoreColl = s->hashPawnStoreColl - b->hashPawnStoreColl;
	r->hashPawnAttempts = s->hashPawnAttempts - b->hashPawnAttempts;
	r->hashPawnHits = s->hashPawnHits - b->hashPawnHits;
	r->hashPawnColls = s->hashPawnColls - b->hashPawnColls;
	r->hashPawnMiss = s->hashPawnMiss - b->hashPawnMiss;
	r->hashPawnStoreMiss = s->hashPawnStoreMiss - b->hashPawnStoreMiss;
	r->hashPawnStoreInPlace = s->hashPawnStoreInPlace
		- b->hashPawnStoreInPlace;
	r->hashPawnStoreHits = s->hashPawnStoreHits - b->hashPawnStoreHits;

	r->poswithmove = s->poswithmove - b->poswithmove;
	r->ebfnodes = s->ebfnodes - b->ebfnodes;
	r->ebfnodespri = s->ebfnodespri - b->ebfnodespri;
	r->elaps = s->elaps - b->elaps;
	r->u_nullnodes = s->u_nullnodes - b->u_nullnodes;
	r->iterations = s->iterations - b->iterations;
	r->aspfailits = s->aspfailits - b->aspfailits;

	r->position_quality_tests = s->position_quality_tests
		- b->position_quality_tests;
	r->position_quality_cutoffs = s->position_quality_cutoffs
		- b->position_quality_cutoffs;

#if 0
	r->depth=s->depth-b->depth;
	r->depth_max=s->depth_max-b->depth_max;
#endif
	r->depth_sum = s->depth_sum - b->depth_sum;
	r->depth_max_sum = s->depth_max_sum - b->depth_max_sum;
}

void printSearchStat(struct _statistics *s)
{
	LOGGER_0(
		"Info: Low %lld, High %lld, Normal %lld, Positions %lld, MovesSearched %lld (%lld%%) of %lld TotalMovesAvail. Branching %f, %f\n",
		s->faillow, s->failhigh, s->failnorm, s->positionsvisited,
		s->movestested, (s->movestested * 100 / (s->possiblemoves + 1)),
		s->possiblemoves,
		((float )s->movestested / ((float )s->positionsvisited + 1)),
		((float )s->possiblemoves / ((float )s->positionsvisited + 1)));
	LOGGER_0(
		"Info: Positions with movegen %lld, last It EBF: %f, speed %f kNPS/s, nodes %lld\n",
		s->poswithmove,
		(float )s->ebfnodes / (float )(s->ebfnodespri + 1),
		(float ) (s->positionsvisited + s->qposvisited)
			/ (float )(s->elaps + 1), s->nodes);
	LOGGER_0("HASH: Get:%lld, GHit:%lld,%%%lld, GMiss:%lld, GCol: %lld\n",
		s->hashAttempts, s->hashHits,
		s->hashHits * 100 / (s->hashAttempts + 1), s->hashMiss,
		s->hashColls);
	LOGGER_0(
		"HASH: Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",
		s->hashStores, s->hashStoreHits, s->hashStoreInPlace,
		s->hashStoreMiss, s->hashColls);
	LOGGER_0("PHSH: Get:%lld, GHit:%lld,%%%lld, GMiss:%lld, GCol: %lld\n",
		s->hashPawnAttempts, s->hashPawnHits,
		s->hashPawnHits * 100 / (s->hashPawnAttempts + 1),
		s->hashPawnMiss, s->hashPawnColls);
	LOGGER_0(
		"PHSH: Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",
		s->hashPawnStores, s->hashPawnStoreHits,
		s->hashPawnStoreInPlace, s->hashPawnStoreMiss,
		s->hashPawnColls);
	LOGGER_0("HASH: TTLow %lld, TTHigh %lld, TTNormal %lld\n",
		s->failhashlow, s->failhashhigh, s->failhashnorm);
	LOGGER_0(
		"Info: QPositions %lld, QMovesSearched %lld,(%lld%%) of %lld QTotalMovesAvail\n",
		s->qposvisited, s->qmovestested,
		(s->qmovestested * 100 / (s->qpossiblemoves + 1)),
		s->qpossiblemoves);
	LOGGER_0(
		"Info: ZeroN %lld, ZeroRerun %lld, QZoverRun %lld, LmrN %lld, LmrRerun %lld, FhFlCount: %lld\n",
		s->zerototal, s->zerorerun, s->quiesceoverrun, s->lmrtotal,
		s->lmrrerun, s->fhflcount);
	LOGGER_0(
		"Info: LMP: count %lld\n", s->lmpcount);
	LOGGER_0(
		"Info: Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%\n",
		s->firstcutoffs, s->cutoffs,
		100 * s->firstcutoffs / (s->cutoffs + 1));
	LOGGER_0(
		"Info: Moves before Cuttoffs %lld, Average %lld, Non cutoff moves %lld\n", s->moves_to_cutoff,(s->moves_to_cutoff/(s->cutoffs+1)),
		s->non_cutoff_moves);

	LOGGER_0(
		"Info: Quiet Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%\n",
		s->first_quiet_cuts, s->quiet_cuts,
		100 * s->first_quiet_cuts / (s->quiet_cuts + 1));
	LOGGER_0(
		"Info: Quiet Cutoffs after capture move %lld\n", s->quiet_cuts_cap);
		
	LOGGER_0(
		"Info: Futility: cuts %lld\n", s->FUT_cuts);

	LOGGER_0(
		"Info: QCutoffs: First move %lld, Any move %lld, Ratio of first %lld%%\n",
		s->qfirstcutoffs, s->qcutoffs,
		100 * s->qfirstcutoffs / (s->qcutoffs + 1));
	LOGGER_0("Info: QuiesceSEE: Tests %lld, Cuts %lld, Ratio %lld%%\n",
		s->qSEE_tests, s->qSEE_cuts,
		100 * s->qSEE_cuts / (s->qSEE_tests + 1));
	LOGGER_0(
		"Info: NULL MOVE: Tries %lld, Cuts %lld, Ratio %lld%%, Nodes under NULL %lld\n",
		s->NMP_tries, s->NMP_cuts,
		100 * s->NMP_cuts / (s->NMP_tries + 1), s->u_nullnodes);
	LOGGER_0("Info: Aspiration: Iterations %lld, Failed It %lld\n",
		s->iterations, s->aspfailits);
	LOGGER_0("Info: Depth: Regular %d, Max %d, RegCum %lld, MaxCum %lld\n",
		s->depth, s->depth_max, s->depth_sum, s->depth_max_sum);
	LOGGER_0("Info: Time in: %dh, %dm, %ds, %dms\n",
		(int ) s->elaps / 3600000, (int ) (s->elaps % 3600000) / 60000,
		(int ) (s->elaps % 60000) / 1000, (int ) (s->elaps % 1000));
	LOGGER_0("Info: Position Quality Tests %lld, Reductions %lld\n",
		s->position_quality_tests, s->position_quality_cutoffs);
}

void printSearchStat2(struct _statistics *s, char *buff)
{
	char bb[2048];
	sprintf(buff,
		"Low %lld, High %lld, Normal %lld, Positions %lld, MovesSearched %lld (%lld%%) of %lld TotalMovesAvail. Branching %f, %f\n",
		s->faillow, s->failhigh, s->failnorm, s->positionsvisited,
		s->movestested, (s->movestested * 100 / (s->possiblemoves + 1)),
		s->possiblemoves,
		((float) s->movestested / ((float) s->positionsvisited + 1)),
		((float) s->possiblemoves / ((float) s->positionsvisited + 1)));
	strcat(buff, bb);
	sprintf(buff,
		"Positions with movegen %lld, last It EBF: %f, speed %f kNPS/s, nodes %lld\n",
		s->poswithmove,
		(float) s->ebfnodes / ((float) s->ebfnodespri + 1),
		(float) (s->positionsvisited + s->qposvisited)
			/ (float) (s->elaps + 1), s->nodes);
	strcat(buff, bb);
	sprintf(bb, "Get:%lld, GHit:%lld,%%%lld, GMiss:%lld, GCol: %lld\n",
		s->hashAttempts, s->hashHits,
		s->hashHits * 100 / (s->hashAttempts + 1), s->hashMiss,
		s->hashColls);
	strcat(buff, bb);
	sprintf(bb,
		"Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",
		s->hashStores, s->hashStoreHits, s->hashStoreInPlace,
		s->hashStoreMiss, s->hashColls);
	strcat(buff, bb);
	sprintf(bb, "Get:%lld, GPHit:%lld,%%%lld, GPMiss:%lld, GPCol: %lld\n",
		s->hashPawnAttempts, s->hashPawnHits,
		s->hashPawnHits * 100 / (s->hashPawnAttempts + 1),
		s->hashPawnMiss, s->hashPawnColls);
	strcat(buff, bb);
	sprintf(bb,
		"Stores:%lld, SPHit:%lld, SPInPlace:%lld, SPMiss:%lld SPCCol:%lld\n",
		s->hashPawnStores, s->hashPawnStoreHits,
		s->hashPawnStoreInPlace, s->hashPawnStoreMiss,
		s->hashPawnColls);
	strcat(buff, bb);
	sprintf(bb, "HASH: TTLow %lld, TTHigh %lld, TTNormal %lld\n",
		s->failhashlow, s->failhashhigh, s->failhashnorm);
	strcat(buff, bb);
	sprintf(bb,
		"QPositions %lld, QMovesSearched %lld,(%lld%%) of %lld QTotalMovesAvail\n",
		s->qposvisited, s->qmovestested,
		(s->qmovestested * 100 / (s->qpossiblemoves + 1)),
		s->qpossiblemoves);
	strcat(buff, bb);
	sprintf(bb,
		"ZeroN %lld, ZeroRerun %lld, QZoverRun %lld, LmrN %lld, LmrRerun %lld, FhFlCount: %lld\n",
		s->zerototal, s->zerorerun, s->quiesceoverrun, s->lmrtotal,
		s->lmrrerun, s->fhflcount);
	strcat(buff, bb);
	sprintf(bb,
		"Info: LMP: count %lld\n", s->lmpcount);
	strcat(buff, bb);
	sprintf(bb,
		"Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%, \n",
		s->firstcutoffs, s->cutoffs,
		100 * s->firstcutoffs / (s->cutoffs + 1));
	strcat(buff, bb);
	sprintf(bb,
		"Info: Moves before Cuttoffs %lld, Average %lld, Non cutoff moves %lld\n", s->moves_to_cutoff, (s->moves_to_cutoff/(s->cutoffs+1)),
		s->non_cutoff_moves);
	strcat(buff, bb);

	sprintf(bb, 
		"Info: Quiet Cutoffs: First move %lld, Any move %lld, Ratio of first %lld%%\n",
		s->first_quiet_cuts, s->quiet_cuts,
		100 * s->first_quiet_cuts / (s->quiet_cuts + 1));
	strcat(buff, bb);
	sprintf(bb,
		"Info: Quiet Cutoffs after capture move %lld\n", s->quiet_cuts_cap);
	strcat(buff, bb);

	sprintf(bb,
		"Info: Futility: cuts %lld\n", s->FUT_cuts);
	strcat(buff, bb);
	sprintf(bb,
		"QCutoffs: First move %lld, Any move %lld, Ratio of first %lld%%, \n",
		s->qfirstcutoffs, s->qcutoffs,
		100 * s->qfirstcutoffs / (s->qcutoffs + 1));
	strcat(buff, bb);
	sprintf(buff, "QuiesceSEE: Tests %lld, Cuts %lld, Ratio %lld%%, \n",
		s->qSEE_tests, s->qSEE_cuts,
		100 * s->qSEE_cuts / (s->qSEE_tests + 1));
	strcat(buff, bb);
	sprintf(bb,
		"NULL MOVE: Tries %lld, Cuts %lld, Ratio %lld%%, Nodes under NULL %lld\n",
		s->NMP_tries, s->NMP_cuts,
		100 * s->NMP_cuts / (s->NMP_tries + 1), s->u_nullnodes);
	strcat(buff, bb);
	sprintf(bb, "Info: Aspiration: Iterations %lld, Failed It %lld\n",
		s->iterations, s->aspfailits);
	strcat(buff, bb);
	sprintf(buff,
		"Info: Depth: Regular %d, Max %d, RegCum %lld, MaxCum %lld\n",
		s->depth, s->depth_max, s->depth_sum, s->depth_max_sum);
	strcat(buff, bb);
	sprintf(bb, "Info: Time in: %dh, %dm, %ds, %dms\n",
		(int) s->elaps / 3600000, (int) (s->elaps % 3600000) / 60000,
		(int) (s->elaps % 60000) / 1000, (int) (s->elaps % 1000));
	strcat(buff, bb);
	sprintf(bb, "Info: Position Quality Tests %lld, Reductions %lld\n",
		s->position_quality_tests, s->position_quality_cutoffs);
	strcat(buff, bb);
}

void clearALLSearchCnt(struct _statistics *s)
{
	int f;
	for (f = MAXPLY + 1; f >= 0; f--) {
		clearSearchCnt(&(s[f]));
	}
}

void printALLSearchCnt(struct _statistics *s)
{
	LOGGER_0("Stats: ** TOTALS **\n");
	printSearchStat(&(s[MAXPLY]));
	LOGGER_0("Stats: Finished\n");
}

struct _statistics* allocate_stats(int count)
{
	struct _statistics *s;
	s = malloc(sizeof(struct _statistics) * (unsigned int) count);
	return s;
}

void deallocate_stats(struct _statistics *s)
{
	free(s);
}
