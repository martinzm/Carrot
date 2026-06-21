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

#ifndef DEFINES_H
#define DEFINES_H

typedef enum _STATS_COLL {
	S_failnorm=0,
	S_faillow,
	S_failhigh,
	S_Qfaillow,
	S_Qfailhigh,
	S_Qfailnorm,
	S_failhashnorm,
	S_failhashlow,
	S_failhashhigh,
	S_Qfailhashnorm,
	S_Qfailhashlow,
	S_Qfailhashhigh,
	S_nodes,
	S_positionsvisited,
	S_PV_positions,
	S_movestested,
	S_possiblemoves,
	S_zerototal,
	S_zerorerun,
	S_zerorerunnodes,
	S_quiesceoverrun,
	S_qposvisited,
	S_qmovestested,
	S_qpossiblemoves,
	S_lmrtotal,
	S_lmrrerun,
	S_lmrrerunnodes,
	S_lmpcount,
	S_fhflcount,
	S_Zfhflcount,
	S_firstcutoffs,
	S_cutoffs,
	S_first_cap_cuts,
//	S_moves_to_cutoff,
	S_non_cutoff_moves,
	S_first_quiet_cuts,
	S_quiet_cuts,
	S_quiet_cuts_cap,
	S_cutoff_cum,
	S_cutoff_long,
	S_qfirstcutoffs,
	S_qcutoffs,
	S_FUT_cuts,
	S_NMP_cuts,
	S_NMP_tries,
	S_qSEE_tests,
	S_qSEE_cuts,
	S_poswithmove,
	S_ebfnodes,
	S_ebfnodespri,
	S_elaps,
	S_u_nullnodes,
	S_iterations,
	S_ITsearch,
	S_aspfailits,
	S_hashStores,
	S_hashStoreColl,
	S_hashAttempts,
	S_hashHits,
	S_hashColls,
	S_hashMiss,
	S_hashStoreMiss,
	S_hashStoreInPlace,
	S_hashStoreHits,
	S_hashPawnStores,
	S_hashPawnStoreColl,
	S_hashPawnAttempts,
	S_hashPawnHits,
	S_hashPawnColls,
	S_hashPawnMiss,
	S_hashPawnStoreMiss,
	S_hashPawnStoreInPlace,
	S_hashPawnStoreHits,
	S_position_quality_tests,
	S_position_quality_cutoffs,
	S_depth,
	S_depth_max,
	S_depth_sum,
	S_depth_max_sum,
	S_wasted_time,
	S_MAX_COLL
} STATS_COLL;

//#define DEBUG_FILENAME "./"
#define DEBUG_FILENAME "../logs/debug"

#define LOGGER_0(...) blogger2f(1, __VA_ARGS__)
#define NLOGGER_0(...) blogger2f(0, __VA_ARGS__)

#define LX(out, off, len, ...) { off += snprintf(out+off, Max(0, len-off), __VA_ARGS__); }

#define L0 LOGGER_0 
#define L1 LOGGER_1
#define L2 LOGGER_2 
#define L3 LOGGER_3 
#define L4 LOGGER_4 
#define LS2 LOGGER_S2

#define NL0 NLOGGER_0 
#define NL1 NLOGGER_1
#define NL2 NLOGGER_2 
#define NL3 NLOGGER_3 
#define NL4 NLOGGER_4 
#define NLS2 NLOGGER_S2



#define DEB_0(x) x
#define DEB_X(x)

#define LOGGER LOGGER_0

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG1) || defined (DEBUG4)
	#define LOGGER_1(...) blogger2f(1, __VA_ARGS__)
	#define NLOGGER_1(...) blogger2f(0, __VA_ARGS__)
#else
#define LOGGER_1(...)
#define NLOGGER_1(...)
#endif

#if defined (DEBUG2) || defined (DEBUG3) || defined (DEBUG4)
	#define LOGGER_2(...) blogger2f(1, __VA_ARGS__)
	#define NLOGGER_2(...) blogger2f(0, __VA_ARGS__)
#else
#define LOGGER_2(...)
#define NLOGGER_2(...)
#endif

#if defined (DEBUG3) || defined (DEBUG4)
	#define LOGGER_3(...) blogger2f(1, __VA_ARGS__)
	#define NLOGGER_3(...) blogger2f(0, __VA_ARGS__)
#else
#define LOGGER_3(...)
#define NLOGGER_3(...)
#endif

#if defined (DEBUG4)
	#define LOGGER_4(...) blogger2f(1, __VA_ARGS__)
	#define NLOGGER_4(...) blogger2f(1, __VA_ARGS__)
#else
#define LOGGER_4(...)
#define NLOGGER_4(...)
#endif

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG1) || defined (DEBUG4)
	#define DEB_1(x) x
#else
#define DEB_1(x)
#endif

#if defined (DEBUG3) || defined (DEBUG2) || defined (DEBUG4)
	#define DEB_2(x) x
#else
#define DEB_2(x)
#endif

#if defined (DEBUG3) || defined (DEBUG4)
	#define DEB_3(x) x
#else
#define DEB_3(x)
#endif

#if defined (DEBUG4)
	#define DEB_4(x) x
#else
#define DEB_4(x)
#endif

#if defined (SEDEBUG)
	#define DEB_SE(x) x
	#define LOGGER_SE(...) blogger2f(1, __VA_ARGS__)
	#define NLOGGER_SE(...) blogger2f(0, __VA_ARGS__)
#else
#define DEB_SE(x)
#define LOGGER_SE(...)
#define NLOGGER_SE(...)
#endif

#if defined (SEDEBUG2)
	#define DEB_S2(x) x
	#define LOGGER_S2(...) blogger2f(1, 0__VA_ARGS__)
	#define NLOGGER_S2(...) blogger2f(0, __VA_ARGS__)
#else
#define DEB_S2(x)
#define LOGGER_S2(...)
#define NLOGGER_S2(...)
#endif



#endif
