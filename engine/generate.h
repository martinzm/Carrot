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

#ifndef GENERATE_H
#define GENERATE_H

#include "bitmap.h"

void generate_rook(BITVAR norm[]);
void generate_bishop(BITVAR norm[]);
void generate_knight(BITVAR norm[]);
void generate_king(BITVAR norm[]);
void generate_w_pawn_moves(BITVAR norm[]);
void generate_w_pawn_moves2(BITVAR norm[]);
void generate_w_pawn_attack(BITVAR norm[]);
void generate_b_pawn_moves(BITVAR norm[]);
void generate_b_pawn_moves2(BITVAR norm[]);
void generate_b_pawn_attack(BITVAR norm[]);
void generate_ep_mask(BITVAR norm[]);

void generate_file_mask(BITVAR map[64]);
void generate_rank_mask(BITVAR map[64]);
void generate_iso_w_pawn_mask(BITVAR map[64]);

void generate_rays(BITVAR[64][64], BITVAR[64][64]);
void generate_rays_dir(BITVAR[64][64]);
void generate_topos(int*);
void generate_directions(BITVAR[64][8]);

void generate_w_passed_pawn_mask(BITVAR map[64]);
void generate_b_passed_pawn_mask(BITVAR map[64]);
void generate_w_back_pawn_mask(BITVAR map[64]);
void generate_b_back_pawn_mask(BITVAR map[64]);

void generate_color_map(int map[64]);
void generate_distance(int map[64][64]);
void generate_distance2(int map[64][64]);
void generate_uphalf(BITVAR map[64], att_mov att);
void generate_downhalf(BITVAR map[64], att_mov att);
void generate_lefthalf(BITVAR map[64], att_mov att);
void generate_righthalf(BITVAR map[64], att_mov att);
void generate_pawn_surr(BITVAR map[64], att_mov att);
void generate_king_surr1_bitmap(BITVAR ksur[64]);
void generate_king_surr2_bitmap(BITVAR ksur[64]);

void generate_attack_r45R(BITVAR map[64][256], int skip);
void generate_attack_r45L(BITVAR map[64][256], int skip);
void generate_attack_r90R(BITVAR map[64][256], int skip);
void generate_attack_norm(BITVAR map[64][256], int skip);
void setup_normal_board(board *b);
void setup_FEN_board(board *b, char *fen);
void setup_FEN_board_fast(board *b, char *fen);
void writeEPD_FEN(board const *b, char *fen, int epd, char *option);

int collect_material_from_board(board const *b, int *pw, int *pb, int *nw, int *nb, int *bwl, int *bwd, int *bbl, int *bbd, int *rw, int *rb, int *qw, int *qb);

void empty_board(board *b);

#endif
