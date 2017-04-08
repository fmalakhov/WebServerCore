/*
Copyright (c) 2012-2017 MFBS, Fedor Malakhov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include "gzip.h"

/* ===========================================================================
 * Configuration parameters
 */

/* Compile with MEDIUM_MEM to reduce the memory requirements or
 * with SMALL_MEM to use as little memory as possible. Use BIG_MEM if the
 * entire input file can be held in memory (not possible on 16 bit systems).
 * Warning: defining these symbols affects HASH_BITS (see below) and thus
 * affects the compression ratio. The compressed output
 * is still correct, and might even be smaller in some cases.
 */


/* DECLARE(uch, window, 2L*WSIZE); */
/* Sliding window. Input bytes are read into the second half of the window,
 * and move to the first half later to keep a dictionary of at least WSIZE
 * bytes. With this organization, matches are limited to a distance of
 * WSIZE-MAX_MATCH bytes, but this ensures that IO is always
 * performed with a length multiple of the block size. Also, it limits
 * the window size to 64K, which is quite useful on MSDOS.
 * To do: limit the window size to WSIZE+BSZ if SMALL_MEM (the code would
 * be less efficient).
 */

/* DECLARE(Pos, prev, WSIZE); */
/* Link to older string with same hash index. To limit the size of this
 * array to 64K, this link is maintained only for the last 32K strings.
 * An index in this array is thus a window index modulo 32K.
 */

/* DECLARE(Pos, head, 1<<HASH_BITS); */
/* Heads of the hash chains or NIL. */

ulg window_size = (ulg)2*WSIZE;
/* window size, 2*WSIZE except for MMAP or BIG_MEM, where it is the
 * input file length plus MIN_LOOKAHEAD.
 */

#define H_SHIFT  ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)
/* Number of bits by which ins_h and del_h must be shifted at each
 * input step. It must be such that after MIN_MATCH steps, the oldest
 * byte no longer takes part in the hash key, that is:
 *   H_SHIFT * MIN_MATCH >= HASH_BITS
 */

/* Insert new strings in the hash table only if the match length
 * is not greater than this length. This saves time but degrades compression.
 * max_insert_length is used only for compression levels <= 3.
 */

config configuration_table[10] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0},  /* store only */
/* 1 */ {4,    4,  8,    4},  /* maximum speed, no lazy matches */
/* 2 */ {4,    5, 16,    8},
/* 3 */ {4,    6, 32,   32},

/* 4 */ {4,    4, 16,   16},  /* lazy matches */
/* 5 */ {8,   16, 32,   32},
/* 6 */ {8,   16, 128, 128},
/* 7 */ {8,   32, 128, 256},
/* 8 */ {32, 128, 258, 1024},
/* 9 */ {32, 258, 258, 4096}}; /* maximum compression */

/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
 * meaning.
 */

#define EQUAL 0
/* result of memcmp for equal strings */

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

/* ===========================================================================
 * Insert string s in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of s are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#define INSERT_STRING(s, match_head) \
   (UPDATE_HASH(ZipInfoPtr->ins_h, ZipInfoPtr->window[(s) + MIN_MATCH-1]), \
    ZipInfoPtr->prev[(s) & WMASK] = match_head = (ZipInfoPtr->prev+WSIZE)[ZipInfoPtr->ins_h], \
    (ZipInfoPtr->prev+WSIZE)[ZipInfoPtr->ins_h] = (s))
/* ===========================================================================
 * Initialize the "longest match" routines for a new file
 */
void lm_init (GZIP_TASK_INFO *ZipInfoPtr, int pack_level, ush *flags)
/* pack_level -  0: store, 1: best speed, 9: best compression
   flags      - general purpose bit flag */
{
    register unsigned j;

    if (pack_level < 1 || pack_level > 9) error("bad pack level");
    ZipInfoPtr->compr_level = pack_level;

    /* Initialize the hash table. */
#if defined(MAXSEG_64K) && HASH_BITS == 15
    for (j = 0;  j < HASH_SIZE; j++) head[j] = NIL;
#else
    memzero((char*)(ZipInfoPtr->prev+WSIZE), HASH_SIZE*sizeof(*(ZipInfoPtr->prev+WSIZE)));
#endif
    /* prev will be initialized on the fly */

    /* Set the default configuration parameters:
     */
    ZipInfoPtr->max_lazy_match   = configuration_table[pack_level].max_lazy;
    ZipInfoPtr->good_match       = configuration_table[pack_level].good_length;
#ifndef FULL_SEARCH
    ZipInfoPtr->nice_match       = configuration_table[pack_level].nice_length;
#endif
    ZipInfoPtr->max_chain_length = configuration_table[pack_level].max_chain;
    if (pack_level == 1) {
       *flags |= FAST;
    } else if (pack_level == 9) {
       *flags |= SLOW;
    }
    /* ??? reduce max_chain_length for binary files */

    ZipInfoPtr->strstart = 0;
    ZipInfoPtr->block_start = 0L;
    ZipInfoPtr->lookahead = read_buf(ZipInfoPtr, (char*)ZipInfoPtr->window,
			 sizeof(int) <= 2 ? (unsigned)WSIZE : 2*WSIZE);

    if (ZipInfoPtr->lookahead == 0 || ZipInfoPtr->lookahead == (unsigned)EOF) {
       ZipInfoPtr->eofile = 1, ZipInfoPtr->lookahead = 0;
       return;
    }
    ZipInfoPtr->eofile = 0;
    /* Make sure that we always have enough lookahead. This is important
     * if input comes from a device such as a tty.
     */
    while (ZipInfoPtr->lookahead < MIN_LOOKAHEAD && !ZipInfoPtr->eofile) fill_window(ZipInfoPtr);

    ZipInfoPtr->ins_h = 0;
    for (j=0; j<MIN_MATCH-1; j++) UPDATE_HASH(ZipInfoPtr->ins_h, ZipInfoPtr->window[j]);
    /* If lookahead < MIN_MATCH, ins_h is garbage, but this is
     * not important since only literal bytes will be emitted.
     */
}

/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 */
#ifndef ASMV
/* For MSDOS, OS/2 and 386 Unix, an optimized version is in match.asm or
 * match.s. The code is functionally equivalent, so you can use the C version
 * if desired.
 */
int longest_match(GZIP_TASK_INFO *ZipInfoPtr, IPos cur_match)
    /* cur_match - current match */
{
    unsigned chain_length = ZipInfoPtr->max_chain_length;   /* max hash chain length */
    register uch *scan = ZipInfoPtr->window + ZipInfoPtr->strstart;     /* current string */
    register uch *match;                        /* matched string */
    register int len;                           /* length of current match */
    int best_len = ZipInfoPtr->prev_length;     /* best match length so far */
    IPos limit = ZipInfoPtr->strstart > (IPos)MAX_DIST ? ZipInfoPtr->strstart - (IPos)MAX_DIST : NIL;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */

/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
 * It is easy to get rid of this optimization if necessary.
 */
#if HASH_BITS < 8 || MAX_MATCH != 258
   error: Code too clever
#endif

#ifdef UNALIGNED_OK
    /* Compare two bytes at a time. Note: this is not always beneficial.
     * Try with and without -DUNALIGNED_OK to check.
     */
    register uch *strend = ZipInfoPtr->window + ZipInfoPtr->strstart + MAX_MATCH - 1;
    register ush scan_start = *(ush*)scan;
    register ush scan_end   = *(ush*)(scan+best_len-1);
#else
    register uch *strend = ZipInfoPtr->window + ZipInfoPtr->strstart + MAX_MATCH;
    register uch scan_end1  = scan[best_len-1];
    register uch scan_end   = scan[best_len];
#endif

    /* Do not waste too much time if we already have a good match: */
    if (ZipInfoPtr->prev_length >= ZipInfoPtr->good_match) {
        chain_length >>= 2;
    }
    Assert(strstart <= window_size-MIN_LOOKAHEAD, "insufficient lookahead");

    do {
        Assert(cur_match < strstart, "no future");
        match = ZipInfoPtr->window + cur_match;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2:
         */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
        /* This code assumes sizeof(unsigned short) == 2. Do not use
         * UNALIGNED_OK if your compiler uses a different size.
         */
        if (*(ush*)(match+best_len-1) != scan_end ||
            *(ush*)match != scan_start) continue;

        /* It is not necessary to compare scan[2] and match[2] since they are
         * always equal when the other bytes match, given that the hash keys
         * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
         * strstart+3, +5, ... up to strstart+257. We check for insufficient
         * lookahead only every 4th comparison; the 128th check will be made
         * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
         * necessary to put more guard bytes at the end of the window, or
         * to check more often for insufficient lookahead.
         */
        scan++, match++;
        do {
        } while (*(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 scan < strend);
        /* The funny "do {}" generates better code on most compilers */

        /* Here, scan <= window+strstart+257 */
        Assert(scan <= window+(unsigned)(window_size-1), "wild scan");
        if (*scan == *match) scan++;

        len = (MAX_MATCH - 1) - (int)(strend-scan);
        scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

        if (match[best_len]   != scan_end  ||
            match[best_len-1] != scan_end1 ||
            *match            != *scan     ||
            *++match          != scan[1])      continue;

        /* The check at best_len-1 can be removed because it will be made
         * again later. (This heuristic is not always a win.)
         * It is not necessary to compare scan[2] and match[2] since they
         * are always equal when the other bytes match, given that
         * the hash keys are equal and that HASH_BITS >= 8.
         */
        scan += 2, match++;

        /* We check for insufficient lookahead only every 8th comparison;
         * the 256th check will be made at strstart+258.
         */
        do {
        } while (*++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 scan < strend);

        len = MAX_MATCH - (int)(strend - scan);
        scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

        if (len > best_len) {
            ZipInfoPtr->match_start = cur_match;
            best_len = len;
            if (len >= ZipInfoPtr->nice_match) break;
#ifdef UNALIGNED_OK
            scan_end = *(ush*)(scan+best_len-1);
#else
            scan_end1  = scan[best_len-1];
            scan_end   = scan[best_len];
#endif
        }
    } while ((cur_match = ZipInfoPtr->prev[cur_match & WMASK]) > limit
	     && --chain_length != 0);

    return best_len;
}
#endif /* ASMV */

#ifdef DEBUG
/* ===========================================================================
 * Check that the match at match_start is indeed a match.
 */
local void check_match(start, match, length)
    IPos start, match;
    int length;
{
    /* check that the match is indeed a match */
    if (memcmp((char*)ZipInfoPtr->window + match,
                (char*)ZipInfoPtr->window + start, length) != EQUAL) {
        fprintf(stderr,
            " start %d, match %d, length %d\n",
            start, match, length);
        error("invalid match");
    }
    if (verbose > 1) {
        fprintf(stderr,"\\[%d,%d]", start-match, length);
        do { putc(ZipInfoPtr->window[start++], stderr); } while (--length != 0);
    }
}
#else
#  define check_match(start, match, length)
#endif

/* ===========================================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead, and sets eofile if end of input file.
 * IN assertion: lookahead < MIN_LOOKAHEAD && strstart + lookahead > 0
 * OUT assertions: at least one byte has been read, or eofile is set;
 *    file reads are performed for at least two bytes (required for the
 *    translate_eol option).
 */
void fill_window(GZIP_TASK_INFO *ZipInfoPtr)
{
    register unsigned n, m;
    unsigned more = (unsigned)(window_size - (ulg)ZipInfoPtr->lookahead - (ulg)ZipInfoPtr->strstart);
    /* Amount of free space at the end of the window. */

    /* If the window is almost full and there is insufficient lookahead,
     * move the upper half to the lower one to make room in the upper half.
     */
    if (more == (unsigned)EOF) {
        /* Very unlikely, but possible on 16 bit machine if strstart == 0
         * and lookahead == 1 (input done one byte at time)
         */
        more--;
    } else if (ZipInfoPtr->strstart >= WSIZE+MAX_DIST) {
        /* By the IN assertion, the window is not empty so we can't confuse
         * more == 0 with more == 64K on a 16 bit machine.
         */
        Assert(window_size == (ulg)2*WSIZE, "no sliding with BIG_MEM");

        memcpy((char*)ZipInfoPtr->window, (char*)ZipInfoPtr->window+WSIZE, (unsigned)WSIZE);
        ZipInfoPtr->match_start -= WSIZE;
        ZipInfoPtr->strstart    -= WSIZE; /* we now have strstart >= MAX_DIST: */

        ZipInfoPtr->block_start -= (long) WSIZE;

        for (n = 0; n < HASH_SIZE; n++) {
            m = (ZipInfoPtr->prev+WSIZE)[n];
            (ZipInfoPtr->prev+WSIZE)[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
        }
        for (n = 0; n < WSIZE; n++) {
            m = ZipInfoPtr->prev[n];
            ZipInfoPtr->prev[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
            /* If n is not on any hash chain, prev[n] is garbage but
             * its value will never be used.
             */
        }
        more += WSIZE;
    }
    /* At this point, more >= 2 */
    if (!ZipInfoPtr->eofile)
    {
        n = read_buf(ZipInfoPtr, (char*)ZipInfoPtr->window+ZipInfoPtr->strstart+ZipInfoPtr->lookahead, more);
        if (n == 0 || n == (unsigned)EOF) 
        {
            ZipInfoPtr->eofile = 1;
        } 
        else 
        {
            ZipInfoPtr->lookahead += n;
        }
    }
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: strstart is set to the end of the current match.
 */
#define FLUSH_BLOCK(eof) \
   flush_block(ZipInfoPtr, ZipInfoPtr->block_start >= 0L ? (char*)&ZipInfoPtr->window[(unsigned)ZipInfoPtr->block_start] : \
                (char*)NULL, (long)ZipInfoPtr->strstart - ZipInfoPtr->block_start, (eof))

/* ===========================================================================
 * Processes a new input file and return its compressed length. This
 * function does not perform lazy evaluationof matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options.
 */
ulg deflate_fast(GZIP_TASK_INFO *ZipInfoPtr)
{
    IPos hash_head; /* head of the hash chain */
    int flush;      /* set if current block must be flushed */
    unsigned match_length = 0;  /* length of best match */

    ZipInfoPtr->prev_length = MIN_MATCH-1;
    while (ZipInfoPtr->lookahead != 0) {
        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        INSERT_STRING(ZipInfoPtr->strstart, hash_head);

        /* Find the longest match, discarding those <= prev_length.
         * At this point we have always match_length < MIN_MATCH
         */
        if (hash_head != NIL && ZipInfoPtr->strstart - hash_head <= MAX_DIST) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            match_length = longest_match (ZipInfoPtr, hash_head);
            /* longest_match() sets match_start */
            if (match_length > ZipInfoPtr->lookahead) match_length = ZipInfoPtr->lookahead;
        }
        if (match_length >= MIN_MATCH) {
            check_match(strstart, match_start, match_length);

            flush = ct_tally(ZipInfoPtr, ZipInfoPtr->strstart-ZipInfoPtr->match_start, match_length - MIN_MATCH);

            ZipInfoPtr->lookahead -= match_length;

	    /* Insert new strings in the hash table only if the match length
             * is not too large. This saves time but degrades compression.
             */
            if (match_length <= ZipInfoPtr->max_lazy_match) {
                match_length--; /* string at strstart already in hash table */
                do {
                    ZipInfoPtr->strstart++;
                    INSERT_STRING(ZipInfoPtr->strstart, hash_head);
                    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                     * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                     * these bytes are garbage, but it does not matter since
                     * the next lookahead bytes will be emitted as literals.
                     */
                } while (--match_length != 0);
	        ZipInfoPtr->strstart++; 
            } else {
	        ZipInfoPtr->strstart += match_length;
	        match_length = 0;
	        ZipInfoPtr->ins_h = ZipInfoPtr->window[ZipInfoPtr->strstart];
	        UPDATE_HASH(ZipInfoPtr->ins_h, ZipInfoPtr->window[ZipInfoPtr->strstart+1]);
#if MIN_MATCH != 3
                Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
            }
        } else {
            /* No match, output a literal byte */
            Tracevv((stderr,"%c", ZipInfoPtr->window[strstart]));
            flush = ct_tally (ZipInfoPtr, 0, ZipInfoPtr->window[ZipInfoPtr->strstart]);
            ZipInfoPtr->lookahead--;
	    ZipInfoPtr->strstart++; 
        }
        if (flush) FLUSH_BLOCK(0), ZipInfoPtr->block_start = ZipInfoPtr->strstart;

        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        while (ZipInfoPtr->lookahead < MIN_LOOKAHEAD && !ZipInfoPtr->eofile) fill_window(ZipInfoPtr);

    }
    return FLUSH_BLOCK(1); /* eof */
}

/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
ulg deflate(GZIP_TASK_INFO *ZipInfoPtr)
{
    IPos hash_head;          /* head of hash chain */
    IPos prev_match;         /* previous match */
    int flush;               /* set if current block must be flushed */
    int match_available = 0; /* set if previous match exists */
    register unsigned match_length = MIN_MATCH-1; /* length of best match */

    if (ZipInfoPtr->compr_level <= 3) return deflate_fast(ZipInfoPtr); /* optimized for speed */

    /* Process the input block. */
    while (ZipInfoPtr->lookahead != 0) {
        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        INSERT_STRING(ZipInfoPtr->strstart, hash_head);

        /* Find the longest match, discarding those <= prev_length.
         */
        ZipInfoPtr->prev_length = match_length, prev_match = ZipInfoPtr->match_start;
        match_length = MIN_MATCH-1;

        if (hash_head != NIL && ZipInfoPtr->prev_length < ZipInfoPtr->max_lazy_match &&
            ZipInfoPtr->strstart - hash_head <= MAX_DIST) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            match_length = longest_match(ZipInfoPtr, hash_head);
            /* longest_match() sets match_start */
            if (match_length > ZipInfoPtr->lookahead) match_length = ZipInfoPtr->lookahead;

            /* Ignore a length 3 match if it is too distant: */
            if (match_length == MIN_MATCH && ZipInfoPtr->strstart-ZipInfoPtr->match_start > TOO_FAR){
                /* If prev_match is also MIN_MATCH, match_start is garbage
                 * but we will ignore the current match anyway.
                 */
                match_length--;
            }
        }
        /* If there was a match at the previous step and the current
         * match is not better, output the previous match:
         */
        if (ZipInfoPtr->prev_length >= MIN_MATCH && match_length <= ZipInfoPtr->prev_length) {

            check_match(strstart-1, prev_match, prev_length);

            flush = ct_tally(ZipInfoPtr, ZipInfoPtr->strstart-1-prev_match, ZipInfoPtr->prev_length - MIN_MATCH);

            /* Insert in hash table all strings up to the end of the match.
             * strstart-1 and strstart are already inserted.
             */
            ZipInfoPtr->lookahead -= ZipInfoPtr->prev_length-1;
            ZipInfoPtr->prev_length -= 2;
            do {
                ZipInfoPtr->strstart++;
                INSERT_STRING(ZipInfoPtr->strstart, hash_head);
                /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                 * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                 * these bytes are garbage, but it does not matter since the
                 * next lookahead bytes will always be emitted as literals.
                 */
            } while (--ZipInfoPtr->prev_length != 0);
            match_available = 0;
            match_length = MIN_MATCH-1;
            ZipInfoPtr->strstart++;
            if (flush) FLUSH_BLOCK(0), ZipInfoPtr->block_start = ZipInfoPtr->strstart;

        } else if (match_available) {
            /* If there was no match at the previous position, output a
             * single literal. If there was a match but the current match
             * is longer, truncate the previous match to a single literal.
             */
            Tracevv((stderr,"%c", ZipInfoPtr->window[ZipInfoPtr->strstart-1]));
            if (ct_tally (ZipInfoPtr, 0, ZipInfoPtr->window[ZipInfoPtr->strstart-1])) {
                FLUSH_BLOCK(0), ZipInfoPtr->block_start = ZipInfoPtr->strstart;
            }
            ZipInfoPtr->strstart++;
            ZipInfoPtr->lookahead--;
        } else {
            /* There is no previous match to compare with, wait for
             * the next step to decide.
             */
            match_available = 1;
            ZipInfoPtr->strstart++;
            ZipInfoPtr->lookahead--;
        }
        Assert (strstart <= ZipInfoPtr->bytes_in && lookahead <= ZipInfoPtr->bytes_in, "a bit too far");

        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        while (ZipInfoPtr->lookahead < MIN_LOOKAHEAD && !ZipInfoPtr->eofile) fill_window(ZipInfoPtr);
    }
    if (match_available) ct_tally (ZipInfoPtr, 0, ZipInfoPtr->window[ZipInfoPtr->strstart-1]);

    return FLUSH_BLOCK(1); /* eof */
}
//---------------------------------------------------------------------------

