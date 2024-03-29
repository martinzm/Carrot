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
LDFLAGS = -lxml2 -lpthread -lz -lm -ldl -fopenmp
CPPFLAGS = -I/usr/include/libxml2 -fopenmp

OBJS = attacks.o bitmap.o evaluate.o generate.o globals.o hash.o main.o movgen.o openings.o pers.o search.o stats.o tests.o ui.o utils.o tuner.o
DEPS = bitmap.h defines.h attacks.h evaluate.h generate.h globals.h hash.h movgen.h openings.h randoms.h pers.h search.h ui.h utils.h tests.h tuner.h

ifeq ($(MAKECMDGOALS), normal)
    CPPFLAGS += -Wall -O1 -DDEBUG1 -Finline-functions -Fwhole-program -Winline -pedantic
    LDFLAGS += -DDEBUG1 
endif
ifeq ($(MAKECMDGOALS), sedebug)
    CPPFLAGS += -Wall -O1 -DDEBUG1 -DSEDEBUG -Wno-unused-variable -Finline-functions -Winline -pedantic
    LDFLAGS += -DDEBUG1
endif
ifeq ($(MAKECMDGOALS), release)
    CPPFLAGS += -Wall -O3 -Wno-unused-variable -DNDEBUG -Finline-functions -Funroll-loops -Fwhole-program -Winline
    LDFLAGS += -DNDEBUG
endif
ifeq ($(MAKECMDGOALS), prof)
    CPPFLAGS += -pg -g -DNDEBUG -Finline-functions -Winline -Og -fprofile-arcs --coverage -Fwhole-program -Funroll-loops
    LDFLAGS += -pg -g -DNDEBUG -fprofile-arcs --coverage
endif
ifeq ($(MAKECMDGOALS), cov)
    CPPFLAGS += --coverage -Og -DDEBUG1 -Wno-unused-variable -Finline-functions -Winline -pedantic
    LDFLAGS += --coverage -DDEBUG1 -fprofile-arcs
endif
ifeq ($(MAKECMDGOALS), covs)
    CPPFLAGS += -O1 -DDEBUG1 -Wno-unused-variable -pedantic -ffunction-sections
    LDFLAGS += -DDEBUG1 -Wl,--gc-sections -Wl,--print-gc-sections
endif
ifeq ($(MAKECMDGOALS), debug)
    CPPFLAGS += -DDEBUG1 -Og -g -pedantic -Wall -Wconversion -c -Wno-unused-label -Wno-unused-variable
    LDFLAGS += -DDEBUG1 
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

all normal cov covs sedebug release prof debug sanit: index
	
tune tuned: index-t
	
clean:
	rm -rf *.o carrot core gmon.out carrot.exe *.gc?? *.d carrot-t
index: $(OBJS)
	$(LINK) -o carrot $(OBJS) $(LDFLAGS)
index-t: $(OBJS)
	$(LINK) -o carrot-t $(OBJS) $(LDFLAGS)
$(OBJS): %.o: %.c
	$(CC) -c $< $(CPPFLAGS) -o $@

