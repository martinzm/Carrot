###########################################################################
#                                                                         #
#  Carrot is a UCI chess playing engine by Martin Žampach.                #
#  <https://github.com/martinzm/Carrot>     <martinzm@centrum.cz>         #
#                                                                         #
#  Carrot is free software: you can redistribute it and/or modify         #
#  it under the terms of the GNU General Public License as published by   #
#  the Free Software Foundation, either version 3 of the License, or      #
#  (at your option) any later version.                                    #
#                                                                         #
#  Carrot is distributed in the hope that it will be useful,              #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of         #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          #
#  GNU General Public License for more details.                           #
#                                                                         #
#  You should have received a copy of the GNU General Public License      #
#  along with this program.  If not, see <https://www.gnu.org/licenses/>  #
#                                                                         #
###########################################################################

NAME = carrot
MKDIR = mkdir -p

PATHS = src/
PATHO = build/objs/
PATHD = build/depends/
PATHU = unity/src/
PATHB = build/

BUILD_PATHS = $(PATHB) $(PATHO) $(PATHD)
SRCT = $(wildcard $(PATHS) *.c)

OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o tuner.o inlines.o
DEPS = bitmap.h defines.h attacks.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h tuner.h inlines.h

COMPILE = gcc
LINK = gcc
DEPEND = gcc -MM -MG -MF
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl -fopenmp -std=c99 -I/usr/lib64
CFLAGS = -I. -I$(PATHS) -I/usr/include/libxml2 -fopenmp -DTEST

ifeq ($(MAKECMDGOALS), normal)
    CFLAGS += -Wall -O2 -DDEBUG1 -Finline-functions -Fwhole-program -Winline -pedantic -Funroll-loops -Wno-unused-variable
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), vnormal)
    CFLAGS += -g -Wall -O2 -DDEBUG1 -Finline-functions -Fwhole-program -Winline -Funroll-loops -Wno-unused-variable
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), sedebug)
    CFLAGS += -Wall -Og -DDEBUG1 -DSEDEBUG2 -Wno-unused-variable -Finline-functions -Winline -pedantic
    LDFLAGS += -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), release)
    CFLAGS += -Wall -Ofast -Wno-unused-variable -DNDEBUG -Finline-functions -Funroll-loops -Fwhole-program -Winline
    LDFLAGS += -DNDEBUG
endif
ifeq ($(MAKECMDGOALS), prof)
    CFLAGS += -g -pg -Og -DDEBUG1
    LDFLAGS += -pg -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), perf)
    CFLAGS += -g -O2 -DDEBUG1
    LDFLAGS += -pg -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), cov)
    CFLAGS += -pg --coverage -Og -DDEBUG1 -Wno-unused-variable -Finline-functions -Winline -Fwhole-program -pedantic -Funroll-loops
    LDFLAGS += -pg --coverage -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), covs)
    CFLAGS += -Og -DDEBUG1 -Wno-unused-variable -pedantic -ffunction-sections
    LDFLAGS += -DDEBUG1 -Wl,--gc-sections -Wl,--print-gc-sections
endif
ifeq ($(MAKECMDGOALS), debug)
    CFLAGS += -DDEBUG3 -Og -g -pedantic -Wall -Wconversion -c -Wno-unused-label -Wno-unused-variable
    LDFLAGS += -DDEBUG3 
endif
ifeq ($(MAKECMDGOALS), tune)
    CFLAGS += -DTUNING -DNDEBUG -O3 -Wall -Wno-unused-variable -Finline-functions -Funroll-loops -Fwhole-program  -Winline
    LDFLAGS += -DTUNING -DNDEBUG
endif
ifeq ($(MAKECMDGOALS), tuned)
    CFLAGS += -pg -g -DTUNING  -DDEBUG1 -Og -Wall -c -Wno-unused-variable -Winline -fprofile-arcs --coverage
    LDFLAGS += -pg -g -DTUNING -DDEBUG1  -fprofile-arcs --coverage
endif
ifeq ($(MAKECMDGOALS), sanit)
    CFLAGS += -DDEBUG3 -Og -pg -g3 -pedantic -Wall -Wconversion -c -MMD -MP -Wno-unused-label -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment -Wno-unused-variable -Finline-functions --coverage
    LDFLAGS += -DDEBUG3 -pg -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment --coverage
endif

all normal cov covs sedebug release prof debug sanit perf vnormal: $(PATHB)$(NAME)
	
tune tuned: $(PATHB)$(NAME)-t
	
clean:
	rm -rf $(PATHO)*.o
	rm -rf $(PATHB)carrot core gmon.out carrot.exe *.gc?? *.d carrot-t

$(PATHB)$(NAME): $(PATHO)pers_default.o $(addprefix $(PATHO),$(OBJS)) | $(PATHO)
	$(LINK) -o $@ $(LDFLAGS) $^

$(PATHB)$(NAME)-t: $(PATHO)pers_default.o $(addprefix $(PATHO),$(OBJS))  | $(PATHO)
	$(LINK) -o $@ $(LDFLAGS) $^

$(PATHO)pers_default.o:
	objcopy -I binary -O elf64-x86-64 -B i386 $(PATHS)pers_default.xml $@

$(PATHO)%.o:: $(PATHS)%.c
	$(COMPILE) -c $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHU)%.c
	$(COMPILE) -c $(CFLAGS) $< -o $@

$(PATHB):
	$(MKDIR) $(PATHB)
$(PATHD):
	$(MKDIR) $(PATHD)
$(PATHO):
	$(MKDIR) $(PATHO)

.PHONY: clean
.PHONY: Makefile
.PHONY: normal
