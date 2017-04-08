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

#include <ctype.h>
#include "gzip.h"

/* ===========================================================================
 * Constants
 */

int extra_lbits[LENGTH_CODES] /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};

int extra_dbits[D_CODES] /* extra bits for each distance code */
   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

int extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

uch bl_order[BL_CODES] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */

//    ct_data static_dtree[D_CODES];

#ifdef DEBUG
extern ulg bits_sent;  /* bit length of the compressed data */
extern long isize;     /* byte length of input file */
#endif

/* ===========================================================================
 * Local (static) routines in this file.
 */


#ifndef DEBUG
#  define send_code(c, tree) send_bits(ZipInfoPtr, tree[c].Code, tree[c].Len)
   /* Send a code of the given tree. c and tree must not have side effects */

#else /* DEBUG */
#  define send_code(c, tree) \
     { if (verbose>1) fprintf(stderr,"\ncd %3d ",(c)); \
       send_bits(ZipInfoPtr, tree[c].Code, tree[c].Len); }
#endif

#define d_code(dist) \
   ((dist) < 256 ? ZipInfoPtr->dist_code[dist] : ZipInfoPtr->dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. dist_code[256] and dist_code[257] are never
 * used.
 */

#define MAX(a,b) (a >= b ? a : b)
/* the arguments must not have side effects */

/* ===========================================================================
 * Allocate the match buffer, initialize the various tables and save the
 * location of the internal file attribute (ascii/binary) and method
 * (DEFLATE/STORE).
 */
void ct_init(GZIP_TASK_INFO *ZipInfoPtr, ush  *attr, int  *methodp)
/* attr    - pointer to internal file attribute
   methodp - pointer to compression method */
{
    int n;        /* iterates over tree elements */
    int bits;     /* bit counter */
    int length;   /* length value */
    int code;     /* code value */
    int dist;     /* distance index */

    ZipInfoPtr->file_type = attr;
    ZipInfoPtr->file_method = methodp;
    ZipInfoPtr->compressed_len = 0L;
	ZipInfoPtr->input_len = 0L;
        
    if (ZipInfoPtr->static_dtree[0].Len != 0) return; /* ct_init already called */

    /* Initialize the mapping length (0..255) -> length code (0..28) */
    length = 0;
    for (code = 0; code < LENGTH_CODES-1; code++) {
        ZipInfoPtr->base_length[code] = length;
        for (n = 0; n < (1<<extra_lbits[code]); n++) {
            ZipInfoPtr->length_code[length++] = (uch)code;
        }
    }
    Assert (length == 256, "ct_init: length != 256");
    /* Note that the length 255 (match length 258) can be represented
     * in two different ways: code 284 + 5 bits or code 285, so we
     * overwrite length_code[255] to use the best encoding:
     */
    ZipInfoPtr->length_code[length-1] = (uch)code;

    /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
    dist = 0;
    for (code = 0 ; code < 16; code++) {
        ZipInfoPtr->base_dist[code] = dist;
        for (n = 0; n < (1<<extra_dbits[code]); n++) {
            ZipInfoPtr->dist_code[dist++] = (uch)code;
        }
    }
    Assert (dist == 256, "ct_init: dist != 256");
    dist >>= 7; /* from now on, all distances are divided by 128 */
    for ( ; code < D_CODES; code++) {
        ZipInfoPtr->base_dist[code] = dist << 7;
        for (n = 0; n < (1<<(extra_dbits[code]-7)); n++) {
            ZipInfoPtr->dist_code[256 + dist++] = (uch)code;
        }
    }
    Assert (dist == 256, "ct_init: 256+dist != 512");

    /* Construct the codes of the static literal tree */
    for (bits = 0; bits <= MAX_BITS; bits++) ZipInfoPtr->bl_count[bits] = 0;
    n = 0;
    while (n <= 143) ZipInfoPtr->static_ltree[n++].Len = 8, ZipInfoPtr->bl_count[8]++;
    while (n <= 255) ZipInfoPtr->static_ltree[n++].Len = 9, ZipInfoPtr->bl_count[9]++;
    while (n <= 279) ZipInfoPtr->static_ltree[n++].Len = 7, ZipInfoPtr->bl_count[7]++;
    while (n <= 287) ZipInfoPtr->static_ltree[n++].Len = 8, ZipInfoPtr->bl_count[8]++;
    /* Codes 286 and 287 do not exist, but we must include them in the
     * tree construction to get a canonical Huffman tree (longest code
     * all ones)
     */
    gen_codes(ZipInfoPtr, (ct_data *)ZipInfoPtr->static_ltree, L_CODES+1);

    /* The static distance tree is trivial: */
    for (n = 0; n < D_CODES; n++) {
        ZipInfoPtr->static_dtree[n].Len = 5;
        ZipInfoPtr->static_dtree[n].Code = bi_reverse(n, 5);
    }

    /* Initialize the first block of the first file: */
    init_block(ZipInfoPtr);
}

/* ===========================================================================
 * Initialize a new block.
 */
void init_block(GZIP_TASK_INFO *ZipInfoPtr)
{
    int n; /* iterates over tree elements */

    /* Initialize the trees. */
    for (n = 0; n < L_CODES;  n++) ZipInfoPtr->dyn_ltree[n].Freq = 0;
    for (n = 0; n < D_CODES;  n++) ZipInfoPtr->dyn_dtree[n].Freq = 0;
    for (n = 0; n < BL_CODES; n++) ZipInfoPtr->bl_tree[n].Freq = 0;

    ZipInfoPtr->dyn_ltree[END_BLOCK].Freq = 1;
    ZipInfoPtr->opt_len = 0L;
	ZipInfoPtr->static_len = 0L;
    ZipInfoPtr->last_lit = 0;
	ZipInfoPtr->last_dist = 0;
	ZipInfoPtr->last_flags = 0;
    ZipInfoPtr->flags = 0;
	ZipInfoPtr->flag_bit = 1;
}

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */


/* ===========================================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len.
 */
#define pqremove(tree, top) \
{\
    top = ZipInfoPtr->heap[SMALLEST]; \
    ZipInfoPtr->heap[SMALLEST] = ZipInfoPtr->heap[ZipInfoPtr->heap_len--]; \
    pqdownheap(ZipInfoPtr, tree, SMALLEST); \
}

/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define smaller(tree, n, m) \
   (tree[n].Freq < tree[m].Freq || \
   (tree[n].Freq == tree[m].Freq && ZipInfoPtr->depth[n] <= ZipInfoPtr->depth[m]))

/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
void pqdownheap(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int k)
/* tree -  the tree to restore
   k    -  node to move down */
{
    int v = ZipInfoPtr->heap[k];
    int j = k << 1;  /* left son of k */
    while (j <= ZipInfoPtr->heap_len) {
        /* Set j to the smallest of the two sons: */
        if (j < ZipInfoPtr->heap_len && smaller(tree, ZipInfoPtr->heap[j+1], ZipInfoPtr->heap[j])) j++;

        /* Exit if v is smaller than both sons */
        if (smaller(tree, v, ZipInfoPtr->heap[j])) break;

        /* Exchange v with the smallest son */
        ZipInfoPtr->heap[k] = ZipInfoPtr->heap[j];  k = j;

        /* And continue down the tree, setting j to the left son of k */
        j <<= 1;
    }
    ZipInfoPtr->heap[k] = v;
}

/* ===========================================================================
 * Compute the optimal bit lengths for a tree and update the total bit length
 * for the current block.
 * IN assertion: the fields freq and dad are set, heap[heap_max] and
 *    above are the tree nodes sorted by increasing frequency.
 * OUT assertions: the field len is set to the optimal bit length, the
 *     array bl_count contains the frequencies for each bit length.
 *     The length opt_len is updated; static_len is also updated if stree is
 *     not null.
 */
void gen_bitlen(GZIP_TASK_INFO *ZipInfoPtr, tree_desc *desc)
/* desc -  the tree descriptor */
{
    ct_data *tree  = desc->dyn_tree;
    int *extra     = desc->extra_bits;
    int base            = desc->extra_base;
    int max_code        = desc->max_code;
    int max_length      = desc->max_length;
    ct_data *stree = desc->static_tree;
    int h;              /* heap index */
    int n, m;           /* iterate over the tree elements */
    int bits;           /* bit length */
    int xbits;          /* extra bits */
    ush f;              /* frequency */
    int overflow = 0;   /* number of elements with bit length too large */

    for (bits = 0; bits <= MAX_BITS; bits++) ZipInfoPtr->bl_count[bits] = 0;

    /* In a first pass, compute the optimal bit lengths (which may
     * overflow in the case of the bit length tree).
     */
    tree[ZipInfoPtr->heap[ZipInfoPtr->heap_max]].Len = 0; /* root of the heap */

    for (h = ZipInfoPtr->heap_max+1; h < HEAP_SIZE; h++) {
        n = ZipInfoPtr->heap[h];
        bits = tree[tree[n].Dad].Len + 1;
        if (bits > max_length) bits = max_length, overflow++;
        tree[n].Len = (ush)bits;
        /* We overwrite tree[n].Dad which is no longer needed */

        if (n > max_code) continue; /* not a leaf node */

        ZipInfoPtr->bl_count[bits]++;
        xbits = 0;
        if (n >= base) xbits = extra[n-base];
        f = tree[n].Freq;
        ZipInfoPtr->opt_len += (ulg)f * (bits + xbits);
        if (stree) ZipInfoPtr->static_len += (ulg)f * (stree[n].Len + xbits);
    }
    if (overflow == 0) return;

    Trace((stderr,"\nbit length overflow\n"));
    /* This happens for example on obj2 and pic of the Calgary corpus */

    /* Find the first bit length which could increase: */
    do {
        bits = max_length-1;
        while (ZipInfoPtr->bl_count[bits] == 0) bits--;
        ZipInfoPtr->bl_count[bits]--;      /* move one leaf down the tree */
        ZipInfoPtr->bl_count[bits+1] += 2; /* move one overflow item as its brother */
        ZipInfoPtr->bl_count[max_length]--;
        /* The brother of the overflow item also moves one step up,
         * but this does not affect bl_count[max_length]
         */
        overflow -= 2;
    } while (overflow > 0);

    /* Now recompute all bit lengths, scanning in increasing frequency.
     * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
     * lengths instead of fixing only the wrong ones. This idea is taken
     * from 'ar' written by Haruhiko Okumura.)
     */
    for (bits = max_length; bits != 0; bits--) {
        n = ZipInfoPtr->bl_count[bits];
        while (n != 0) {
            m = ZipInfoPtr->heap[--h];
            if (m > max_code) continue;
            if (tree[m].Len != (unsigned) bits) {
                Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
                ZipInfoPtr->opt_len += ((long)bits-(long)tree[m].Len)*(long)tree[m].Freq;
                tree[m].Len = (ush)bits;
            }
            n--;
        }
    }
}

/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
void gen_codes(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int max_code)
/* tree     -  the tree to decorate
   max_code - largest code with non zero frequency */
{
    ush next_code[MAX_BITS+1]; /* next code value for each bit length */
    ush code = 0;              /* running code value */
    int bits;                  /* bit index */
    int n;                     /* code index */

    /* The distribution counts are first used to generate the code values
     * without bit reversal.
     */
    for (bits = 1; bits <= MAX_BITS; bits++) {
        next_code[bits] = code = (code + ZipInfoPtr->bl_count[bits-1]) << 1;
    }
    /* Check that the bit counts in bl_count are consistent. The last code
     * must be all ones.
     */
    Assert (code + bl_count[MAX_BITS]-1 == (1<<MAX_BITS)-1,
            "inconsistent bit counts");
    Tracev((stderr,"\ngen_codes: max_code %d ", max_code));

    for (n = 0;  n <= max_code; n++) {
        int len = tree[n].Len;
        if (len == 0) continue;
        /* Now reverse the bits */
        tree[n].Code = bi_reverse(next_code[len]++, len);

        Tracec(tree != static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
             n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
    }
}

/* ===========================================================================
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
void build_tree(GZIP_TASK_INFO *ZipInfoPtr, tree_desc *desc)
/*desc - the tree descriptor */
{
    ct_data *tree   = desc->dyn_tree;
    ct_data *stree  = desc->static_tree;
    int elems            = desc->elems;
    int n, m;          /* iterate over heap elements */
    int max_code = -1; /* largest code with non zero frequency */
    int node = elems;  /* next internal node of the tree */

    /* Construct the initial heap, with least frequent element in
     * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
     * heap[0] is not used.
     */
    ZipInfoPtr->heap_len = 0, ZipInfoPtr->heap_max = HEAP_SIZE;

    for (n = 0; n < elems; n++) {
        if (tree[n].Freq != 0) {
            ZipInfoPtr->heap[++ZipInfoPtr->heap_len] = max_code = n;
            ZipInfoPtr->depth[n] = 0;
        } else {
            tree[n].Len = 0;
        }
    }

    /* The pkzip format requires that at least one distance code exists,
     * and that at least one bit should be sent even if there is only one
     * possible code. So to avoid special checks later on we force at least
     * two codes of non zero frequency.
     */
    while (ZipInfoPtr->heap_len < 2) {
        int new = ZipInfoPtr->heap[++ZipInfoPtr->heap_len] = (max_code < 2 ? ++max_code : 0);
        tree[new].Freq = 1;
        ZipInfoPtr->depth[new] = 0;
        ZipInfoPtr->opt_len--; if (stree) ZipInfoPtr->static_len -= stree[new].Len;
        /* new is 0 or 1 so it does not have extra bits */
    }
    desc->max_code = max_code;

    /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
     * establish sub-heaps of increasing lengths:
     */
    for (n = ZipInfoPtr->heap_len/2; n >= 1; n--) pqdownheap(ZipInfoPtr, tree, n);

    /* Construct the Huffman tree by repeatedly combining the least two
     * frequent nodes.
     */
    do {
        pqremove(tree, n);   /* n = node of least frequency */
        m = ZipInfoPtr->heap[SMALLEST];  /* m = node of next least frequency */

        ZipInfoPtr->heap[--ZipInfoPtr->heap_max] = n; /* keep the nodes sorted by frequency */
        ZipInfoPtr->heap[--ZipInfoPtr->heap_max] = m;

        /* Create a new node father of n and m */
        tree[node].Freq = tree[n].Freq + tree[m].Freq;
        ZipInfoPtr->depth[node] = (uch) (MAX(ZipInfoPtr->depth[n], ZipInfoPtr->depth[m]) + 1);
        tree[n].Dad = tree[m].Dad = (ush)node;
#ifdef DUMP_BL_TREE
        if (tree == bl_tree) {
            fprintf(stderr,"\nnode %d(%d), sons %d(%d) %d(%d)",
                    node, tree[node].Freq, n, tree[n].Freq, m, tree[m].Freq);
        }
#endif
        /* and insert the new node in the heap */
        ZipInfoPtr->heap[SMALLEST] = node++;
        pqdownheap(ZipInfoPtr, tree, SMALLEST);

    } while (ZipInfoPtr->heap_len >= 2);

    ZipInfoPtr->heap[--ZipInfoPtr->heap_max] = ZipInfoPtr->heap[SMALLEST];

    /* At this point, the fields freq and dad are set. We can now
     * generate the bit lengths.
     */
    gen_bitlen(ZipInfoPtr, (tree_desc *)desc);

    /* The field len is now set, we can generate the bit codes */
    gen_codes(ZipInfoPtr, (ct_data *)tree, max_code);
}

/* ===========================================================================
 * Scan a literal or distance tree to determine the frequencies of the codes
 * in the bit length tree. Updates opt_len to take into account the repeat
 * counts. (The contribution of the bit length codes will be added later
 * during the construction of bl_tree.)
 */
void scan_tree(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int max_code)
/* tree      - the tree to be scanned
   max_code  - and its largest code of non zero frequency */
{
    int n;                     /* iterates over all tree elements */
    int prevlen = -1;          /* last emitted length */
    int curlen;                /* length of current code */
    int nextlen = tree[0].Len; /* length of next code */
    int count = 0;             /* repeat count of the current code */
    int max_count = 7;         /* max repeat count */
    int min_count = 4;         /* min repeat count */

    if (nextlen == 0) max_count = 138, min_count = 3;
    tree[max_code+1].Len = (ush)0xffff; /* guard */

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            ZipInfoPtr->bl_tree[curlen].Freq += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) ZipInfoPtr->bl_tree[curlen].Freq++;
            ZipInfoPtr->bl_tree[REP_3_6].Freq++;
        } else if (count <= 10) {
            ZipInfoPtr->bl_tree[REPZ_3_10].Freq++;
        } else {
            ZipInfoPtr->bl_tree[REPZ_11_138].Freq++;
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}

/* ===========================================================================
 * Send a literal or distance tree in compressed form, using the codes in
 * bl_tree.
 */
void send_tree(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int max_code)
/* tree     - the tree to be scanned
   max_code - and its largest code of non zero frequency */
{
    int n;                     /* iterates over all tree elements */
    int prevlen = -1;          /* last emitted length */
    int curlen;                /* length of current code */
    int nextlen = tree[0].Len; /* length of next code */
    int count = 0;             /* repeat count of the current code */
    int max_count = 7;         /* max repeat count */
    int min_count = 4;         /* min repeat count */

    /* tree[max_code+1].Len = -1; */  /* guard already set */
    if (nextlen == 0) max_count = 138, min_count = 3;

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            do { send_code(curlen, ZipInfoPtr->bl_tree); } while (--count != 0);

        } else if (curlen != 0) {
            if (curlen != prevlen) {
                send_code(curlen, ZipInfoPtr->bl_tree); count--;
            }
            Assert(count >= 3 && count <= 6, " 3_6?");
            send_code(REP_3_6, ZipInfoPtr->bl_tree); send_bits(ZipInfoPtr, count-3, 2);

        } else if (count <= 10) {
            send_code(REPZ_3_10, ZipInfoPtr->bl_tree); send_bits(ZipInfoPtr, count-3, 3);

        } else {
            send_code(REPZ_11_138, ZipInfoPtr->bl_tree); send_bits(ZipInfoPtr, count-11, 7);
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}

/* ===========================================================================
 * Construct the Huffman tree for the bit lengths and return the index in
 * bl_order of the last bit length code to send.
 */
int build_bl_tree(GZIP_TASK_INFO *ZipInfoPtr)
{
    int max_blindex;  /* index of last bit length code of non zero freq */

    /* Determine the bit length frequencies for literal and distance trees */
    scan_tree(ZipInfoPtr, (ct_data *)ZipInfoPtr->dyn_ltree, ZipInfoPtr->l_desc.max_code);
    scan_tree(ZipInfoPtr, (ct_data *)ZipInfoPtr->dyn_dtree, ZipInfoPtr->d_desc.max_code);

    /* Build the bit length tree: */
    build_tree(ZipInfoPtr, (tree_desc *)(&ZipInfoPtr->bl_desc));
    /* opt_len now includes the length of the tree representations, except
     * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
     */

    /* Determine the number of bit length codes to send. The pkzip format
     * requires that at least 4 bit length codes be sent. (appnote.txt says
     * 3 but the actual value used is 4.)
     */
    for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
        if (ZipInfoPtr->bl_tree[bl_order[max_blindex]].Len != 0) break;
    }
    /* Update opt_len to include the bit length tree and counts */
    ZipInfoPtr->opt_len += 3*(max_blindex+1) + 5+5+4;
    Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld", opt_len, static_len));

    return max_blindex;
}

/* ===========================================================================
 * Send the header for a block using dynamic Huffman trees: the counts, the
 * lengths of the bit length codes, the literal tree and the distance tree.
 * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
 */
void send_all_trees(GZIP_TASK_INFO *ZipInfoPtr, int lcodes, int dcodes, int blcodes)
/* lcodes, dcodes, blcodes - number of codes for each tree */
{
    int rank;                    /* index in bl_order */

    Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
    Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
            "too many codes");
    Tracev((stderr, "\nbl counts: "));
    send_bits(ZipInfoPtr, lcodes-257, 5); /* not +255 as stated in appnote.txt */
    send_bits(ZipInfoPtr, dcodes-1,   5);
    send_bits(ZipInfoPtr, blcodes-4,  4); /* not -3 as stated in appnote.txt */
    for (rank = 0; rank < blcodes; rank++) {
        Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
        send_bits(ZipInfoPtr, ZipInfoPtr->bl_tree[bl_order[rank]].Len, 3);
    }
    Tracev((stderr, "\nbl tree: sent %ld", bits_sent));

    send_tree(ZipInfoPtr, (ct_data *)ZipInfoPtr->dyn_ltree, lcodes-1); /* send the literal tree */
    Tracev((stderr, "\nlit tree: sent %ld", bits_sent));

    send_tree(ZipInfoPtr, (ct_data *)ZipInfoPtr->dyn_dtree, dcodes-1); /* send the distance tree */
    Tracev((stderr, "\ndist tree: sent %ld", bits_sent));
}

/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file. This function
 * returns the total compressed length for the file so far.
 */
ulg flush_block(GZIP_TASK_INFO *ZipInfoPtr, char *buf, ulg stored_len, int eof)
/* buf            - input block, or NULL if too old
   ulg stored_len - length of input block
   int eof        -  true if this is the last block for a file */
{
    ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
    int max_blindex;  /* index of last bit length code of non zero freq */

    ZipInfoPtr->flag_buf[ZipInfoPtr->last_flags] = ZipInfoPtr->flags; /* Save the flags for the last 8 items */

    /* Construct the literal and distance trees */
    build_tree(ZipInfoPtr, (tree_desc *)(&ZipInfoPtr->l_desc));
    Tracev((stderr, "\nlit data: dyn %ld, stat %ld", opt_len, static_len));

    build_tree(ZipInfoPtr, (tree_desc *)(&ZipInfoPtr->d_desc));
    Tracev((stderr, "\ndist data: dyn %ld, stat %ld", opt_len, static_len));
    /* At this point, opt_len and static_len are the total bit lengths of
     * the compressed block data, excluding the tree representations.
     */

    /* Build the bit length tree for the above two trees, and get the index
     * in bl_order of the last bit length code to send.
     */
    max_blindex = build_bl_tree(ZipInfoPtr);

    /* Determine the best encoding. Compute first the block length in bytes */
    opt_lenb = (ZipInfoPtr->opt_len+3+7)>>3;
    static_lenb = (ZipInfoPtr->static_len+3+7)>>3;
    ZipInfoPtr->input_len += stored_len; /* for debugging only */

    Trace((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u dist %u ",
            opt_lenb, opt_len, static_lenb, static_len, stored_len,
            last_lit, last_dist));

    if (static_lenb <= opt_lenb) opt_lenb = static_lenb;

    /* If compression failed and this is the first and last block,
     * and if the zip file can be seeked (to rewrite the local header),
     * the whole file is transformed into a stored file:
     */
#ifdef FORCE_METHOD
    if (level == 1 && eof && compressed_len == 0L) { /* force stored file */
#else
    if (stored_len <= opt_lenb && eof && ZipInfoPtr->compressed_len == 0L && seekable()) {
#endif
        /* Since LIT_BUFSIZE <= 2*WSIZE, the input data must be there: */
        if (buf == (char*)0) error ("block vanished");

        copy_block(ZipInfoPtr, buf, (unsigned)stored_len, 0); /* without header */
        ZipInfoPtr->compressed_len = stored_len << 3;
        *ZipInfoPtr->file_method = STORED;

#ifdef FORCE_METHOD
    } else if (level == 2 && buf != (char*)0) { /* force stored block */
#else
    } else if (stored_len+4 <= opt_lenb && buf != (char*)0) {
                       /* 4: two words for the lengths */
#endif
        /* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
         * Otherwise we can't have processed more than WSIZE input bytes since
         * the last block flush, because compression would have been
         * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
         * transform a block into a stored block.
         */
        send_bits(ZipInfoPtr, (STORED_BLOCK<<1)+eof, 3);  /* send block type */
        ZipInfoPtr->compressed_len = (ZipInfoPtr->compressed_len + 3 + 7) & ~7L;
        ZipInfoPtr->compressed_len += (stored_len + 4) << 3;

        copy_block(ZipInfoPtr, buf, (unsigned)stored_len, 1); /* with header */

#ifdef FORCE_METHOD
    } else if (level == 3) { /* force static trees */
#else
    } else if (static_lenb == opt_lenb) {
#endif
        send_bits(ZipInfoPtr, (STATIC_TREES<<1)+eof, 3);
        compress_block(ZipInfoPtr, (ct_data *)ZipInfoPtr->static_ltree, (ct_data*)ZipInfoPtr->static_dtree);
        ZipInfoPtr->compressed_len += 3 + ZipInfoPtr->static_len;
    } else {
        send_bits(ZipInfoPtr, (DYN_TREES<<1)+eof, 3);
        send_all_trees(ZipInfoPtr, ZipInfoPtr->l_desc.max_code+1, ZipInfoPtr->d_desc.max_code+1, max_blindex+1);
        compress_block(ZipInfoPtr, (ct_data *)ZipInfoPtr->dyn_ltree, (ct_data *)ZipInfoPtr->dyn_dtree);
        ZipInfoPtr->compressed_len += 3 + ZipInfoPtr->opt_len;
    }
    Assert (compressed_len == bits_sent, "bad compressed size");
    init_block(ZipInfoPtr);

    if (eof) {
        Assert (input_len == isize, "bad input size");
        bi_windup(ZipInfoPtr);
        ZipInfoPtr->compressed_len += 7;  /* align on byte boundary */
    }
    Tracev((stderr,"\ncomprlen %lu(%lu) ", compressed_len>>3,
           ZipInfoPtr->compressed_len-7*eof));

    return ZipInfoPtr->compressed_len >> 3;
}

/* ===========================================================================
 * Save the match info and tally the frequency counts. Return true if
 * the current block must be flushed.
 */
int ct_tally(GZIP_TASK_INFO *ZipInfoPtr, int dist, int lc)
/* dist - distance of matched string
   lc   - match length-MIN_MATCH or unmatched char (if dist==0) */
{
    ZipInfoPtr->inbuf[ZipInfoPtr->last_lit++] = (uch)lc;
    if (dist == 0) {
        /* lc is the unmatched char */
        ZipInfoPtr->dyn_ltree[lc].Freq++;
    } else {
        /* Here, lc is the match length - MIN_MATCH */
        dist--;             /* dist = match distance - 1 */
        Assert((ush)dist < (ush)MAX_DIST &&
               (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
               (ush)d_code(dist) < (ush)D_CODES,  "ct_tally: bad match");

        ZipInfoPtr->dyn_ltree[ZipInfoPtr->length_code[lc]+LITERALS+1].Freq++;
        ZipInfoPtr->dyn_dtree[d_code(dist)].Freq++;

        ZipInfoPtr->d_buf[ZipInfoPtr->last_dist++] = (ush)dist;
        ZipInfoPtr->flags |= ZipInfoPtr->flag_bit;
    }
    ZipInfoPtr->flag_bit <<= 1;

    /* Output the flags if they fill a byte: */
    if ((ZipInfoPtr->last_lit & 7) == 0) {
        ZipInfoPtr->flag_buf[ZipInfoPtr->last_flags++] = ZipInfoPtr->flags;
        ZipInfoPtr->flags = 0, ZipInfoPtr->flag_bit = 1;
    }
    /* Try to guess if it is profitable to stop the current block here */
    if (ZipInfoPtr->level > 2 && (ZipInfoPtr->last_lit & 0xfff) == 0) {
        /* Compute an upper bound for the compressed length */
        ulg out_length = (ulg)ZipInfoPtr->last_lit*8L;
        ulg in_length = (ulg)ZipInfoPtr->strstart-ZipInfoPtr->block_start;
        int dcode;
        for (dcode = 0; dcode < D_CODES; dcode++) {
            out_length += (ulg)ZipInfoPtr->dyn_dtree[dcode].Freq*(5L+extra_dbits[dcode]);
        }
        out_length >>= 3;
        Trace((stderr,"\nlast_lit %u, last_dist %u, in %ld, out ~%ld(%ld%%) ",
               last_lit, last_dist, in_length, out_length,
               100L - out_length*100L/in_length));
        if (ZipInfoPtr->last_dist < ZipInfoPtr->last_lit/2 && out_length < in_length/2) return 1;
    }
    return (ZipInfoPtr->last_lit == LIT_BUFSIZE-1 || ZipInfoPtr->last_dist == DIST_BUFSIZE);
    /* We avoid equality with LIT_BUFSIZE because of wraparound at 64K
     * on 16 bit machines and because stored blocks are restricted to
     * 64K-1 bytes.
     */
}

/* ===========================================================================
 * Send the block data compressed using the given Huffman trees
 */
void compress_block(GZIP_TASK_INFO *ZipInfoPtr, ct_data *ltree, ct_data *dtree)
/* ltree - literal tree
   dtree - distance tree */
{
    unsigned dist;      /* distance of matched string */
    int lc;             /* match length or unmatched char (if dist == 0) */
    unsigned lx = 0;    /* running index in l_buf */
    unsigned dx = 0;    /* running index in d_buf */
    unsigned fx = 0;    /* running index in flag_buf */
    uch flag = 0;       /* current flags */
    unsigned code;      /* the code to send */
    int extra;          /* number of extra bits to send */

    if (ZipInfoPtr->last_lit != 0) do {
        if ((lx & 7) == 0) flag = ZipInfoPtr->flag_buf[fx++];
        lc = ZipInfoPtr->inbuf[lx++];
        if ((flag & 1) == 0) {
            send_code(lc, ltree); /* send a literal byte */
            Tracecv(isgraph(lc), (stderr," '%c' ", lc));
        } else {
            /* Here, lc is the match length - MIN_MATCH */
            code = ZipInfoPtr->length_code[lc];
            send_code(code+LITERALS+1, ltree); /* send the length code */
            extra = extra_lbits[code];
            if (extra != 0) {
                lc -= ZipInfoPtr->base_length[code];
                send_bits(ZipInfoPtr, lc, extra);        /* send the extra length bits */
            }
            dist = ZipInfoPtr->d_buf[dx++];
            /* Here, dist is the match distance - 1 */
            code = d_code(dist);
            Assert (code < D_CODES, "bad d_code");

            send_code(code, dtree);       /* send the distance code */
            extra = extra_dbits[code];
            if (extra != 0) {
                dist -= ZipInfoPtr->base_dist[code];
                send_bits(ZipInfoPtr, dist, extra);   /* send the extra distance bits */
            }
        } /* literal or match pair ? */
        flag >>= 1;
    } while (lx < ZipInfoPtr->last_lit);

    send_code(END_BLOCK, ltree);
}
