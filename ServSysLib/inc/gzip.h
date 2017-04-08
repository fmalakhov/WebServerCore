# if ! defined( GZipH )
#	define GZipH	/* only include me once */

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

#ifdef WIN32 /* Windows NT */
#  define HAVE_SYS_UTIME_H
#  define NO_UTIME_H
#  define PATH_SEP2 '\\'
#  define PATH_SEP3 ':'
#  define MAX_PATH_LEN  260
#  define NO_CHOWN
#  define PROTO
#  define STDC_HEADERS
#  define SET_BINARY_MODE(fd) setmode(fd, O_BINARY)
#  include <io.h>
#  include <malloc.h>
#  ifdef NTFAT
#    define NO_MULTIPLE_DOTS
#    define MAX_EXT_CHARS 3
#    define Z_SUFFIX "z"
#    define casemap(c) tolow(c) /* Force file names to lower case */
#  endif
#  define OS_CODE  0x0b
#endif

#if defined(pyr) && !defined(NOMEMCPY) /* Pyramid */
#  define NOMEMCPY /* problem with overlapping copies */
#endif

#ifdef TOPS20
#  define OS_CODE  0x0a
#endif

#ifndef unix
#  define NO_ST_INO /* don't rely on inode numbers */
#endif

/* Common defaults */

#ifndef OS_CODE
#  define OS_CODE  0x03  /* assume Unix */
#endif

#ifndef casemap
#  define casemap(c) (c)
#endif

#ifndef OPTIONS_VAR
#  define OPTIONS_VAR "GZIP"
#endif

#ifdef MAX_EXT_CHARS
#  define MAX_SUFFIX  MAX_EXT_CHARS
#else
#  define MAX_SUFFIX  30
#endif

#ifndef MIN_PART
#  define MIN_PART 3
   /* keep at least MIN_PART chars between dots in a file name. */
#endif

#ifndef EXPAND
#  define EXPAND(argc,argv)
#endif

#ifndef RECORD_IO
#  define RECORD_IO 0
#endif

#ifndef SET_BINARY_MODE
#  define SET_BINARY_MODE(fd)
#endif

#ifndef OPEN
#  define OPEN(name, flags, mode) open(name, flags, mode)
#endif

#ifndef get_char
#  define get_char() get_byte()
#endif

#ifndef put_char
#  define put_char(c) put_byte(c)
#endif

#if defined(__STDC__) || defined(PROTO)
#  define OF(args)  args
#else
#  define OF(args)  ()
#endif

#ifdef __STDC__
   typedef void *voidp;
#else
   typedef char *voidp;
#endif

/* I don't like nested includes, but the string and io functions are used
 * too often
 */
#include <stdio.h>
#if !defined(NO_STRING_H) || defined(STDC_HEADERS)
#  include <string.h>
#  if !defined(STDC_HEADERS) && !defined(NO_MEMORY_H) && !defined(__GNUC__)
#    include <memory.h>
#  endif
#  define memzero(s, n)     memset ((voidp)(s), 0, (n))
#else
#  include <strings.h>
#  define strchr            index 
#  define strrchr           rindex
#  define memcpy(d, s, n)   bcopy((s), (d), (n)) 
#  define memcmp(s1, s2, n) bcmp((s1), (s2), (n)) 
#  define memzero(s, n)     bzero((s), (n))
#endif

#ifndef RETSIGTYPE
#  define RETSIGTYPE void
#endif

#define local static

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* Compression methods (see algorithm.doc) */
#define STORED      0
#define COMPRESSED  1
#define PACKED      2
#define LZHED       3
/* methods 4 to 7 reserved */
#define DEFLATED    8
#define MAX_METHODS 9

/* To save memory for 16 bit systems, some arrays are overlaid between
 * the various modules:
 * deflate:  prev+head   window      d_buf  l_buf  outbuf
 * unlzw:    tab_prefix  tab_suffix  stack  inbuf  outbuf
 * inflate:              window             inbuf
 * unpack:               window             inbuf  prefix_len
 * unlzh:    left+right  window      c_table inbuf c_len
 * For compression, input is done in window[]. For decompression, output
 * is done in window except for unlzw.
 */

#ifndef	INBUFSIZ
#define INBUFSIZ  0x8000    /* input buffer size */
#endif
#define INBUF_EXTRA  64     /* required by unlzw() */

#ifndef	OUTBUFSIZ
#define OUTBUFSIZ  16384  /* output buffer size */
#endif
#define OUTBUF_EXTRA 2048   /* required by unlzw() */

#ifndef DIST_BUFSIZE
#define DIST_BUFSIZE 0x8000 /* buffer for distances, see trees.c */
#endif

#define EXTERN(type, array)  extern type array[]
#define DECLARE(type, array, size)  type array[size]
#define ALLOC(type, array, size)
#define FREE(array)

/* for compatibility with old zip sources (to be cleaned) */

//extern long time_stamp; /* original time stamp (modification time) */
extern long ifile_size; /* input file size, -1 for devices (debug only) */

typedef int file_t;     /* Do not use stdio */

#define	PACK_MAGIC     "\037\036" /* Magic header for packed files */
#define	GZIP_MAGIC     "\037\213" /* Magic header for gzip files, 1F 8B */
#define	OLD_GZIP_MAGIC "\037\236" /* Magic header for gzip 0.5 = freeze 1.x */
#define	LZH_MAGIC      "\037\240" /* Magic header for SCO LZH Compress files*/
#define PKZIP_MAGIC    "\120\113\003\004" /* Magic header for pkzip files */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define CONTINUATION 0x02 /* bit 1 set: continuation of multi-part gzip file */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define ENCRYPTED    0x20 /* bit 5 set: file is encrypted */
#define RESERVED     0xC0 /* bit 6,7:   reserved */

/* internal file attribute */
#define UNKNOWN 0xffff
#define BINARY  0
#define ASCII   1

#ifndef WSIZE
#  define WSIZE 0x8000     /* window size--must be a power of two, and */
#endif                     /*  at least 32K for zip's deflate method */

#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */


#ifndef HASH_BITS
#   define HASH_BITS  15
#endif

#define HASH_SIZE (unsigned)(1<<HASH_BITS)
#define HASH_MASK (HASH_SIZE-1)
#define WMASK     (WSIZE-1)
/* HASH_SIZE and WSIZE must be powers of two */

#define NIL 0
/* Tail of hash chains */

#define FAST 4
#define SLOW 2
/* speed options for the general purpose bit flag */

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

/* ===========================================================================
 * Local data used by the "longest match" routines.
 */

#ifndef BITS
#  define BITS 16
#endif
#define INIT_BITS 9              /* Initial number of bits per code */

#define	LZW_MAGIC  "\037\235"   /* Magic header for lzw files, 1F 9D */

#define BIT_MASK    0x1f /* Mask for 'number of compression bits' */
/* Mask 0x20 is reserved to mean a fourth header byte, and 0x40 is free.
 * It's a pity that old uncompress does not check bit 0x20. That makes
 * extension of the format actually undesirable because old compress
 * would just crash on the new format instead of giving a meaningful
 * error message. It does check the number of bits, but it's more
 * helpful to say "unsupported format, get a new version" than
 * "can only handle 16 bits".
 */

#define BLOCK_MODE  0x80
/* Block compression: if table is full and compression rate is dropping,
 * clear the dictionary.
 */

#define LZW_RESERVED 0x60 /* reserved bits */

#define	CLEAR  256       /* flush the dictionary */
#define FIRST  (CLEAR+1) /* first free entry */

typedef ush Pos;
typedef unsigned IPos;
/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

/*********** For trees.c ***************/ 
#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define MAX_BL_BITS 7
/* Bit length codes must not exceed MAX_BL_BITS bits */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define END_BLOCK 256
/* end of block literal code */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#ifndef LIT_BUFSIZE
#  ifdef SMALL_MEM
#    define LIT_BUFSIZE  0x2000
#  else
#  ifdef MEDIUM_MEM
#    define LIT_BUFSIZE  0x4000
#  else
#    define LIT_BUFSIZE  0x8000
#  endif
#  endif
#endif
#ifndef DIST_BUFSIZE
#  define DIST_BUFSIZE  LIT_BUFSIZE
#endif
/* Sizes of match buffers for literals/lengths and distances.  There are
 * 4 reasons for limiting LIT_BUFSIZE to 64K:
 *   - frequencies can be kept in 16 bit counters
 *   - if compression is not successful for the first block, all input data is
 *     still in the window so we can still emit a stored block even when input
 *     comes from standard input.  (This can also be done for all blocks if
 *     LIT_BUFSIZE is not greater than 32K.)
 *   - if compression is not successful for a file smaller than 64K, we can
 *     even emit a stored file instead of a stored block (saving 5 bytes).
 *   - creating new Huffman trees less frequently may not provide fast
 *     adaptation to changes in the input data statistics. (Take for
 *     example a binary file with poorly compressible code followed by
 *     a highly compressible string table.) Smaller buffer sizes give
 *     fast adaptation but have of course the overhead of transmitting trees
 *     more frequently.
 *   - I can't count above 4
 * The current code is general and allows DIST_BUFSIZE < LIT_BUFSIZE (to save
 * memory at the expense of compression). Some optimizations would be possible
 * if we rely on DIST_BUFSIZE == LIT_BUFSIZE.
 */
#if LIT_BUFSIZE > INBUFSIZ
    error cannot overlay l_buf and inbuf
#endif

#define REP_3_6      16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10    17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138  18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

#define HEAP_SIZE (2*L_CODES+1) /* maximum heap size */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */
typedef struct config {
   ush good_length; /* reduce lazy search above this match length */
   ush max_lazy;    /* do not perform lazy search above this match length */
   ush nice_length; /* quit search above this match length */
   ush max_chain;
} config;

/*********** End for trees.c ***************/

/* Data structure describing a single value and its code string. */
typedef struct ct_data {
    union {
        ush  freq;       /* frequency count */
        ush  code;       /* bit string */
    } fc;
    union {
        ush  dad;        /* father node in Huffman tree */
        ush  len;        /* length of bit string */
    } dl;
} ct_data;

typedef struct tree_desc {
    ct_data *dyn_tree;    /* the dynamic tree */
    ct_data *static_tree; /* corresponding static tree or NULL */
    int     *extra_bits;  /* extra bits for each code or NULL */
    int     extra_base;   /* base index for extra_bits */
    int     elems;        /* max number of elements in the tree */
    int     max_length;   /* max bit length for the codes */
    int     max_code;     /* largest code with non zero frequency */
} tree_desc;

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

typedef struct {
    ulg  crc;            /* crc on uncompressed file data */
    long header_bytes;   /* number of bytes in gzip header */
    long time_stamp;     /* original time stamp (modification time) */
    long bytes_in;       /* number of input bytes */
    long bytes_out;      /* number of output bytes */
    unsigned inptr;      /* index of next byte to be processed in inbuf */
    unsigned outcnt;     /* bytes in output buffer */
    int level;           /* compression level */
    unsigned insize;     /* valid bytes in inbuf */
    int test;            /* test .gz file integrity */
    int quiet;           /* be very quiet (-q) */
    int method;          /* compression method */

    unsigned char* SourceBufPtr;
    unsigned char* CompressBufPtr;     /* Pointer to buffer with compression data */
    unsigned int   BufLenForRead;
    unsigned short bi_buf;   /* Output buffer. bits are inserted starting at the bottom (least significant bits).*/
    int            bi_valid; /* Number of valid bits in bi_buf.  All bits above the last valid bit are always zero.*/

	uch inbuf[INBUFSIZ +INBUF_EXTRA];  /* input buffer */
    uch outbuf[OUTBUFSIZ+OUTBUF_EXTRA];/* output buffer */
	ush d_buf[DIST_BUFSIZE];           /* buffer for distances, see trees.c */
	uch window[2L*WSIZE];              /* Sliding window and suffix table (unlzw) */
	ush prev[1L<<BITS];

	/* For trees.c */
    ct_data dyn_ltree[HEAP_SIZE];   /* literal and length tree */
    ct_data dyn_dtree[2*D_CODES+1]; /* distance tree */
    ct_data static_ltree[L_CODES+2];
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see ct_init
 * below).
 */
    ct_data static_dtree[D_CODES];
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */
    ct_data bl_tree[2*BL_CODES+1]; /* Huffman tree for the bit lengths */
    tree_desc l_desc;
    tree_desc d_desc;
    tree_desc bl_desc;
    ush bl_count[MAX_BITS+1]; /* number of codes at each bit length for an optimal tree */
    uch depth[2*L_CODES+1];   /* Depth of each subtree used as tie breaker for trees of equal frequency */
    int heap[2*L_CODES+1];    /* heap used to build the Huffman trees */
    int heap_len;             /* number of elements in the heap */
    int heap_max;             /* element of largest frequency */
/* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
 * The same heap array is used to build all trees. */
    uch length_code[MAX_MATCH-MIN_MATCH+1]; /* length code for each normalized match length (0 == MIN_MATCH) */
    uch dist_code[512];
/* distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */
    int base_length[LENGTH_CODES]; /* First normalized length for each code (0 = MIN_MATCH) */
    int base_dist[D_CODES];        /* First normalized distance for each code (0 = distance of 1) */

    uch flag_buf[(LIT_BUFSIZE/8)];
/* flag_buf is a bit array distinguishing literals from lengths in
 * l_buf, thus indicating the presence or absence of a distance.*/

    unsigned last_lit;    /* running index in l_buf */
    unsigned last_dist;   /* running index in d_buf */
    unsigned last_flags;  /* running index in flag_buf */
    uch flags;            /* current flags not yet saved in flag_buf */
    uch flag_bit;         /* current bit used in flags */
/* bits are filled in flags starting at bit 0 (least significant).
 * Note: these flags are overkill in the current code since we don't
 * take advantage of DIST_BUFSIZE == LIT_BUFSIZE.
 */

    ulg opt_len;        /* bit length of current block with optimal trees */
    ulg static_len;     /* bit length of current block with static trees */
    ulg compressed_len; /* total bit length of compressed file */
    ulg input_len;      /* total byte length of input file */
/* input_len is for debugging only since we can get it by other means. */

    ush *file_type;     /* pointer to UNKNOWN, BINARY or ASCII */
    int *file_method;   /* pointer to DEFLATE or STORE */

	/* For deflate.c */
    long block_start;
/* window position at the beginning of the current output block. Gets
 * negative when the window is moved backwards. */
    unsigned ins_h;     /* hash index of string to be inserted */

    unsigned int prev_length;
/* Length of the best match at previous step. Matches not greater than this
 * are discarded. This is used in the lazy match evaluation. */
    unsigned strstart;    /* start of string to insert */
    unsigned match_start; /* start of matching string */
    int      eofile;      /* flag set at end of input file */
    unsigned lookahead;   /* number of valid bytes ahead in window */
    unsigned max_chain_length;
/* To speed up deflation, hash chains are never searched beyond this length.
 * A higher limit improves compression ratio but degrades the speed. */

    unsigned int max_lazy_match;
/* Attempt to find a better match only when the current match is strictly
 * smaller than this value. This mechanism is used only for compression
 * levels >= 4. */

    int compr_level; /* compression level (1..9) */
    unsigned good_match; /* Use a faster search when the previous match is longer than this */

#ifdef  FULL_SEARCH
# define nice_match MAX_MATCH
#else
    int nice_match; /* Stop searching when current match exceeds this */
#endif
} GZIP_TASK_INFO;

extern int decrypt;        /* flag to turn on decryption */
extern int verbose;        /* be verbose (-v) */
extern int to_stdout;      /* output to stdout (-c) */
extern int save_orig_name; /* set if original name must be saved */

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf(0))
#define try_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf(1))

/* put_byte is used for the compressed output, put_ubyte for the
 * uncompressed output. However unlzw() uses window for its
 * suffix table instead of its output buffer, so it does not use put_ubyte
 * (to be cleaned up).
 */
#define put_byte(c) {ZipInfoPtr->outbuf[ZipInfoPtr->outcnt++]=(uch)(c); if (ZipInfoPtr->outcnt==OUTBUFSIZ)\
   flush_outbuf(ZipInfoPtr);}
#define put_ubyte(c) {window[ZipInfoPtr->outcnt++]=(uch)(c); if (ZipInfoPtr->outcnt==WSIZE)\
   flush_window(ZipInfoPtr);}

/* Output a 16 bit value, lsb first */
#define put_short(w) \
{ if (ZipInfoPtr->outcnt < OUTBUFSIZ-2) { \
    ZipInfoPtr->outbuf[ZipInfoPtr->outcnt++] = (uch) ((w) & 0xff); \
    ZipInfoPtr->outbuf[ZipInfoPtr->outcnt++] = (uch) ((ush)(w) >> 8); \
  } else { \
    put_byte((uch)((w) & 0xff)); \
    put_byte((uch)((ush)(w) >> 8)); \
  } \
}

/* Output a 32 bit value to the bit stream, lsb first */
#define put_long(n) { \
    put_short((n) & 0xffff); \
    put_short(((ulg)(n)) >> 16); \
}

#define seekable()    0  /* force sequential output */
#define translate_eol 0  /* no option -a yet */

#define tolow(c)  (isupper(c) ? (c)-'A'+'a' : (c))    /* force to lower case */

/* Macros for getting two-byte and four-byte header values */
#define SH(p) ((ush)(uch)((p)[0]) | ((ush)(uch)((p)[1]) << 8))
#define LG(p) ((ulg)(SH(p)) | ((ulg)(SH((p)+2)) << 16))

/* Diagnostic functions */
#ifdef DEBUG
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define Assert(cond,msg)
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

	/* in zip.c: */
extern int zip(GZIP_TASK_INFO *ZipInfoPtr, char *HtmlPageName, 
    unsigned char isBinaryData, int SrcSize, 
    unsigned char* SrcPtr, unsigned char* DestPtr);

	/* in gzip.c */
RETSIGTYPE abort_gzip OF((void));

        /* in deflate.c */
void lm_init OF((GZIP_TASK_INFO *ZipInfoPtr, int pack_level, ush *flags));
ulg  deflate(GZIP_TASK_INFO *ZipInfoPtr);
void fill_window(GZIP_TASK_INFO *ZipInfoPtr);
ulg deflate_fast(GZIP_TASK_INFO *ZipInfoPtr);
int longest_match(GZIP_TASK_INFO *ZipInfoPtr, IPos cur_match);

#ifdef DEBUG
void check_match OF((IPos start, IPos match, int length));
#endif

        /* in trees.c */
void ct_init(GZIP_TASK_INFO *ZipInfoPtr, ush *attr, int *method);
int  ct_tally(GZIP_TASK_INFO *ZipInfoPtr, int dist, int lc);
ulg  flush_block(GZIP_TASK_INFO *ZipInfoPtr, char *buf, ulg stored_len, int eof);
void send_tree(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int max_code);
void send_all_trees(GZIP_TASK_INFO *ZipInfoPtr, int lcodes, int dcodes, int blcodes);
void compress_block(GZIP_TASK_INFO *ZipInfoPtr, ct_data *ltree, ct_data *dtree);
void init_block(GZIP_TASK_INFO *ZipInfoPtr);
void gen_bitlen(GZIP_TASK_INFO *ZipInfoPtr, tree_desc *desc);
void gen_codes(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int max_code);
void build_tree(GZIP_TASK_INFO *ZipInfoPtr, tree_desc *desc);
void scan_tree(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int max_code);
int  build_bl_tree(GZIP_TASK_INFO *ZipInfoPtr);
void pqdownheap(GZIP_TASK_INFO *ZipInfoPtr, ct_data *tree, int k);

        /* in bits.c */
void     bi_init(GZIP_TASK_INFO *ZipInfoPtr);
void     send_bits(GZIP_TASK_INFO *ZipInfoPtr, int value, int length);
unsigned bi_reverse OF((unsigned value, int length));
void     bi_windup(GZIP_TASK_INFO *ZipInfoPtr);
void     copy_block(GZIP_TASK_INFO *ZipInfoPtr, char *buf, unsigned len, int header);
int      read_buf(GZIP_TASK_INFO *ZipInfoPtr, char *buf, unsigned size);

	/* in util.c: */
extern int copy(GZIP_TASK_INFO *ZipInfoPtr, int in, int out);
extern ulg  updcrc        OF((uch *s, unsigned n));
extern void clear_bufs(GZIP_TASK_INFO *ZipInfoPtr);
extern int  fill_inbuf    OF((int eof_ok));
extern void flush_outbuf(GZIP_TASK_INFO *ZipInfoPtr);
extern void flush_window(GZIP_TASK_INFO *ZipInfoPtr);
extern void write_buf(GZIP_TASK_INFO *ZipInfoPtr, voidp buf, unsigned cnt);
//extern char *strlwr       OF((char *s));
extern char *basename     OF((char *fname));
extern void make_simple_name OF((char *name));
extern char *add_envopt   OF((int *argcp, char ***argvp, char *env));
extern void error         OF((char *m));
extern void display_ratio OF((long num, long den, FILE *file));
extern voidp xmalloc      OF((unsigned int size));

	/* in inflate.c */
extern int inflate OF((void));

extern int extra_lbits[];
extern int extra_dbits[];
extern int extra_blbits[];

//---------------------------------------------------------------------------
#endif  /* if ! defined( GZipH ) */
