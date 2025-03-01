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

#include "bitmap.h"
#include "generate.h"
#include "evaluate.h"
#include "attacks.h"
#include "movgen.h"
#include "search.h"
#include "tests.h"
#include "hash.h"
#include "pers.h"
#include "ui.h"
#include "utils.h"
#include "openings.h"
#include "globals.h"

#include <stdio.h>
#include <stdlib.h>

personality default_pers;

int main(int argc, char **argv)
{
	char logn[512];
	int second;

	int i;

	if (parse_cmd_line_check_sec(argc, argv)) {
		second = 1;
		generate_log_name(DEBUG_FILENAME, "_B_", logn);
	} else {
		second = 0;
		generate_log_name(DEBUG_FILENAME, "_A_", logn);
	}
	open_log(logn);
	LOGGER_3("INFO: Logging opened\n");
	LOGGER_0("Started as %s\n", argv[0]);
	LOGGER_0("%s v%s, REL %s, Features tested %s, %s %s\n", eNAME, eVERS,
		eREL, eFEATS, __DATE__, __TIME__);

	setvbuf(stdout, NULL, _IOLBF, 16384);
	setvbuf(stdin, NULL, _IOLBF, 1024);

//	generateRandomFile("rands.h");

	generate_rook(attack.maps[ROOK]);
	generate_bishop(attack.maps[BISHOP]);
	generate_knight(attack.maps[KNIGHT]);
	generate_king(attack.maps[KING]);
	generate_w_pawn_moves(attack.pawn_move[WHITE]);
	generate_w_pawn_moves2(attack.pawn_move2[WHITE]);
	generate_w_pawn_attack(attack.pawn_att[WHITE]);
	generate_b_pawn_moves(attack.pawn_move[BLACK]);
	generate_b_pawn_moves2(attack.pawn_move2[BLACK]);
	generate_b_pawn_attack(attack.pawn_att[BLACK]);

	generate_ep_mask(attack.ep_mask);
	
	init_nmarks();
	generate_rays(attack.rays, attack.rays_int);
	generate_rays_dir(attack.rays_dir);
	generate_attack_norm(attack.attack_norm, 0);
	generate_attack_r45L(attack.attack_r45L, 0);
	generate_attack_r45R(attack.attack_r45R, 0);
	generate_attack_r90R(attack.attack_r90R, 0);

	generate_attack_norm(attack.attack_norm_2, 1);
	generate_attack_r45L(attack.attack_r45L_2, 1);
	generate_attack_r45R(attack.attack_r45R_2, 1);
	generate_attack_r90R(attack.attack_r90R_2, 1);

	generate_w_passed_pawn_mask(attack.passed_p[WHITE]);
	generate_b_passed_pawn_mask(attack.passed_p[BLACK]);
	generate_w_back_pawn_mask(attack.back_span_p[WHITE]);
	generate_b_back_pawn_mask(attack.back_span_p[BLACK]);
	generate_file_mask(attack.file);
	generate_rank_mask(attack.rank);
	generate_iso_w_pawn_mask(attack.isolated_p);
	generate_color_map(attack.color_map);
	
	generate_topos(attack.ToPos);
	generate_directions(attack.dirs);
	generate_distance(attack.distance);
	generate_distance2(attack.distance2);
	generate_lefthalf(attack.lefthalf, attack);
	generate_righthalf(attack.righthalf, attack);
	generate_uphalf(attack.uphalf, attack);
	generate_downhalf(attack.downhalf, attack);
	generate_pawn_surr(attack.pawn_surr, attack);
	generate_king_surr1_bitmap(attack.surr1);
	generate_king_surr2_bitmap(attack.surr2);

	initRandom();

	LOGGER_3("INFO: Opening book\n");
	open_open("book.bin");

	printmask(1, "1");
	printmask(0-1, "-1");
	printmask(2, "2");
	printmask(0-2, "-2");
	printmask(3, "3");
	printmask(0-3, "-3");

	printmask(0x101010101010106, "0x0");
	printmask(0-0x101010101010106, "-0x0");
#ifdef NTUNING
	texel_test();
#else
	uci_loop(second);
#endif
	close_open();
	LOGGER_3("INFO: Book closed\n"); LOGGER_1("INFO: Finishing...\n");
	close_log();
	return 0;
}
