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

#include "defines.h"
#include "pers.h"
#include "evaluate.h"
#include "globals.h"
#include "utils.h"
#include "macros.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include <wchar.h>
#include <assert.h>

int NZEROONLY=1;

extern const xmlChar _binary_pers_default_xml_start;

int lim_out=1;

int valuetoint(unsigned char *buf, int *bb, int max)
{
	int count = 0, i;
	char *p;
	char d[] = { ',', '\n' };
	char *last = NULL;
	//fixme
	p = (char*) buf;
	last = p;
	while (count < max) {
		if (*p == '\0') {
			bb[count] = atoi(last);
			last = p;
			count++;
			break;
		}
		for (i = 0; i < 2; i++) {
			if (*p == d[i]) {
				*p = '\0';
				bb[count] = atoi(last);
				last = p + 1;
				count++;
				break;
			}
		}
		p++;
	}
	return (count < max ? -1 : 0);
}

int valuetointW(wchar_t *buf, int *bb, int max)
{
	int count = 0, i;
	wchar_t *p;
	wchar_t d[] = { L',', L'\n' };
	wchar_t *last = NULL, *rest;
	long int res;

	p = (wchar_t*) buf;
	last = p;
	while (count < max) {
		if (*p == L'\0') {
			res = wcstol(last, &rest, 10);
			bb[count] = (int) res;
			last = p;
			count++;
			break;
		}
		for (i = 0; i < 2; i++) {
			if (*p == d[i]) {
				*p = L'\0';
				res = wcstol(last, &rest, 10);
				bb[count] = (int) res;
				last = p + 1;
				count++;
				break;
			}
		}
		p++;
	}
	return (count);
}

int parse_basic_value(xmlDocPtr doc, xmlNodePtr cur, int *st)
{
	wchar_t ww[256];
	int r=-1;
  
	xmlChar *s;
	s = xmlNodeGetContent(cur);
	if (s != NULL) {
		UTF8toWchar(s, ww, sizeof(wchar_t) * 256);
		r=valuetointW(ww, st, 1);
		xmlFree(s);
	}
	return r;
}

int parse_basic_value_n(xmlDocPtr doc, xmlNodePtr cur, int n, int *st){
	wchar_t ww[256];
	int r=-1;
  
	xmlChar *s;
	s = xmlNodeGetContent(cur);
	if (s != NULL) {
		UTF8toWchar(s, ww, sizeof(wchar_t) * 256);
		r=valuetointW(ww, st, n);
		xmlFree(s);
	}
	return r;
}


int parse_value(xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int *st)
{
	int r=-1;
	wchar_t ww[256];

	xmlChar *key;
	xmlChar *s;
	key = xmlNodeGetContent(cur);

	s = xmlGetProp(cur, (const xmlChar*) "gamestage");
	if (s != NULL) {
		UTF8toWchar(s, ww, sizeof(wchar_t) * 256);
		r = valuetointW(ww, st, 1);
		xmlFree(s);
		if(r!=1) return -1;
	}
	if (key != NULL) {
		UTF8toWchar(key, ww, sizeof(wchar_t) * 256);
		r = valuetointW(ww, bb, max);
		xmlFree(key);
	}

	return r;
}

int parse_value2(xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int *st, int *side, int *piece)
{
	int r=-1;
	wchar_t ww[2048];

	r = -1;
	xmlChar *key;
	xmlChar *s;
	*st = 9;
	*side = 9;
	*piece = 9;
	s = xmlGetProp(cur, (const xmlChar*) "gamestage");
	if (s != NULL) {
		if (UTF8toWchar(s, ww, sizeof(wchar_t) * 2048) != 0) {
			LOGGER_0("conversion2 error!");
			abort();
		}
		r = valuetointW(ww, st, 1);
		if(r!=1) return -1;
		xmlFree(s);
	}
	s = xmlGetProp(cur, (const xmlChar*) "piece");
	if (s != NULL) {
		if (UTF8toWchar(s, ww, sizeof(wchar_t) * 2048) != 0) {
			LOGGER_0("conversion3 error!");
			abort();
		}
		r = valuetointW(ww, piece, 1);
		if(r!=1) return -1;
		xmlFree(s);
	}
	s = xmlGetProp(cur, (const xmlChar*) "side");
	if (s != NULL) {
		if (UTF8toWchar(s, ww, sizeof(wchar_t) * 2048) != 0) {
			LOGGER_0("conversion4 error!");
			abort();
		}
		r = valuetointW(ww, side, 1);
		if(r!=1) return -1;
		xmlFree(s);
	}
	key = xmlNodeGetContent(cur);
	if (key != NULL) {
		if (UTF8toWchar(key, ww, sizeof(wchar_t) * 2048) != 0) {
			LOGGER_0("conversion1 error!");
			abort();
		}
		r = valuetointW(ww, bb, max);
		xmlFree(key);
	}
	return r;
}

int parse_value5(xmlDocPtr doc, xmlNodePtr cur, int *bb, int max, int *piece)
{
	int r;
	wchar_t ww[2048];

	r = -1;
	xmlChar *key;
	xmlChar *s;
	*piece = 9;
	s = xmlGetProp(cur, (const xmlChar*) "piece");
	if (s != NULL) {
		if (UTF8toWchar(s, ww, sizeof(wchar_t) * 2048) != 0) {
			LOGGER_0("conversion3 error!");
			abort();
		}
		r = valuetointW(ww, piece, 1);
		if(r!=1) return -1;
		xmlFree(s);
	}
	key = xmlNodeGetContent(cur);
	if (key != NULL) {
		if (UTF8toWchar(key, ww, sizeof(wchar_t) * 2048) != 0) {
			LOGGER_0("conversion1 error!");
			abort();
		}
		r = valuetointW(ww, bb, max);
		xmlFree(key);
	}
	return r;
}

int setup_gamestage(_gamestage *v, int *buffer, int stage)
{
	if (stage >= ER_GAMESTAGE)
		return 1;
	(*v)[stage] = *buffer;

	return 0;
}

int setup_value(_values *v, int *buffer, int count, int stage)
{
	int f;
	if (stage >= ER_GAMESTAGE)
		return 1;
	for (f = 0; f < count; f++) {
		(*v)[stage][f] = buffer[f];
	}
	return 0;
}

int setup_value5(_dvalues *v, int *buffer, int count, int piece)
{
	int f;
	if (piece >= ER_PIECE)
		return 1;
	for (f = 0; f < count; f++) {
		(*v)[piece][f] = buffer[f];
	}
	return 0;
}

int setup_value2(_squares_p *s, int *buffer, int count, int stage, int side, int piece)
{
	int f;
	if (stage >= ER_GAMESTAGE)
		return 1;
	if (side >= ER_SIDE)
		return 2;
	if (piece >= ER_PIECE+1)
		return 3;
	for (f = 0; f < count; f++) {
		(*s)[stage][side][piece][f] = buffer[f];
	}
	return 0;
}

int setup_value3(_mobility *s, int *buffer, int count, int stage, int side, int piece)
{
	int f;
	if (stage >= ER_GAMESTAGE)
		return 1;
	if (side >= ER_SIDE)
		return 2;
	if (piece >= ER_PIECE)
		return 3;
	for (f = 0; f < count; f++) {
		(*s)[stage][side][piece][f] = buffer[f];
		if (f % 8 == 0) {
		}
	}
	return 0;
}

int setup_value4(_passer *s, int *buffer, int count, int stage, int side)
{
	int f;
	if (stage >= ER_GAMESTAGE)
		return 1;
	if (side >= ER_SIDE)
		return 2;
	for (f = 0; f < count; f++) {
		(*s)[stage][side][f] = buffer[f];
	}
	return 0;
}

int reorganize_values(int *b)
{
	int pawn, queen, rook, bishop, king, knight;
	bishop = b[2];
	knight = b[1];
	rook = b[3];
	king = b[5];
	queen = b[4];
	pawn = b[0];

	b[PAWN] = pawn;
	b[KNIGHT] = knight;
	b[BISHOP] = bishop;
	b[ROOK] = rook;
	b[QUEEN] = queen;
	b[KING] = king;

	return 0;
}

int swap_board(int *bb)
{
	int f;
	int b;

	for (f = 0; f < 32; f++) {
		b = bb[f];
		bb[f] = bb[Square_Swap[f]];
		bb[Square_Swap[f]] = b;
	}
	return 0;
}

int params_init_general_option(_general_option *x, int s_r, int *i)
{
	*x = *i;
	return 0;
}

int params_load_general_option(xmlDocPtr doc, xmlNodePtr cur, int *st, int s_r, _general_option *o)
{
	int count;
	count=parse_basic_value(doc, cur, st);
	if(count==1) params_init_general_option(o, s_r, st);
	return 0;
}

int params_out_general_option(char *x, _general_option *i)
{
	if(NZEROONLY && *i!=0) LOGGER_0("PERS: %s %i\n", x, *i);
	return 0;
}

int params_write_general_option(xmlNodePtr parent, char *name, int s_r, _general_option *i)
{
	wchar_t bw[1024];
	xmlChar b8[1024], v8[1024];

	swprintf(bw, 999, L"%s", name);
	WchartoUTF8(bw, b8, 1024);

	swprintf(bw, 999, L"%i", *i);
	WchartoUTF8(bw, v8, 1024);
	xmlNewTextChild(parent, NULL, b8, v8);

	return 0;
}

int params_init_gamestage(_gamestage *x, int s_r, int *i)
{
	setup_gamestage(x, i, 0);
	setup_gamestage(x, i + 1, 1);
	return 0;
}

int params_load_gamestage(xmlDocPtr doc, xmlNodePtr cur, int *st, int s_r, _gamestage *o)
{
	int stage;
	int count;
	count=parse_value(doc, cur, st, 1, &stage);
	if(count==1) setup_gamestage(o, st, stage);
	return 0;
}

int params_out_gamestage(char *x, _gamestage *i)
{
	int f;
	int q=0;
	char buf[512], b2[512];
	sprintf(buf, "PERS: %s ", x);
	for (f = 0; f < ER_GAMESTAGE; f++) {
		if((*i)[f]!=0) { sprintf(b2, "GS[%i]:%i\t", f, (*i)[f]); q=1; }
		else sprintf(b2, "GS[%i]: ,\t ", f);

		strcat(buf, b2);
	}
		if(NZEROONLY && q!=0) LOGGER_0("%s\n", buf);
	return 0;
}

int params_write_gamestage(xmlNodePtr parent, char *name, int s_r, _gamestage *i)
{
	int f;

	wchar_t bw[1024];
	xmlChar b8[256], b82[256], n8[256];
	xmlNodePtr cur;
	swprintf(bw, 999, L"%s", name);
	WchartoUTF8(bw, n8, 256);
	for (f = 0; f < (ER_GAMESTAGE); f++) {
		swprintf(bw, 999, L"%i", (*i)[f]);
		WchartoUTF8(bw, b8, 256);
		swprintf(bw, 999, L"%d", f);
		WchartoUTF8(bw, b82, 256);
		cur = xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewProp(cur, (xmlChar*) "gamestage", b82);
	}

	return 0;
}

int params_init_values(_values *x, int s_r, int *i)
{
	setup_value(x, i, ER_PIECE, 0);
	setup_value(x, i + ER_PIECE, ER_PIECE, 1);
	return 0;
}

int params_init_general_option_8(_general_option_8 *x, int s_r, int *i)
{
	for(int n=0; n<8;n++) (*x)[n]=i[n];
	return 0;
}

int params_map_values(_values *x, int s_r, int *i)
{
	setup_value(x, i, ER_PIECE, 0);
	setup_value(x, i + ER_PIECE, ER_PIECE, 1);
	return 0;
}

int params_load_values(xmlDocPtr doc, xmlNodePtr cur, int *st, int s_r, _values *o)
{
	int val[256];
	int count;
	count=parse_value(doc, cur, val, ER_PIECE, st);
	if(count>=ER_PIECE) setup_value(o, val, ER_PIECE, *st);
	else L0("Values load problem\n");
	return 0;
}

int params_load_general_option_8(xmlDocPtr doc, xmlNodePtr cur, int *st, int s_r, _general_option_8 *o)
{
	int val[256];
	int count;
	int r=parse_basic_value_n(doc, cur, 8, val);
	if(r>=8) for(int i=0;i<8;i++) (*o)[i]=val[i];
	else L0("General_option_8 load problem\n");
	return 0;
}

int params_out_general_option_8(char *x, _general_option_8 *i)
{
	int f;
	char buf[512], b2[512];
	sprintf(buf, "PERS: %s ", x);
	for (f = 0; f < 8; f++) {
		sprintf(b2, "depth[%i]:%i,\t", f,(*i)[f]);
	}
	LOGGER_0("%s\n", b2);
	return 0;
}

int params_out_values(char *x, _values *i)
{
	int f;
	char buf[512], b2[512];
	sprintf(buf, "PERS: %s ", x);
	for (f = 0; f < 8; f++) {
		sprintf(b2, "PERS: VAL[%i]:%i, %i, %i, %i, %i, %i\t", f,
			(*i)[f][PAWN], (*i)[f][KNIGHT], (*i)[f][BISHOP],
			(*i)[f][ROOK], (*i)[f][QUEEN], (*i)[f][KING]);
		LOGGER_0("%s\n", b2);
	}
	return 0;
}

int params_write_values(xmlNodePtr parent, char *name, int s_r, _values *i)
{
	int f, n;
	char buf[512], b2[128];
	xmlNodePtr cur;

	wchar_t bw[1024];
	xmlChar b8[512], b82[256], n8[256];

	swprintf(bw, 999, L"%s", name);
	WchartoUTF8(bw, n8, 256);

	for (f = 0; f < (ER_GAMESTAGE); f++) {
		buf[0] = '\0';
		for (n = 0; n < 5; n++) {
			sprintf(b2, "%d,", (*i)[f][n]);
			strcat(buf, b2);
		}
		sprintf(b2, "%d", (*i)[f][5]);
		strcat(buf, b2);

		swprintf(bw, 999, L"%d", f);
		WchartoUTF8(bw, b82, 256);

		swprintf(bw, 999, L"%s", buf);
		WchartoUTF8(bw, b8, 256);

		cur = xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewProp(cur, (xmlChar*) "gamestage", b82);
	}
	return 0;
}

int params_write_general_option_8(xmlNodePtr parent, char *name, int s_r, _general_option_8 *i)
{
	int f, n;
	char buf[512], b2[128];
	xmlNodePtr cur;

	wchar_t bw[1024];
	xmlChar b8[512], b82[256], n8[256];

	swprintf(bw, 999, L"%s", name);
	WchartoUTF8(bw, n8, 256);

		buf[0] = '\0';
		for (n = 0; n < 7; n++) {
			sprintf(b2, "%d,", (*i)[n]);
			strcat(buf, b2);
		}
		sprintf(b2, "%d", (*i)[7]);
		strcat(buf, b2);

		swprintf(bw, 999, L"%s", buf);
		WchartoUTF8(bw, b8, 256);

//		cur = xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewTextChild(parent, NULL, n8, b8);
	return 0;
}

int params_init_dvalues(_dvalues *x, int s_r, int *i)
{
	int piece;
	for (piece = 0; piece < ER_PIECE; piece++) {
		LOGGER_3("_dvalues %p, %p, %d, %d\n",x, i + piece * (PAWNS_TOT), PAWNS_TOT, piece);
		setup_value5(x, i + piece * (PAWNS_TOT), PAWNS_TOT,
			piece);
	}
	return 0;
}

int params_load_dvalues(xmlDocPtr doc, xmlNodePtr cur, int *st, int s_r, _dvalues *o)
{
	int val[PAWNS_TOT + 1];
	int piece;
	int count;
	count=parse_value5(doc, cur, val, PAWNS_TOT, &piece);
	if(count>0) setup_value5(o, val, Min(count,PAWNS_TOT), piece);
	return 0;
}

int params_out_dvalues(char *x, _dvalues *i)
{
	int f, n;
	char buf[512], b2[512];
	sprintf(buf, "PERS: %s \n", x);
	for (f = 0; f < ER_PIECE; f++) {
		sprintf(buf, "VAL[%i]:\t", f);
		for (n = 0; n < (PAWNS_TOT-1); n++) {
			sprintf(b2, "%d\n", (*i)[f][n]);
			strcat(buf, b2);
		}
		sprintf(b2, "%d\n", (*i)[f][n]);
		strcat(buf, b2);

	}
	return 0;
}

int params_write_dvalues(xmlNodePtr parent, char *name, int s_r, _dvalues *i)
{
	int f, n;
	char buf[512], b2[128];
	xmlNodePtr cur;

	wchar_t bw[1024];
	xmlChar b8[512], b82[256], n8[256];

	swprintf(bw, 999, L"%s", name);
	WchartoUTF8(bw, n8, 256);

	for (f = 0; f < (ER_PIECE); f++) {
		buf[0] = '\0';
		for (n = 0; n < (PAWNS_TOT-1); n++) {
			sprintf(b2, "%d,", (*i)[f][n]);
			strcat(buf, b2);
		}
		sprintf(b2, "%d", (*i)[f][n]);
		strcat(buf, b2);

		swprintf(bw, 999, L"%d", f);
		WchartoUTF8(bw, b82, 256);

		swprintf(bw, 999, L"%s", buf);
		WchartoUTF8(bw, b8, 256);

		cur = xmlNewTextChild(parent, NULL, n8, b8);
		xmlNewProp(cur, (xmlChar*) "piece", b82);
	}
	return 0;
}

int params_init_passer(_passer *x, int s_r, int *i)
{
	int side, f;

	side = *i;
	i++;
	if (side == 2) {
		setup_value4(x, i, ER_RANKS, 0, 0);
		setup_value4(x, i + ER_RANKS, ER_RANKS, 1, 0);
		// swap if needed
		if (s_r & 1) {
			for (f = 0; f < ER_RANKS; f++)
				(*x)[0][1][f] = (*x)[0][0][ER_RANKS - 1 - f];
			for (f = 0; f < ER_RANKS; f++)
				(*x)[1][1][f] = (*x)[1][0][ER_RANKS - 1 - f];
		} else {
			for (f = 0; f < ER_RANKS; f++)
				(*x)[0][1][f] = (*x)[0][0][f];
			for (f = 0; f < ER_RANKS; f++)
				(*x)[1][1][f] = (*x)[1][0][f];
		}
	} else {
		if (side > 2)
			return 1;
		setup_value4(x, i, ER_RANKS, 0, side);
		setup_value4(x, i + ER_RANKS, ER_RANKS, 1, side);
		x += ER_RANKS * 2;
		side = *i;
		i++;
		if (side > 2)
			return 1;
		setup_value4(x, i, ER_RANKS, 0, side);
		setup_value4(x, i + ER_RANKS, ER_RANKS, 1, side);
	}
	return 0;
}

int params_load_passer(xmlDocPtr doc, xmlNodePtr cur, int *st, int s_r, _passer *o)
{
	int side, stage, piece, f;
	int bb[128];
	int count;

	count=parse_value2(doc, cur, bb, ER_RANKS, &stage, &side, &piece);
	if(count>0) {
		assert(stage != 9);
		assert(side != 9);
		if ((side == 1) || (side == 0)) {
			setup_value4(o, bb, count, stage, side);
		} else if (side == 2) {
			setup_value4(o, bb, count, stage, 0);
			if (s_r & 1) {
				for (f = 0; f < count; f++)
//					(*o)[stage][1][f] = (*o)[stage][0][ER_RANKS - 1 - f];
					(*o)[stage][1][f] = (*o)[stage][0][count - 1 - f]; //????
			} else {
				setup_value4(o, bb, count, stage, 1);
			}
		}
	}
	return 0;
}

int params_out_passer(char *x, _passer *i)
{
	int f,l,c;
	char buf[1024];
	char bb[256];
	int q[8];
	char cc[9][16];
	
	sprintf(buf, "PERS: %s ", x);

	for (f = 0; f < ER_GAMESTAGE; f++) {
#if 0
		LOGGER_0("PERS: %s GS[%i]:SIDE[0]=%i, %i, %i, %i, %i, %i, %i, %i\n", x,
			f, (*i)[f][0][0], (*i)[f][0][1], (*i)[f][0][2],
			(*i)[f][0][3], (*i)[f][0][4], (*i)[f][0][5],
			(*i)[f][0][6], (*i)[f][0][7]);
			
			
			
		LOGGER_0("PERS: %s GS[%i]:SIDE[1]=%i, %i, %i, %i, %i, %i, %i, %i\n", x,
			f, (*i)[f][1][0], (*i)[f][1][1], (*i)[f][1][2],
			(*i)[f][1][3], (*i)[f][1][4], (*i)[f][1][5],
			(*i)[f][1][6], (*i)[f][1][7]);
#endif
		c=0;
		for(l=0;l<8;l++) {
			q[l]=(*i)[f][0][l];
			if(q[l]==0) sprintf(cc[l], " "); else { sprintf(cc[l], "%d", q[l]); c=1; }
		}
			if(NZEROONLY && c!=0) LOGGER_0("PERS: %s GS[%i]:SIDE[0]=%s, %s, %s, %s, %s, %s, %s, %s\n", x,
			f, cc[0], cc[1], cc[2], cc[3], cc[4], cc[5], cc[6], cc[7]);

		c=0;
		for(l=0;l<8;l++) {
			q[l]=(*i)[f][1][l];
			if(q[l]==0) sprintf(cc[l], " "); else { sprintf(cc[l], "%d", q[l]); c=1; }
		}
			if(NZEROONLY && c!=0) LOGGER_0("PERS: %s GS[%i]:SIDE[1]=%s, %s, %s, %s, %s, %s, %s, %s\n", x,
			f, cc[0], cc[1], cc[2], cc[3], cc[4], cc[5], cc[6], cc[7]);
	}
	return 0;
}

int params_write_passer(xmlNodePtr parent, char *name, int s_r, _passer *i)
{
	int f, n, side;
	char buf[512], b2[512];
	xmlNodePtr cur;
	wchar_t bw[2048];
	xmlChar g8[256], s8[256], v8[1024], n8[256];

	swprintf(bw, 999, L"%s", name);
	WchartoUTF8(bw, n8, 256);

	for (f = 0; f < (ER_GAMESTAGE); f++) {
		// check for side type
		side = 2;
		for (n = 0; n < ER_RANKS; n++) {
			if (s_r & 1) {
				if ((*i)[f][1][n]
					!= (*i)[f][0][ER_RANKS - 1 - n]) {
					side = 0;
					break;
				}
			} else {
				if ((*i)[f][1][n] != (*i)[f][0][n]) {
					side = 0;
					break;
				}
			}
		}
		buf[0] = '\0';
		for (n = 0; n < (ER_RANKS - 1); n++) {
			sprintf(b2, "%d,", (*i)[f][0][n]);
			strcat(buf, b2);
		}

		swprintf(bw, 999, L"%s%d", buf, (*i)[f][0][ER_RANKS - 1]);
		WchartoUTF8(bw, v8, 1024);
		swprintf(bw, 999, L"%d", f);
		WchartoUTF8(bw, g8, 256);
		swprintf(bw, 999, L"%d", side);
		WchartoUTF8(bw, s8, 256);

		cur = xmlNewTextChild(parent, NULL, n8, v8);
		xmlNewProp(cur, (xmlChar*) "gamestage", g8);
		xmlNewProp(cur, (xmlChar*) "side", s8);
		//		xmlNewProp(cur, (xmlChar *) "value", v8);
		if (side != 2) {
			side = 1;
			buf[0] = '\0';
			if (s_r & 1) {
				for (n = ER_RANKS - 1; n > 0; n--) {
					sprintf(b2, "%d,", (*i)[f][1][n]);
					strcat(buf, b2);
				}
				sprintf(b2, "%d", (*i)[f][1][0]);
				strcat(buf, b2);
			} else {
				for (n = 0; n < (ER_RANKS - 1); n++) {
					sprintf(b2, "%d,", (*i)[f][1][n]);
					strcat(buf, b2);
				}
				sprintf(b2, "%d", (*i)[f][1][ER_RANKS - 1]);
				strcat(buf, b2);
			}

			swprintf(bw, 999, L"%s", buf);
			WchartoUTF8(bw, v8, 1024);
			swprintf(bw, 999, L"%d", side);
			WchartoUTF8(bw, s8, 256);

			cur = xmlNewTextChild(parent, NULL, n8, v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "side", s8);
		}
	}
	return 0;
}

#define OPTION_GS(x,y)	if ((!xmlStrcmp(cur->name, (const xmlChar *)x)))\
		{	parse_value (doc, cur, bb, 1, &stage);\
		setup_gamestage(&(y), bb, stage); }

#undef MLINE
#define MLINE(x,y,z,s_r,i)	if ((!xmlStrcmp(cur->name, (const xmlChar *) #x)))\
		{	params_load ## z(doc, cur, bb, s_r, &(p->y)); }

static void parsedoc_int(xmlDocPtr doc, personality *p)
{
	//char buf[256];
	int bb[128];
	int stage, side, piece;

	xmlNodePtr cur;

	if (doc == NULL) {
		fprintf(stderr, "Document not parsed successfully. \n");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	if (xmlStrcmp(cur->name, (xmlChar*) "configuration")) {
		fprintf(stderr,
			"document of the wrong type, root node != documentation");
		xmlFreeDoc(doc);
		return;
	}
	cur = cur->xmlChildrenNode;
	// musime najit material a nastavit premapovavani figur, v souboru je to vzdy v poradi PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
	// ktere ale v programu nemusi mit takto pridelena cisla

	while (cur != NULL) {

		E_OPTS;
		// musi byt prochazeno jako prvni...
		if ((!xmlStrcmp(cur->name, (const xmlChar*) "PieceToSquare"))) {
			parse_value2(doc, cur, bb, 64, &stage, &side, &piece);
			piece = Piece_Map[piece];

			if ((side == 1) || (side == 0)) {
				setup_value2(&(p->piecetosquare), bb, 64, stage,
					side, piece);
			} else if (side == 2) {
				setup_value2(&(p->piecetosquare), bb, 64, stage,
					0, piece);
				swap_board(bb);
				setup_value2(&(p->piecetosquare), bb, 64, stage,
					1, piece);
			}
		}
		if ((!xmlStrcmp(cur->name, (const xmlChar*) "Mobility"))) {
			parse_value2(doc, cur, bb, 29, &stage, &side, &piece);
			piece = Piece_Map[piece];
			if ((side == 1) || (side == 0)) {
				setup_value3(&(p->mob_val), bb, 29, stage, side,
					piece);
			} else if (side == 2) {
				setup_value3(&(p->mob_val), bb, 29, stage, 0,
					piece);
				setup_value3(&(p->mob_val), bb, 29, stage, 1,
					piece);
			}
		}
		if ((!xmlStrcmp(cur->name, (const xmlChar*) "MobUnSec"))) {
			parse_value2(doc, cur, bb, 29, &stage, &side, &piece);
			piece = Piece_Map[piece];
			if ((side == 1) || (side == 0)) {
				setup_value3(&(p->mob_uns), bb, 29, stage, side,
					piece);
			} else if (side == 2) {
				setup_value3(&(p->mob_uns), bb, 29, stage, 0,
					piece);
				setup_value3(&(p->mob_uns), bb, 29, stage, 1,
					piece);
			}
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);

	// make sure that King value is high enough, needed for SEE
	int v0, v1;
	v0 = (p->Values[0][PAWN] + p->Values[0][KNIGHT] + p->Values[0][BISHOP]
		+ p->Values[0][ROOK] + p->Values[0][QUEEN]) * 10;
	v1 = (p->Values[1][PAWN] + p->Values[1][KNIGHT] + p->Values[1][BISHOP]
		+ p->Values[1][ROOK] + p->Values[1][QUEEN]) * 10;
	p->Values[0][KING] = Max(v0, 88888);
	p->Values[1][KING] = Max(v1, 88888);
	LOGGER_3("%d %d %d %d %d %d\n", p->Values[0][PAWN],
		p->Values[0][KNIGHT], p->Values[0][BISHOP], p->Values[0][ROOK],
		p->Values[0][QUEEN], p->Values[0][KING]);
	LOGGER_3("KING %d, %d\n", p->Values[0][KING], p->Values[1][KING]);

	return;
}

#undef MLINE
#define MLINE(x,y,z,s_r,i) { int __qq__[]={i}; params_init ## z(&(p->y), s_r, __qq__); }

void setup_init_pers(personality *p)
{
	int f, x, i;

	E_OPTS
	;

	for (f = 0; f < ER_GAMESTAGE; f++) {
		for (x = 0; x < ER_PIECE+1; x++) {
			for (i = A1; i <= H8; i++) {
				p->piecetosquare[f][WHITE][x][i] = 0;
				p->piecetosquare[f][BLACK][x][i] = 0;
			}
		}
	}
	for (f = 0; f < ER_GAMESTAGE; f++) {
		for (x = 0; x < ER_PIECE; x++) {
			for (i = 0; i < ER_MOBILITY; i++) {
				p->mob_val[f][WHITE][x][i] = 0;
				p->mob_val[f][BLACK][x][i] = 0;
			}
		}
	}
	for (f = 0; f < ER_GAMESTAGE; f++) {
		for (x = 0; x < ER_PIECE; x++) {
			for (i = 0; i < ER_MOBILITY; i++) {
				p->mob_uns[f][WHITE][x][i] = 0;
				p->mob_uns[f][BLACK][x][i] = 0;
			}
		}
	}
}

int print_pers_values2(char *b, _squares_p *s, int count, int stage, int side, int piece)
{
	int f,i;
	int q=0;
	char b2[2048];
	i=(*s)[stage][side][piece][0];
		if(i!=0) { sprintf(b, ", %6d", i); q=1;} else sprintf(b, ",  ");
//	sprintf(b, "%6d", (*s)[stage][side][piece][0]);
	for (f = 1; f < count; f++) {
		i=(*s)[stage][side][piece][f];
		if(i!=0) sprintf(b2, ", %6d", i); else sprintf(b2, ",  ");

//		sprintf(b2, ", %6d", (*s)[stage][side][piece][f]);
		strcat(b, b2);
	}
	return q;
}

int print_pers_values3(char *b, _mobility *s, int count, int stage, int side, int piece)
{
	int f, i;
	char b2[2048];
	char bb[256];
	int q=0;
	i=(*s)[stage][side][piece][0];
	if(i!=0) sprintf(b, "%2d", i); else sprintf(b, "  ");
	for (f = 1; f < count; f++) {
		i=(*s)[stage][side][piece][f];
		if(i!=0) { sprintf(b2, ", %2d", i); q=1; }  else sprintf(b2, ",  ");
//		sprintf(b2, ", %2d", (*s)[stage][side][piece][f]);
		strcat(b, b2);
	}
	return q;
}

int print_pers_values(char *b, _squares_p *s, int count, int stage, int side, int piece)
{
	int f,n, i[8], l;
	int c=0;
	char b2[2048], q[8][16];
	f = 7;
	b[0]='\0';
	for(n=7;n>=0;n--) {
	  for (f = n*1+0; f >= n*1; f--) {
#if 0
		sprintf(b2, "%1d\t%1d\t%1d\t%1d\t%1d\t%1d\t%1d\t%1d|\t",
			(*s)[stage][side][piece][f * 8 + 0],
			(*s)[stage][side][piece][f * 8 + 1],
			(*s)[stage][side][piece][f * 8 + 2],
			(*s)[stage][side][piece][f * 8 + 3],
			(*s)[stage][side][piece][f * 8 + 4],
			(*s)[stage][side][piece][f * 8 + 5],
			(*s)[stage][side][piece][f * 8 + 6],
			(*s)[stage][side][piece][f * 8 + 7]);
		strcat(b, b2);
#endif

		for(l=0;l<8;l++) {
			i[l]=(*s)[stage][side][piece][f * 8 + l];
			if(i[l]==0) sprintf(q[l], " "); else { sprintf(q[l], "%d", i[l]), c=1; };
		}
		sprintf(b2, "%1s,\t%1s,\t%1s,\t%1s,\t%1s,\t%1s,\t%1s,\t%1s|\t",q[0],q[1],q[2],q[3],q[4],q[5],q[6],q[7]);
		strcat(b, b2);
	  }
	  strcat(b ,"\n");
	}
	return c;
}

#undef MLINE
#define MLINE(x,y,z,s_r,i) { params_out ## z(#x,&(p->y)); }

int personality_dump(personality *p)
{
	int f, x;
	char buf[2048];

	DEB_0(E_OPTS;)

	for (f = 0; f < ER_GAMESTAGE; f++) {
		for (f = 0; f < ER_GAMESTAGE; f++) {
			for (x = 0; x < ER_PIECE; x++) {
				if(print_pers_values(buf, &(p->piecetosquare), 64,
					f, WHITE, x))
				LOGGER_0("PERS: PSQ, stage %d, side %d, piece %d\n%s",
					f, 0, x, buf);
			}
		}
		for (f = 0; f < ER_GAMESTAGE; f++) {
			for (x = 0; x < ER_PIECE; x++) {
				if(print_pers_values(buf, &(p->piecetosquare), 64,
					f, BLACK, x))
				LOGGER_0("PERS: PSQ, stage %d, side %d, piece %d\n%s",
					f, 1, x, buf);
			}
		}
		for (x = 0; x < ER_SIDE; x++) {
			for (f = 0; f < ER_GAMESTAGE; f++) {
				if(print_pers_values3(buf, &(p->mob_val), 13, f, x,
					PAWN))
				LOGGER_0(
					"PERS: Mob, stage %d, side %d, piece %d, %s\n",
					f, x, 0, buf);
				if(print_pers_values3(buf, &(p->mob_val), 9, f, x,
					KNIGHT))
				LOGGER_0(
					"PERS: Mob, stage %d, side %d, piece %d, %s\n",
					f, x, 1, buf);
				if(print_pers_values3(buf, &(p->mob_val), 14, f, x,
					BISHOP))
				LOGGER_0(
					"PERS: Mob, stage %d, side %d, piece %d, %s\n",
					f, x, 2, buf);
				if(print_pers_values3(buf, &(p->mob_val), 15, f, x,
					ROOK))
				LOGGER_0(
					"PERS: Mob, stage %d, side %d, piece %d, %s\n",
					f, x, 3, buf);
				if(print_pers_values3(buf, &(p->mob_val), 29, f, x,
					QUEEN))
				LOGGER_0(
					"PERS: Mob, stage %d, side %d, piece %d, %s\n",
					f, x, 4, buf);
				if(print_pers_values3(buf, &(p->mob_val), 9, f, x,
					KING))
				LOGGER_0("PERS: Mob, stage %d, side %d, piece %d, %s\n",
					f, x, 5, buf);
			}
		}
	}
	return 0;
}

int load_personality(char *docname, personality *p)
{
	xmlDocPtr doc;

	doc = xmlReadFile(docname, NULL,0);
	parsedoc_int(doc, p);
//	return;
//	parsedoc(docname, p);
	return 1;
}

int load_personality_inmem(personality *p)
{
	xmlDocPtr doc;

	doc = xmlReadDoc(&_binary_pers_default_xml_start, "noname.xml", NULL,0);
	if(doc==NULL) return 0;
	parsedoc_int(doc, p);

return 1;
}

void* init_personality(char *docname)
{
	personality *p;

	p = (personality*) malloc(sizeof(personality));
	setup_init_pers(p);
	load_personality_inmem(p);
	if (docname != NULL) if(load_personality(docname, p)) {
			LOGGER_1("INFO: Personality file: %s loaded.\n",docname);
			DEB_3(personality_dump(p));
	}
//	meval_table_gen(p->mat, p, 0);
//	meval_table_gen(p->mate_e, p, 1);
//	mat_info(p->mat_info);
//	mat_faze(p->mat_faze);
	meval_t_gen(p);
	MVVLVA_gen((p->LVAcap), p->Values);
	return p;
}

#undef MLINE
#define MLINE(x,y,z,s_r,i)	{ params_write ## z(root, #x, s_r, &(p->y)); }

int write_personality(personality *p, char *docname)
{
	xmlDocPtr doc;
	xmlNodePtr root, cur;
	char buf[1024], b2[1024];

	wchar_t bw[2048];
	xmlChar g8[512], s8[512], v8[2048], p8[512];

	int gs, piece, sq;

	int mob_lengths[] = { 13, 9, 14, 15, 28, 9, -1 };

	doc = xmlNewDoc((unsigned char*) "1.0");
	xmlNewDocComment(doc, (xmlChar*) "test");
	root = xmlNewDocNode(doc, NULL, (xmlChar*) "configuration", NULL);
	xmlDocSetRootElement(doc, root);

	E_OPTS
	;

	//psts gs, piece, side, value

int check;

	for (piece = 0; piece <= ER_PIECE; piece++) {
		swprintf(bw, 999, L"%d", piece);
		WchartoUTF8(bw, p8, 512);
		for (gs = 0; gs <= 1; gs++) {
			swprintf(bw, 999, L"%d", gs);
			WchartoUTF8(bw, g8, 512);

			check=0;

			for (sq = A1; sq <= H8; sq++) {
				if(p->piecetosquare[gs][WHITE][piece][sq]!=p->piecetosquare[gs][BLACK][piece][Square_Swap[sq]]) {
					check=1;
					break;
				}
			}
			if(check==0) {

			buf[0] = '\0';
			for (sq = A1; sq < H8; sq++) {
				sprintf(b2, "%d,",
					p->piecetosquare[gs][WHITE][piece][sq]);
				strcat(buf, b2);
			}

			swprintf(bw, 999, L"%s%d", buf,
				p->piecetosquare[gs][WHITE][piece][sq]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw, 999, L"%d", 2);
			WchartoUTF8(bw, s8, 512);
			cur = xmlNewTextChild(root, NULL,
				(xmlChar*) "PieceToSquare", v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "piece", p8);
			xmlNewProp(cur, (xmlChar*) "side", s8);
			} else 
			{
			buf[0] = '\0';
			for (sq = A1; sq < H8; sq++) {
				sprintf(b2, "%d,",
					p->piecetosquare[gs][WHITE][piece][sq]);
				strcat(buf, b2);
			}
			swprintf(bw, 999, L"%s%d", buf,
				p->piecetosquare[gs][WHITE][piece][sq]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw, 999, L"%d", 0);
			WchartoUTF8(bw, s8, 512);
			cur = xmlNewTextChild(root, NULL,
				(xmlChar*) "PieceToSquare", v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "piece", p8);
			xmlNewProp(cur, (xmlChar*) "side", s8);

			buf[0] = '\0';
			for (sq = A1; sq < H8; sq++) {
				sprintf(b2, "%d,",
					p->piecetosquare[gs][BLACK][piece][sq]);
				strcat(buf, b2);
			}
			swprintf(bw, 999, L"%s%d", buf,
				p->piecetosquare[gs][BLACK][piece][sq]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw, 999, L"%d", 1);
			WchartoUTF8(bw, s8, 512);
			cur = xmlNewTextChild(root, NULL,
				(xmlChar*) "PieceToSquare", v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "piece", p8);
			xmlNewProp(cur, (xmlChar*) "side", s8);
			}
		}
	}

// check mobility

	for (piece = 0; piece <= 5; piece++) {
		swprintf(bw, 999, L"%d", piece);
		WchartoUTF8(bw, p8, 512);
		for (gs = 0; gs <= 1; gs++) {
			swprintf(bw, 999, L"%d", gs);
			WchartoUTF8(bw, g8, 512);
			
			check=0;

			for (sq = 0; sq < (mob_lengths[piece]); sq++) {
				if(p->mob_val[gs][WHITE][piece][sq]!=p->mob_val[gs][BLACK][piece][sq]) {
					check=1;
					break;
				}
			}
			if(check==0) {
			buf[0] = '\0';
			for (sq = 0; sq < (mob_lengths[piece] - 1); sq++) {
				sprintf(b2, "%d,",
					p->mob_val[gs][WHITE][piece][sq]);
				strcat(buf, b2);
			}
			swprintf(bw, 999, L"%s%d", buf,
				p->mob_val[gs][WHITE][piece][mob_lengths[piece] - 1]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw, 999, L"%d", 2);
			WchartoUTF8(bw, s8, 512);
			cur = xmlNewTextChild(root, NULL, (xmlChar*) "Mobility",
				v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "piece", p8);
			xmlNewProp(cur, (xmlChar*) "side", s8);
			} else
			{
			buf[0] = '\0';
			for (sq = 0; sq < (mob_lengths[piece] - 1); sq++) {
				sprintf(b2, "%d,",
					p->mob_val[gs][WHITE][piece][sq]);
				strcat(buf, b2);
			}
			swprintf(bw, 999, L"%s%d", buf,
				p->mob_val[gs][WHITE][piece][mob_lengths[piece] - 1]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw, 999, L"%d", 0);
			WchartoUTF8(bw, s8, 512);
			cur = xmlNewTextChild(root, NULL, (xmlChar*) "Mobility",
				v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "piece", p8);
			xmlNewProp(cur, (xmlChar*) "side", s8);

			buf[0] = '\0';
			for (sq = 0; sq < (mob_lengths[piece] - 1); sq++) {
				sprintf(b2, "%d,",
					p->mob_val[gs][BLACK][piece][sq]);
				strcat(buf, b2);
			}
			swprintf(bw, 999, L"%s%d", buf,
				p->mob_val[gs][BLACK][piece][mob_lengths[piece] - 1]);
			WchartoUTF8(bw, v8, 2048);
			swprintf(bw, 999, L"%d", 1);
			WchartoUTF8(bw, s8, 512);
			cur = xmlNewTextChild(root, NULL, (xmlChar*) "Mobility",
				v8);
			xmlNewProp(cur, (xmlChar*) "gamestage", g8);
			xmlNewProp(cur, (xmlChar*) "piece", p8);
			xmlNewProp(cur, (xmlChar*) "side", s8);

			}
		}
	}

	for (piece = 0; piece <= 5; piece++) {
		swprintf(bw, 999, L"%d", piece);
		WchartoUTF8(bw, p8, 512);
		for (gs = 0; gs <= 1; gs++) {
			swprintf(bw, 999, L"%d", gs);
			WchartoUTF8(bw, g8, 512);

			check=0;
			for (sq = 0; sq < (mob_lengths[piece]); sq++) {
				if(p->mob_uns[gs][WHITE][piece][sq]!=p->mob_uns[gs][BLACK][piece][sq]) {
					check=1;
					break;
				}
			}

			if(check==0) {
				buf[0] = '\0';
				for (sq = 0; sq < (mob_lengths[piece] - 1); sq++) {
					sprintf(b2, "%d,",
						p->mob_uns[gs][WHITE][piece][sq]);
					strcat(buf, b2);
				}
				swprintf(bw, 999, L"%s%d", buf,
					p->mob_uns[gs][0][piece][mob_lengths[piece] - 1]);
				WchartoUTF8(bw, v8, 2048);
				swprintf(bw, 999, L"%d", 2);
				WchartoUTF8(bw, s8, 512);
				cur = xmlNewTextChild(root, NULL, (xmlChar*) "MobUnSec",
					v8);
				xmlNewProp(cur, (xmlChar*) "gamestage", g8);
				xmlNewProp(cur, (xmlChar*) "piece", p8);
				xmlNewProp(cur, (xmlChar*) "side", s8);
			} else
			{
				buf[0] = '\0';
				for (sq = 0; sq < (mob_lengths[piece] - 1); sq++) {
					sprintf(b2, "%d,",
						p->mob_uns[gs][WHITE][piece][sq]);
					strcat(buf, b2);
				}
				swprintf(bw, 999, L"%s%d", buf,
					p->mob_uns[gs][WHITE][piece][mob_lengths[piece] - 1]);
				WchartoUTF8(bw, v8, 2048);
				swprintf(bw, 999, L"%d", 0);
				WchartoUTF8(bw, s8, 512);
				cur = xmlNewTextChild(root, NULL, (xmlChar*) "MobUnSec",
					v8);
				xmlNewProp(cur, (xmlChar*) "gamestage", g8);
				xmlNewProp(cur, (xmlChar*) "piece", p8);
				xmlNewProp(cur, (xmlChar*) "side", s8);
				buf[0] = '\0';
				for (sq = 0; sq < (mob_lengths[piece] - 1); sq++) {
					sprintf(b2, "%d,",
						p->mob_uns[gs][BLACK][piece][sq]);
					strcat(buf, b2);
				}
				swprintf(bw, 999, L"%s%d", buf,
					p->mob_uns[gs][BLACK][piece][mob_lengths[piece] - 1]);
				WchartoUTF8(bw, v8, 2048);
				swprintf(bw, 999, L"%d", 1);
				WchartoUTF8(bw, s8, 512);
				cur = xmlNewTextChild(root, NULL, (xmlChar*) "MobUnSec",
					v8);
				xmlNewProp(cur, (xmlChar*) "gamestage", g8);
				xmlNewProp(cur, (xmlChar*) "piece", p8);
				xmlNewProp(cur, (xmlChar*) "side", s8);
			}
		}
	}

	xmlSaveFormatFile(docname, doc, 1);
	xmlFreeDoc(doc);
	return 0;
}

int copyPers(personality *source, personality *dest)
{
	memcpy(dest, source, sizeof(personality));
	return 0;
}
