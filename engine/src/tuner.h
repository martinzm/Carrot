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

#ifndef TUNER_H
#define TUNER_H
#include "bitmap.h"

typedef struct {
	personality *p;
	int stage;
} tuner_variables_pass;

void replay_stacker(stacker *, pers_uni *, pers_uni *);
int eval_dir_stacker(board *, attack_model *, personality *, stacker *);

void texel_test(char *);
#endif
