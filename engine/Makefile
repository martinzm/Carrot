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

CC = gcc
LINK = gcc
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl -fopenmp -std=c99
CPPFLAGS = -I/usr/include/libxml2 -fopenmp

OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o tuner.o inlines.o
DEPS = bitmap.h defines.h attacks.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h tuner.h inlines.h

ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -Wall -O2 -DDEBUG1 -Finline-functions -Fwhole-program -Winline -pedantic -Funroll-loops -Wno-unused-variable
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), vnormal)
    CPPFLAGS += -g -Wall -O3 -DDEBUG1 -Finline-functions -Fwhole-program -Winline -Funroll-loops -Wno-unused-variable
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), sedebug)
    CPPFLAGS += -Wall -Og -DDEBUG1 -DSEDEBUG -Wno-unused-variable -Finline-functions -Winline -pedantic
    LDFLAGS += -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), release)
    CPPFLAGS += -Wall -Ofast -Wno-unused-variable -DNDEBUG -Finline-functions -Funroll-loops -Fwhole-program -Winline
    LDFLAGS += -DNDEBUG
endif
ifeq ($(MAKECMDGOALS), prof)
    CPPFLAGS += -g -pg -Og -DDEBUG1
    LDFLAGS += -pg -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), perf)
    CPPFLAGS += -g -Og -DDEBUG1
    LDFLAGS += -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), cov)
    CPPFLAGS += -pg --coverage -Og -DDEBUG1 -Wno-unused-variable -Finline-functions -Winline -Fwhole-program -pedantic -Funroll-loops
    LDFLAGS += -pg --coverage -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), covs)
    CPPFLAGS += -Og -DDEBUG1 -Wno-unused-variable -pedantic -ffunction-sections
    LDFLAGS += -DDEBUG1 -Wl,--gc-sections -Wl,--print-gc-sections
endif
ifeq ($(MAKECMDGOALS), debug)
    CPPFLAGS += -DDEBUG3 -Og -g -pedantic -Wall -Wconversion -c -Wno-unused-label -Wno-unused-variable
    LDFLAGS += -DDEBUG3 
endif
ifeq ($(MAKECMDGOALS), tune)
    CPPFLAGS += -DTUNING -DNDEBUG -O3 -Wall -Wno-unused-variable -Finline-functions -Funroll-loops -Fwhole-program  -Winline
    LDFLAGS += -DTUNING -DNDEBUG
endif
ifeq ($(MAKECMDGOALS), tuned)
    CPPFLAGS += -pg -g -DTUNING  -DDEBUG1 -Og -Wall -c -Wno-unused-variable -Winline -fprofile-arcs --coverage
    LDFLAGS += -pg -g -DTUNING -DDEBUG1  -fprofile-arcs --coverage
endif
ifeq ($(MAKECMDGOALS), sanit)
    CPPFLAGS += -DDEBUG3 -Og -pg -g3 -pedantic -Wall -Wconversion -c -MMD -MP -Wno-unused-label -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment -Wno-unused-variable -Finline-functions --coverage
    LDFLAGS += -DDEBUG3 -pg -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fno-sanitize=null -fno-sanitize=alignment --coverage
endif

all normal cov covs sedebug release prof debug sanit perf vnormal: index
	
tune tuned: index-t
	
clean:
	rm -rf *.o carrot core gmon.out carrot.exe *.gc?? *.d carrot-t
index: $(OBJS) pers_default.o
	$(LINK) -o carrot $(OBJS) $(LDFLAGS) pers_default.o
index-t: $(OBJS) pers_default.o
	$(LINK) -o carrot-t $(OBJS) $(LDFLAGS) pers_default.o
pers_default.o:
	objcopy -I binary -O elf64-x86-64 -B i386 pers_default.xml pers_default.o
$(OBJS): %.o: %.c
	$(CC) -c $< $(CPPFLAGS) -o $@

