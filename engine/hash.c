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

#include "generate.h"
#include "search.h"
#include "utils.h"
#include "globals.h"
#include "evaluate.h"
#include "movgen.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "randoms.h"
//#include "randoms2.h"
#include <assert.h>
#include "inlines.h"

kmoves kmove_store[MAXPLY * KMOVES_WIDTH];

#define HASHBITS

BITVAR getRandomX(BITVAR *i)
{
	BITVAR ret;
	int l;
	size_t r;
	(*i)++;
	int rd = open("/dev/urandom", O_RDONLY);
	l = 0;
	ret = 0;
	while (l < sizeof(BITVAR)) {
		ssize_t res = read(rd, &r, sizeof(uint8_t));
		if (res < 0) {
			// error, unable to read /dev/random
		} else {
			l += 1;
			ret <<= 8;
			ret += (uint8_t) r;
		}
	}
	close(rd);
	return ret;
}

BITVAR getRandom(int *i)
{
	(*i)++;
	return RANDOMS[*i];
}

// sq, sd, pie = 768
// ep = 8
// sideKey = 1
// castle = 4

void initRandom()
{
	int *y;
	int sq, sd, pc, i;
	y = &i;
	i = -1;
	for (sq = 0; sq < ER_SQUARE; sq++)
		for (sd = 0; sd < ER_SIDE; sd++) {
			for (pc = 0; pc < ER_PIECE; pc++) {
				randomTable[sd][sq][pc] = getRandom(y);
			}
		}
//	sideKey = getRandom(y);
	sideKey = 1ULL;
	for (sq = A1; sq <= H1; sq++) {
		epKey[sq] = getRandom(y);
		epKey[sq + 8] = epKey[sq];
		epKey[sq + 16] = epKey[sq];
		epKey[sq + 24] = epKey[sq];
		epKey[sq + 32] = epKey[sq];
		epKey[sq + 40] = epKey[sq];
		epKey[sq + 48] = epKey[sq];
		epKey[sq + 56] = epKey[sq];
	}
		epKey[0] = 0;

	for (sd = 0; sd < ER_SIDE; sd++) {
		castleKey[sd][NOCASTLE] = 0;
		castleKey[sd][QUEENSIDE] = getRandom(y);
		castleKey[sd][KINGSIDE] = getRandom(y);
		castleKey[sd][BOTHSIDES] = castleKey[sd][QUEENSIDE] ^ castleKey[sd][KINGSIDE];
	}
}

BITVAR getKey(board *b)
{
	BITVAR x;
	BITVAR key;
	int from;
	key = 0;
	x = b->colormaps[WHITE];
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[WHITE][from][b->pieces[from]];
	}
	x = b->colormaps[BLACK];
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[BLACK][from][b->pieces[from] & PIECEMASK];
	}
	key ^= castleKey[WHITE][b->castle[WHITE]];
	key ^= castleKey[BLACK][b->castle[BLACK]];
	if (b->side == BLACK)
		key ^= sideKey;
	if (b->ep != -1)
		key ^= epKey[b->ep];
	return key;
}

void setupRandom(board *b)
{
	b->key = getKey(b);
	b->positions[b->move - b->move_start] = b->key;
	b->posnorm[b->move - b->move_start] = b->norm;
}

void analyzeHash(hashStore *hs){
unsigned long long used, notused, fused, fnotused;

	used=notused=fused=fnotused=0;
	hashBucket *p;
	int f,i;
	for(f=0; f< hs->hashlen; f++) {
		p=hs->hash+f*HASHPOS;
		if(UNPACKHASHAGE(p[0].pld)) fused++; else fnotused++;
		for(i=0;i < HASHPOS; i+=1) {
			if(UNPACKHASHAGE(p[i].pld)) used++; else notused++;
		}
	}
	L0("Hash used %lld, not used %lld,(%f), FIRST used %lld, not %lld, (%f)\n", used, notused, 100.0*used/(used+notused), fused, fnotused, 100.0*fused/(fused+fnotused));
}

/*
 if mated, then score propagated is -MATESCORE+current_DEPTH for in mate position
 we should store score into hash table modified by distance from current depth to depth the mate position occurred
 */

void storeHashX(hashStore *hs, BITVAR key, BITVAR pld, BITVAR ver, struct _statistics *s) {

	int i, c, q, id;
	BITVAR f, hi;
	hashBucket *h;

	s->hashStores++;
	f = key & (BITVAR) (hs->hashlen - 1);
	hi = key;
	h=hs->hash+f*HASHPOS;
	
	c=HASHPOS- 1;

	for (i = 0; i < HASHPOS; i +=1 ) {
		if(((UNPACKHASHAGE(h[i].pld))&0x3F) !=0) 
		  if ((hi == h[i].key) && (h[i].ver==ver)) {
// mame nas zaznam
			s->hashStoreHits++;
			s->hashStoreInPlace++;
			c = i;
			if(pld==h[i].pld) return;
			goto replace;
		}
	}	for (i = 0; i< HASHPOS; i += 1) {
		if (UNPACKHASHAGE(h[i].pld) == 0) {
				c = i;
				goto replace;
		}
	}
	q=-1;
	for (i = 0; i < HASHPOS; i+= 1) {
		int qq=(hs->hashValidId - UNPACKHASHAGE(h[i].pld))&0x3F;
		if (qq > q) {
			q = qq;
			c = i;
		}
	}

replace:
	h[c].key=hi;
	h[c].pld=pld;
	h[c].ver=ver;
}

void storeHash(hashStore *hs, hashEntry *hash, int side, int ply, int depth, BITVAR ver, struct _statistics *s)
{
	int i, c, q, id;
	BITVAR f, hi;
	BITVAR pld;
	hashBucket *h;

	switch (isMATE(hash->value)) {
	case -1:
		hash->value -= ply;
		break;
	case 1:
		hash->value += ply;
		break;
	default:
		break;
	}
	pld=PACKHASH(hash->bestmove, hash->value, hash->depth, hash->scoretype, hs->hashValidId);
	storeHashX(hs, hash->key, pld, ver, s);
	hash->pld=pld;
}

// 6 32 15 9 2, 0 6 38 53 62 64
void storeExactPV(hashStore *hs, BITVAR key, BITVAR ver, tree_store *orig, int level)
{
	int i, c, n, m;
	BITVAR f, hi;

	if (level > MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	f = key % (BITVAR) hs->hashPVlen;
	hi = key / (BITVAR) hs->hashPVlen;
	for (i = 0; i < 16; i++) {
		if ((hi == hs->pv[f].e[i].key)&&(hs->pv[f].e[i].map==ver)) {
// mame nas zaznam
			c = i;
			goto replace;
		}
	}
	for (i = 0; i < 16; i++) {
		if ((hs->pv[f].e[i].age != hs->hashValidId)) {
			c = i;
			goto replace;
		}
	}
	c = 0;
	replace: hs->pv[f].e[c].key = hi;
	hs->pv[f].e[c].age = (uint8_t) hs->hashValidId;
	hs->pv[f].e[c].map = ver;
	for (n = level, m = 0; n <= MAXPLY; n++, m++) {
		hs->pv[f].e[c].pv[m] = orig->tree[level][n];
	}
}

int restoreExactPV(hashStore *hs, BITVAR key, BITVAR ver, int level, tree_store *rest)
{
	int i, n, m, c;
	BITVAR f, hi;
	char buff[2048], b2[128];;

	if (level > MAXPLY) {
		LOGGER_0("Error Depth: %d\n", level);
		abort();
	}

	f = key % (BITVAR) hs->hashPVlen;
	hi = key / (BITVAR) hs->hashPVlen;
	for (i = 0; i < 16; i++) {
		if ((hi == hs->pv[f].e[i].key) && (hs->pv[f].e[i].age == hs->hashValidId) && (hs->pv[f].e[i].map == ver )){
			for (n = level + 1, m = 0; n <= MAXPLY; n++, m++) {
				rest->tree[level][n] = hs->pv[f].e[i].pv[m];
			}
		}
		return 1;
	} 
	sprintf(buff, "ExPV: No restore!\n");
	for (i = 0; i < 16; i++) {
		sprintf(b2,"id:%d ", i);
		strcat(buff, b2);
		
		if (!( hs->pv[f].e[i].age==0)) {
			sprintf(b2,"unused entry\n");
			strcat(buff, b2);
		} else if (!(hi == hs->pv[f].e[i].key)) {
			sprintf(b2,"wrong key\n");
			strcat(buff, b2);
		} else if (!(ver == hs->pv[f].e[i].map)) {
			sprintf(b2,"wrong ver\n");
			strcat(buff, b2);
		} else if (!(hs->hashValidId == hs->pv[f].e[i].age)) {
			sprintf(b2,"wrong ver\n");
			strcat(buff, b2);
		}
		L0(buff);
	} 
	return 0;
}

int initHash(hashStore *hs)
{
	memset(hs->hash, 0, sizeof(hashBucket) * (hs->hashlen) * HASHPOS);
	memset(hs->pv, 0, sizeof(hashEntryPV_e) * hs->hashPVlen);
	hs->hashValidId = 1;
	return 0;
}

int retrieveHash(hashStore *hs, hashEntry *hash, int side, int ply, int depth, int use_previous, BITVAR ver,struct _statistics *s)
{
	int i;
	BITVAR f, hi, pld;
	hashBucket *h;
	
	s->hashAttempts++;
	f = hash->key & (BITVAR) (hs->hashlen - 1);
	hi = hash->key;
	h=hs->hash+ f*HASHPOS;

	for (i = 0; i < HASHPOS; i+=1) {
		if ((h[i].key == hi)
			&& (h[i].ver==ver) 
			&& (UNPACKHASHAGE(h[i].pld)!=0)
			&& ((use_previous > 0)
				|| ((use_previous == 0)
					&& (UNPACKHASHAGE(h[i].pld)== hs->hashValidId))))
			break;
	}
	if (i >= HASHPOS) {
		s->hashMiss++;
		return 0;
	}
	
	pld=h[i].pld;
	UNPACKHASH(pld, hash->bestmove, hash->value, hash->depth, hash->scoretype, hash->age)
	
	if(hash->age!=hs->hashValidId) UPDATEHASHAGE(h[i].pld, hs->hashValidId);
	
	s->hashHits++;

	switch (isMATE(hash->value)) {
	case -1:
		hash->value += ply;
		break;
	case 1:
		hash->value -= ply;
		break;
	default:
		break;
	}
	return 1;
}

void dumpHash(board *b, hashStore *hs, hashEntry *hash, int side, int ply, int depth, int use_previous)
{
#if 0
	BITVAR k;
	int i;
	BITVAR f, hi;
	char b2[10];

	k = getKey(b);
	printBoardNice(b);
	f = hash->key % (BITVAR) hs->hashlen;
	hi = hash->key / (BITVAR) hs->hashlen;
	for (i = 0; i < HASHPOS; i++) {
		if ((hs->hash[f].e[i].key == hi)
			&& ((use_previous > 0)
				|| ((use_previous == 0)
					&& (hs->hash[f].e[i].age
						== hs->hashValidId))))
			break;
	}
	if (i == HASHPOS) {
		LOGGER_0("HASH MISS\n");
	} else {
		LOGGER_0("hash HI entry key %lld, hi %lld ",
			hs->hash[f].e[i].key, hi);
		if (hs->hash[f].e[i].key == hi)
			LOGGER_0("match\n");
		else
			LOGGER_0("Fail\n");
		LOGGER_0("hash Previous %d\n", use_previous);
		LOGGER_0("hash AGE entry map %lld, ageID %lld ",
			hs->hash[f].e[i].age, hs->hashValidId);
		if (hs->hash[f].e[i].age == hs->hashValidId)
			LOGGER_0("match\n");
		else
			LOGGER_0("Fail\n");
		LOGGER_0("hash DEPTH %d\n", hash->depth);
		LOGGER_0("hash MATE %d\n", isMATE(hash->value));
		sprintfMoveSimple(hash->bestmove, b2);
		LOGGER_0("hash VAL %d, hash MOVE %s\n", hash->value, b2);
		LOGGER_0("hash HASH material key used %lld, recomputed %lld",
			hash->key, k);
		if (hash->key == k)
			LOGGER_0("match\n");
		else
			LOGGER_0("Fail\n");
	}
#endif
}

int invalidateHash(hashStore *hs)
{
// check for NULL should be part of a caller!
	if (hs != NULL) {
		hs->hashValidId++;
		if (hs->hashValidId > 63)
			hs->hashValidId = 1;
	}
	return 0;
}

int clear_killer_moves(kmoves *km)
{
	int i, f;
	kmoves *g;
	for (f = 0; f < MAXPLY; f++) {
		g = &(km[f * KMOVES_WIDTH]);
		for (i = 0; i < KMOVES_WIDTH; i++) {
			g->move = NA_MOVE;
			g->value = 0;
			g++;
		}
	}
	return 0;
}

// just 2 killers
int update_killer_move(kmoves *km, int ply, MOVESTORE move, struct _statistics *s)
{
	kmoves *a, *b;
	a = &(km[ply * KMOVES_WIDTH]);
	if (a->move == move)
		return 1;
	b = a + 1;
	*b = *a;
	a->move = move;
	return 0;
}

int get_killer_move(kmoves *km, int ply, int id, MOVESTORE *move)
{

	assert(id < KMOVES_WIDTH);
	assert(ply < MAXPLY);
	*move = (km + ply * KMOVES_WIDTH + id)->move;
	if (*move == NA_MOVE)
		return 0;
	return 1;
}

kmoves* allocateKillerStore(void)
{
	kmoves *m;
	m = (kmoves*) malloc(sizeof(kmoves) * KMOVES_WIDTH * MAXPLY);
	if (m == NULL)
		abort();
	clear_killer_moves(m);
	return m;
}

int freeKillerStore(kmoves *m)
{
	free(m);
	return 1;
}

hashStore* allocateHashStore(size_t hashBytes, unsigned int hashPVLen)
{
	hashStore *hs;

	unsigned int msk = 0;
	size_t hashLen, hl, hp;
	size_t xx;
	int l=0;

	BITVAR *tt;
	
//	hashLen = hashBytes / sizeof(hashEntry_e);
	hashLen = hashBytes / (HASHPOS * sizeof(hashBucket));
// just make sure hashLen is power of 2
	while (msk <= hashLen) {
		msk <<= 1;
		msk |= 1;
		l++;
	}
	msk >>= 1;
	l--;
	msk++;
	LOGGER_0("Bytes: %d, hashEntry %d, HASHLEN %o, msk %o, mask len %d\n", hashBytes, sizeof(hashEntry_e), hashLen, msk, l);
	hashLen = msk;

	xx = (sizeof(hashStore) * 2);
	hs = (hashStore*) aligned_alloc(8,xx);
	
	xx = sizeof(hashBucket) * hashLen * HASHPOS;
	hs->hash = (hashBucket*) aligned_alloc(128,xx);
	hs->hashlen = (size_t) hashLen;
	hs->llen=l;

	xx=sizeof(hashEntryPV_e) * hashPVLen+0xFFU;
	hs->pv = (hashEntryPV_e*) aligned_alloc(128,xx);
	hs->hashPVlen = hashPVLen;

	initHash(hs);
	return hs;
}

hashPawnStore* allocateHashPawnStore(size_t hashBytes)
{
	hashPawnStore *hs;

	size_t hl;
	size_t hashLen;
	int msk = 0;
	int l=0;

	hashLen = hashBytes / sizeof(hashPawnEntry_e);
// just make sure hashLen is power of 2
	while (msk <= hashLen) {
		msk <<= 1;
		msk |= 1;
		l++;
	}
	msk >>= 1;
	l--;
	msk++;
	LOGGER_0("HASHLEN %d, msk %d\n", hashLen, msk);
	hashLen = msk;

	hl = hashLen;
	hs = (hashPawnStore*) aligned_alloc(128,(
		sizeof(hashPawnStore) * 2 + sizeof(hashPawnEntry_e) * hl + 0XFFL)&(~0xFFUL));
	hs->hashlen = hashLen;
	hs->llen=l;
	hs->hash = (hashPawnEntry_e*) (hs + 1);
	initPawnHash(hs);
	return hs;
}

int freeHashStore(hashStore *hs)
{
	free(hs->pv);
	free(hs->hash);
	free(hs);
	return 1;
}

int freeHashPawnStore(hashPawnStore *hs)
{
	free(hs);
	return 1;
}

int generateRandomFile(char *n)
{
	FILE *h;
	BITVAR i1, i2, i3, i4, i5, i6, i7, i8, q;
	int y;

	h = fopen(n, "w");
	q = 0;
	fprintf(h, "BITVAR RANDOMS[]= {\n");
	for (y = 0; y < 249; y++) {
		i1 = getRandomX(&q);
		i2 = getRandomX(&q);
		i3 = getRandomX(&q);
		i4 = getRandomX(&q);
		fprintf(h,"\t\t\t\t0x%016lXULL, 0x%016lXULL, 0x%016lXULL, 0x%016lXULL,\n", i1, i2, i3, i4);
	}
		i1 = getRandomX(&q);
		i2 = getRandomX(&q);
		i3 = getRandomX(&q);
		i4 = getRandomX(&q);
		fprintf(h, "\t\t\t\t0x%016lXULL, 0x%016lXULL, 0x%016lXULL, 0x%016lXULL\n", i1, i2, i3, i4);
	fprintf(h, "\t\t};");
	fclose(h);

	return 0;
}

hashPawnEntry* storePawnHash(hashPawnStore *hs, hashPawnEntry *hash, BITVAR ver, struct _statistics *s)
{
	int i, c;
	BITVAR f, hi;

	s->hashPawnStores++;
	f = hash->key % (BITVAR) hs->hashlen;
	hi = hash->key / (BITVAR) hs->hashlen;

	for (i = 0; i < HASHPAWNPOS; i++) {
		if ((hi == hs->hash[f].e[i].key)) {
// mame nas zaznam
			s->hashPawnStoreHits++;
			s->hashPawnStoreInPlace++;
			c = i;
			goto replace;
		}
	}
	c=0;
	for (i = 0; i < HASHPAWNPOS; i++) {
		if ((hs->hash[f].e[i].age != hs->hashValidId)) {
			c = i;
			break;
		}
	}
//	if (i >= HASHPAWNPOS) c = 0;
	replace: hs->hash[f].e[c].value = hash->value;
	hs->hash[f].e[c].age = (uint8_t) hs->hashValidId;
	hs->hash[f].e[c].key = hi;
	return &(hs->hash[f].e[c]);
}

int invalidatePawnHash(hashPawnStore *hs)
{
	if(hs!=NULL) {
		hs->hashValidId++;
		if (hs->hashValidId > 63)
			hs->hashValidId = 1;
	}
	return 0;
}

int initPawnHash(hashPawnStore *hs)
{
	int f, c;

	for (f = 0; f < hs->hashlen; f++) {
		for (c = 0; c < HASHPAWNPOS; c++) {
			hs->hash[f].e[c].key = 0;
			hs->hash[f].e[c].age = 0;
			hs->hash[f].e[c].map = 0;
		}
	}
	hs->hashValidId = 1;
	return 0;
}

hashPawnEntry* retrievePawnHash(hashPawnStore *hs, hashPawnEntry *hash, BITVAR ver, struct _statistics *s)
{
	int i;
	BITVAR f, hi;
	s->hashPawnAttempts++;
	f = hash->key % (BITVAR) hs->hashlen;
	hi = hash->key / (BITVAR) hs->hashlen;
	for (i = 0; i < HASHPAWNPOS; i++) {
		if ((hs->hash[f].e[i].key == hi)
			&& (hs->hash[f].e[i].age == hs->hashValidId))
			break;
	}
	if (i == HASHPAWNPOS) {
		s->hashPawnMiss++;
		return NULL;
	}
	s->hashPawnHits++;
	return &(hs->hash[f].e[i]);
}

void setupPawnRandom(board *b)
{
	b->pawnkey = getPawnKey(b);
}

BITVAR getPawnKey(board *b)
{
	BITVAR x;
	BITVAR key;
	int from;
	key = 0;
	x = b->colormaps[WHITE] & (b->maps[PAWN]);
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[WHITE][from][b->pieces[from]];
	}
	x = b->colormaps[BLACK] & (b->maps[PAWN]);
	while (x) {
		from = LastOne(x);
		x = ClrNorm(from, x);
		key ^= randomTable[BLACK][from][b->pieces[from] & PIECEMASK];
	}
	return key;
}

hhTable* allocateHHTable(void)
{
	hhTable *hh;
	hh = (hhTable*) malloc(sizeof(hhTable));
	if (hh == NULL)
		abort();
	return hh;
}

int freeHHTable(hhTable *hh)
{
	free(hh);
	return 0;
}

int clearHHTable(hhTable *hh)
{
	int v[ER_PIECE];
	v[PAWN] = P_OR;
	v[KNIGHT] = N_OR;
	v[BISHOP] = B_OR;
	v[ROOK] = R_OR;
	v[QUEEN] = Q_OR;
	v[KING] = K_OR_M;

	int f, q;
	for (q = PAWN; q < ER_PIECE; q++) {
		for (f = 0; f < 64; f++)
			hh->val[0][q][f] = hh->val[1][q][f] = v[q];
	}
	return 0;
}

int updateHHTable(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply)
{
	int fromPos, toPos, piece;
	int upd;

	fromPos = UnPackFrom(m[cutoff].move);
	toPos = UnPackTo(m[cutoff].move);
	piece = b->pieces[fromPos] & PIECEMASK;
	upd=Min(depth*depth, 1600);
	hh->val[side][piece][toPos] += upd;
	return 0;
}

int updateHHTable2(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int bonus)
{
	int fromPos, toPos, piece;
	int upd;

	fromPos = UnPackFrom(m[cutoff].move);
	toPos = UnPackTo(m[cutoff].move);
	piece = b->pieces[fromPos] & PIECEMASK;
	hh->val[side][piece][toPos] += 32*bonus - hh->val[side][piece][toPos] * abs(bonus)/512;
	return 0;
}

int updateHHTableGood(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply){
	return updateHHTable2(b, hh, m, cutoff, side, Min(depth*depth, 400));
}
int updateHHTableBad(board *b, hhTable *hh, move_entry *m, int cutoff, int side, int depth, int ply){
	return updateHHTable2(b, hh, m, cutoff, side, -Min(depth*depth/4, 400));
}

int checkHHTable(hhTable *hh, int side, int piece, int square)
{
	return hh->val[side][piece][square];
}
