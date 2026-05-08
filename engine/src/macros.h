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

#undef MLINE

/* 
 xml parameter keyword, name of variable, type of parameter, default value
 */
/*
 * parameters are initiated as follows
 * general_option directy
 * gamestage options for stages 0 and 1
 * values as 6 values for stage 0 followed by 6 values for stage 1
 * passer as side, 8x values for stage0, 8x values for stage1, side, 8x values for stage0, 8x values for stage1,
 * if side==2 it is only one record and s_r tells whether to swap from white to black
 */

#define MLINE (x,y,z,s_r,i)

#define M_P(...) __VA_ARGS__

#define E_OPTS \
MLINE(simple_EVAL, simple_EVAL, _general_option, 0, 0)\
MLINE(NMP_allowed, NMP_allowed, _general_option, 0, 1)\
MLINE(NMP_reduction, NMP_reduction, _general_option, 0, 2)\
MLINE(NMP_min_depth, NMP_min_depth, _general_option, 0, 2)\
MLINE(NMP_div, NMP_div, _general_option, 0, 6)\
MLINE(NMP_search_reduction, NMP_search_reduction, _general_option, 0, 0)\
MLINE(quality_search_reduction, quality_search_reduction, _general_option, 0, 0)\
MLINE(LMR_reduction, LMR_reduction, _general_option, 0, 3)\
MLINE(LMR_start_move, LMR_start_move, _general_option, 0, 4)\
MLINE(LMR_remain_depth, LMR_remain_depth, _general_option, 0, 3)\
MLINE(LMR_prog_start_move, LMR_prog_start_move, _general_option, 0, 6)\
MLINE(LMR_prog_mod, LMR_prog_mod, _general_option, 0, 3)\
MLINE(LMR_sim, LMR_sim, _general_option, 0, 0)\
MLINE(LMP_enable, LMP_enable, _general_option, 0, 0)\
MLINE(LMP_start_move, LMP_start_move, _general_option, 0, 15)\
MLINE(LMP_depth, LMP_depth, _general_option, 0, 3)\
MLINE(IID_remain_depth, IID_remain_depth, _general_option, 0, 4)\
MLINE(quiesce_phase_limit, quiesce_phase_limit, _general_option, 0, 30)\
MLINE(quiesce_phase_bonus, quiesce_phase_bonus, _general_option, 0, 2000)\
MLINE(use_hash, use_hash, _general_option, 0, 1)\
MLINE(use_pawn_shelter, use_pawn_shelter, _general_option, 0, 0)\
MLINE(use_heavy_material, use_heavy_material, _general_option, 0, 0)\
MLINE(lazy_eval_cutoff, lazy_eval_cutoff, _general_option, 0, 3000)\
MLINE(use_ttable, use_ttable, _general_option, 0, 1)\
MLINE(use_ttable_prev, use_ttable_prev, _general_option, 0, 0)\
MLINE(ttable_clearing, ttable_clearing, _general_option, 0, 2)\
MLINE(use_killer, use_killer, _general_option, 0, 0)\
MLINE(use_aspiration, use_aspiration, _general_option, 0, 2500)\
MLINE(use_quiesce, use_quiesce, _general_option, 0, 1)\
MLINE(quiesce_check_depth_limit, quiesce_check_depth_limit, _general_option, 0, 1)\
MLINE(quiesce_depth_limit_multi, quiesce_depth_limit_multi, _general_option, 0, 3)\
MLINE(PVS_full_moves, PVS_full_moves, _general_option, 0, 2)\
MLINE(Quiesce_PVS_full_moves, Quiesce_PVS_full_moves, _general_option, 0, 1)\
MLINE(PVS_root_full_moves, PVS_root_full_moves, _general_option, 0, 1)\
MLINE(check_extension, check_extension, _general_option, 0, 1)\
MLINE(NEGAMAX, negamax, _general_option, 0, 1)\
MLINE(check_nodes_count, check_nodes_count, _general_option, 0, 3)\
MLINE(eval_BIAS, eval_BIAS, _general_option, 0, 0)\
MLINE(eval_BIAS_e, eval_BIAS_e, _general_option, 0, 0)\
MLINE(move_tempo, move_tempo, _gamestage, 0, M_P(100,0) ) \
MLINE(futility_depth, futility_depth, _general_option, 0, 2)\
MLINE(futility_cut, futility_cut, _general_option_8, 0, M_P(100,200,300,400,500,600,700,800))\
MLINE(futility_sim, futility_sim, _general_option, 0, 0)\
MLINE(mobility_protect, mobility_protect, _general_option, 0, 1)\
MLINE(mobility_unsafe, mobility_unsafe, _general_option, 0, 0)\
MLINE(bishopboth, bishopboth, _gamestage, 0, M_P(500,500) ) \
MLINE(knightpair, knightpair, _gamestage, 0, M_P(0,0) ) \
MLINE(rookpair, rookpair, _gamestage, 0, M_P(0,0) ) \
MLINE(rook_on_seventh, rook_on_seventh, _gamestage, 0, M_P(300, 0) )\
MLINE(rook_on_open, rook_on_open, _gamestage, 0, M_P(150, 50) )\
MLINE(rook_on_semiopen, rook_on_semiopen, _gamestage, 0, M_P(150, 150) )\
MLINE(rook_to_pawn, rook_to_pawn, _gamestage, 0, M_P(125,125) ) \
MLINE(pawn_ah_penalty, pawn_ah_penalty, _gamestage, 0, M_P(-150,-150) )\
MLINE(isolated_penalty, isolated_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(pawn_iso_center_penalty, pawn_iso_center_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(pawn_iso_onopen_penalty, pawn_iso_onopen_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(pawn_weak_onopen_penalty, pawn_weak_onopen_penalty, _gamestage, 0, M_P(0,0) ) \
MLINE(pawn_weak_center_penalty, pawn_weak_center_penalty, _gamestage, 0, M_P(0,0) ) \
MLINE(king_castle_pot_bonus, king_castle_pot_bonus, _gamestage, 0, M_P(0,0) ) \
MLINE(king_moved_away_bonus, king_moved_away_bonus, _gamestage, 0, M_P(0,0) ) \
MLINE(passer_bonus, passer_bonus, _passer, 0, M_P(2, 0,1000,500,250,200,150,100,0, 0,5000,3500,2750,2000,1500,1000,0) )\
MLINE(pot_passer_bonus, pot_passer_bonus, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(passer_my_king_bonus, passer_my_king_bonus, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(passer_op_king_penalty, passer_op_king_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pawn_op_king_bonus, pawn_op_king_bonus, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(passer_unstop_bonus, passer_unstop_bonus, _gamestage, 0, M_P(0,0) )\
MLINE(backward_penalty, backward_penalty, _gamestage, 0, M_P(-250,-250) ) \
MLINE(doubled_n_penalty, doubled_n_penalty, _passer, 0, M_P(2, -1000,-1000,-500,-250,-200,-150,-100,0, -10000,-1000,-1000,-10000,-1000,-1000,-1000,0) )\
MLINE(pawn_stopped_penalty, pawn_stopped_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pawn_blocked_penalty, pawn_blocked_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pawn_issues_penalty, pawn_issues_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pawn_pot_protect, pawn_pot_protect, _passer, 0, M_P(2, 1111,1000,500,250,200,150,100,0, 0,5000,3500,2750,2000,1500,1000,0) )\
MLINE(pawn_protect_count, pawn_protect_count, _passer, 0, M_P(2,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0) ) \
MLINE(pawn_prot_over_penalty, pawn_prot_over_penalty, _passer, 0, M_P(2,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0) ) \
MLINE(pawn_n_protect, pawn_n_protect, _passer, 0, M_P(2,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0) ) \
MLINE(pawn_dir_protect, pawn_dir_protect, _passer, 0, M_P(2,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0) ) \
MLINE(pshelter_dir_protect, pshelter_dir_protect, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) ) \
MLINE(pshelter_blocked_penalty, pshelter_blocked_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pshelter_stopped_penalty, pshelter_stopped_penalty, _passer, 0, M_P(2, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0) )\
MLINE(pshelter_out_penalty, pshelter_out_penalty, _gamestage, 0, M_P(-150, 0) )\
MLINE(pshelter_open_penalty, pshelter_open_penalty, _gamestage, 0, M_P(-100,-50) ) \
MLINE(pshelter_isol_penalty, pshelter_isol_penalty, _gamestage, 0, M_P(-100,-50) ) \
MLINE(pshelter_hopen_penalty, pshelter_hopen_penalty, _gamestage, 0, M_P(-100,-50) ) \
MLINE(pshelter_double_penalty, pshelter_double_penalty, _gamestage, 0, M_P(-100,-50) ) \
MLINE(pshelter_prim_bonus, pshelter_prim_bonus, _gamestage, 0, M_P(100,50) ) \
MLINE(pshelter_sec_bonus, pshelter_sec_bonus, _gamestage, 0, M_P(50,25) ) \
MLINE(dvalues, dvalues, _dvalues, 0, M_P(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
										 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
										 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0))\
MLINE(material, Values, _values, 0, M_P(1000,3250,3250,5000,9750,0,1000,3250,3250,5000,9750,88888))

/*
 * chybi
 * pst
 * mobility
 * 
 */
