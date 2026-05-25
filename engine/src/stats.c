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
#include "defines.h"
#include "globals.h"

void clearSearchCnt(struct _statistics *s)
{
	for(int f=0;f<S_MAX_COLL; f++) { s->s[f]=0; }
}

// do prvniho parametru je pricten druhy
void AddSearchCnt(struct _statistics *s, struct _statistics *b)
{
long long depth, depth_max;

	depth = s->s[S_depth];
	depth_max = s->s[S_depth_max];
	for(int f=0;f<S_MAX_COLL; f++) { s->s[f] += b->s[f]; }

// vyresit depth a depth_max
	s->s[S_depth] = depth;
	s->s[S_depth_max] = depth_max;

}

// do prvniho parametru je skopirovan druhy
void CopySearchCnt(struct _statistics *s, struct _statistics *b)
{
	for(int f=0;f<S_MAX_COLL; f++) { s->s[f] = b->s[f]; }
}

// od prvniho je odecten druhy a vlozen do tretiho
void DecSearchCnt(struct _statistics *s, struct _statistics *b, struct _statistics *r)
{
	for(int f=0;f<S_MAX_COLL; f++) { r->s[f] = s->s[f] - b->s[f]; }
}

void dumpEBF()
{
	int f, mx=Min(MAXPLY, 64);
	for(f=1;f<mx;f++) {
		L0("Depth %d, N1/N-1 %lld/%lld, ebf %.2f\n",
		f, STATS[f].s[S_ebfnodes], STATS[f].s[S_ebfnodespri], STATS[f].s[S_ebfnodes]/(STATS[f].s[S_ebfnodespri]+1.0));
	}
}

// posklada vystup do bufferu o, ktery ma delku l
void printSearchStat3(struct _statistics *s, char *o, int l)
{
int c=0;

	LX(o,c,l,
		"Info: Positions visited %lld, PV %lld (%.2f%%), nonPV %lld, with movgen %lld\n"
		"Info: Resolutions Exact %lld (%.2f%%), High %lld (%.2f%%), Low %lld (%.2f%%), TTExact %lld (%.2f%%), TTHigh %lld (%.2f%%), TTLow %lld (%.2f%%)\n"
		"Info: NMP Tries %lld / Cuts %lld (%.2f%%), RFP cuts %lld (%.2f%%)\n",
		s->s[S_positionsvisited], s->s[S_PV_positions], 100*s->s[S_PV_positions]/(s->s[S_positionsvisited]+1.0), s->s[S_positionsvisited] - s->s[S_PV_positions],
		s->s[S_poswithmove],
		s->s[S_failnorm], 100*s->s[S_failnorm]/(s->s[S_positionsvisited]+1.0), s->s[S_failhigh], 100*s->s[S_failhigh]/(s->s[S_positionsvisited]+1.0),
		s->s[S_faillow], 100*s->s[S_faillow]/(s->s[S_positionsvisited]+1.0),
		s->s[S_failhashnorm], 100*s->s[S_failhashnorm]/(s->s[S_failnorm]+1.0),
		s->s[S_failhashhigh], 100*s->s[S_failhashhigh]/(s->s[S_failhigh]+1.0), 
		s->s[S_failhashlow], 100*s->s[S_failhashlow]/(s->s[S_faillow]+1.0), 
		
		s->s[S_NMP_tries], s->s[S_NMP_cuts], 100 * s->s[S_NMP_cuts] / (s->s[S_NMP_tries] + 1.0),
		s->s[S_FUT_cuts], 100*s->s[S_FUT_cuts]/(s->s[S_positionsvisited]+1.0));
// PVS moves???
	LX(o,c,l,
		"Info: Moves tested %lld (%.2f%%), generated %lld, PVS %lld, PVS in %.2f moves\n", 
		s->s[S_movestested], 100*s->s[S_movestested]/(s->s[S_possiblemoves] + 1.0),s->s[S_possiblemoves],
		s->s[S_movestested]-s->s[S_zerototal], ((s->s[S_movestested]+1.0)/(s->s[S_movestested]-s->s[S_zerototal])));
	LX(o,c,l,
			"Info: LmrN %lld, LmrRerun %lld (%.2f%%), LMP cuts %lld, FhFlCount: %lld\n",
		s->s[S_lmrtotal], s->s[S_lmrrerun], 100*s->s[S_lmrrerun]/(s->s[S_lmrtotal]+1.0), s->s[S_lmpcount], s->s[S_fhflcount]);
	LX(o,c,l,
			"Info: ZeroN %lld, ZeroRerun %lld, Zero Rate %.2f%%\n",
		s->s[S_zerototal], s->s[S_zerorerun],  100*s->s[S_zerorerun]/(s->s[S_zerototal]+1.0));
	LX(o,c,l,
	"HASH: TTHits %lld, PosRes %lld (%.2f%%), Move Ordering %lld (%.2f%%)\n",
		s->s[S_hashHits], s->s[S_failhashnorm]+s->s[S_failhashhigh]+s->s[S_failhashlow], 
		100 *(s->s[S_failhashnorm]+s->s[S_failhashhigh]+s->s[S_failhashlow])/(s->s[S_hashHits]+1.0),
		(s->s[S_hashHits]-s->s[S_failhashnorm]-s->s[S_failhashhigh]-s->s[S_failhashlow]),
		100 *(s->s[S_hashHits]-s->s[S_failhashnorm]-s->s[S_failhashhigh]-s->s[S_failhashlow])/(s->s[S_hashHits]+1.0));

	LX(o,c,l,
	"Info: NMP run node %lld, ZeroRerunMoves %lld, LmrRerunMoves %lld\n", s->s[S_u_nullnodes], s->s[S_zerorerunnodes], s->s[S_lmrrerunnodes]);
	LX(o,c,l,
			"Info: Cutoffs: First move %lld, Any move %lld, Ratio of first %.2f%%\n",
		s->s[S_firstcutoffs], s->s[S_cutoffs], 100 * s->s[S_firstcutoffs] / (s->s[S_cutoffs] + 1.0));
	LX(o,c,l,
			"Info: Moves before Cuttoffs %lld, Average %.2f%%, Non cutoff moves %lld\n", s->s[S_moves_to_cutoff],100*(s->s[S_moves_to_cutoff]/(s->s[S_cutoffs]+1.0)),
		s->s[S_non_cutoff_moves]);
	LX(o,c,l,
			"Info: Quiet Cutoffs: First move %lld, Any move %lld, Ratio of first %.2f%%\n",
		s->s[S_first_quiet_cuts], s->s[S_quiet_cuts],
		100 * s->s[S_first_quiet_cuts] / (s->s[S_quiet_cuts] + 1.0));
	LX(o,c,l,
			"Info: Quiet Cutoffs after capture move %lld\n", s->s[S_quiet_cuts_cap]);
#if 0
	LX(o,c,l,
		"Info: Positions with movegen %lld, last It EBF: %f, speed %f kNPS/s, nodes %lld\n",
		s->s[S_poswithmove],
		(float )s->s[S_ebfnodes] / (float )(s->s[S_ebfnodespri] + 1),
		(float ) (s->s[S_positionsvisited] + s->s[S_qposvisited])
			/ (float )(s->s[S_elaps] + 1), s->s[S_nodes]);
#endif
	LX(o,c,l,
	"HASH: Get:%lld, GHit:%lld (%.2f%%), GMiss:%lld, GCol: %lld\n",
		s->s[S_hashAttempts], s->s[S_hashHits],
		s->s[S_hashHits] * 100 / (s->s[S_hashAttempts] + 1.0), s->s[S_hashMiss], s->s[S_hashColls]);
	LX(o,c,l,
			"HASH: Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",
		s->s[S_hashStores], s->s[S_hashStoreHits], s->s[S_hashStoreInPlace],
		s->s[S_hashStoreMiss], s->s[S_hashStoreColl]);
	LX(o,c,l,
	"PHSH: Get:%lld, GHit:%lld (%.2f%%), GMiss:%lld, GCol: %lld\n",
		s->s[S_hashPawnAttempts], s->s[S_hashPawnHits],
		s->s[S_hashPawnHits] * 100 / (s->s[S_hashPawnAttempts] + 1.0),
		s->s[S_hashPawnMiss], s->s[S_hashPawnColls]);
	LX(o,c,l,
			"PHSH: Stores:%lld, SHit:%lld, SInPlace:%lld, SMiss:%lld SCCol:%lld\n",
		s->s[S_hashPawnStores], s->s[S_hashPawnStoreHits],
		s->s[S_hashPawnStoreInPlace], s->s[S_hashPawnStoreMiss],
		s->s[S_hashPawnStoreColl]);
	LX(o,c,l,
	"HASH: TTNormal %lld, TTHigh %lld,TTLow %lld\n",
		s->s[S_failhashnorm], s->s[S_failhashhigh], s->s[S_failhashlow]);
	LX(o,c,l,
			"Info: QPositions %lld, QMovesSearched %lld (%.2f%%) of %lld QTotalMovesAvail\n",
		s->s[S_qposvisited], s->s[S_qmovestested],
		s->s[S_qmovestested] * 100 / (s->s[S_qpossiblemoves] + 1.0), s->s[S_qpossiblemoves]);
	LX(o,c,l,
			"Info: QCutoffs: First move %lld, Any move %lld, Ratio of first %.2f%%\n",
		s->s[S_qfirstcutoffs], s->s[S_qcutoffs],
		100 * s->s[S_qfirstcutoffs] / (s->s[S_qcutoffs] + 1.0));
	LX(o,c,l,
	"Info: QuiesceSEE: Tests %lld, Cuts %lld, Ratio %.2f%%\n",
		s->s[S_qSEE_tests], s->s[S_qSEE_cuts],
		100 * s->s[S_qSEE_cuts] / (s->s[S_qSEE_tests] + 1.0));
	LX(o,c,l,
	"Info: Aspiration: Iterations %lld, Failed It %lld\n",
		s->s[S_iterations], s->s[S_aspfailits]);
	LX(o,c,l,
	"Info: Search runs %lld, Average Depth %.2f, Average SelDepth %.2f\n",
		s->s[S_ITsearch], s->s[S_depth_sum]/(s->s[S_ITsearch]+0.0), s->s[S_depth_max_sum]/(s->s[S_ITsearch]+0.0));
	LX(o,c,l,
	"Info: Time in: %dh, %dm, %ds, %dms\n",
		(int ) s->s[S_elaps] / 3600000, (int ) (s->s[S_elaps] % 3600000) / 60000,
		(int ) (s->s[S_elaps] % 60000) / 1000, (int ) (s->s[S_elaps] % 1000));
#if 0
	LX(o,c,l,
	"Info: Position Quality Tests %lld, Reductions %lld\n",
		s->s[S_position_quality_tests], s->s[S_position_quality_cutoffs]);
#endif
}

// generate all into buffer
// print buffer into file
void printSearchStat(struct _statistics *s)
{
char buf[5120];
	printSearchStat3(s, buf, sizeof(buf));
	blogger2b(1, buf);
	dumpEBF();
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
