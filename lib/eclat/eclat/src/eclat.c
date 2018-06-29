/*----------------------------------------------------------------------
  File    : eclat.c
  Contents: eclat algorithm for finding frequent item sets
  Author  : Christian Borgelt
  History : 2002.06.09 file created from apriori.c
            2002.12.10 option -l (list supporting transactions added)
            2002.08.16 transaction reading improved
            2003.08.18 memory benchmarking functionality added
            2003.08.20 option -t (target type) added
            2003.08.22 based on transaction module from apriori
            2003.08.23 option -q (item sort control) added
            2003.09.12 option -u (sparse representation) added
            2004.08.16 bug concerning option -q0 fixed (min. support)
            2004.11.23 absolute/relative support output changed
            2004.12.09 filter added (binary logarithm of supp. quotient)
            2005.06.20 use of flag for "no item sorting" corrected
            2006.11.26 adapted to new structures ISFMTR and ISEVAL
            2006.11.28 closed and maximal item sets without repository
            2007.02.13 adapted to modified module tabread
            2008.05.02 default limit for maximal number of items removed
            2008.10.26 complete redesign to simplify the structure
            2008.10.28 basic functionality of redesign completed
            2008.10.30 output of transaction ids per item set added
            2008.10.31 closed and maximal item set mining added
            2008.11.13 adapted to changes in transaction management
            2008.12.05 item set reporting order changed (post-order)
            2009.05.28 adapted to modified function tbg_filter()
            2009.10.09 closed/maximal item set check with repository
            2009.10.15 adapted to item set counter in reporter
            2010.03.09 version using transaction ranges added
            2010.03.17 head union tail pruning for maximal sets added
            2010.04.29 bug in memory organization fixed (if n > m)
            2010.06.23 code for transaction id reporting simplified
            2010.07.01 search based on diffsets added (dEclat)
            2010.07.04 bug in tid list setup fixed (after tbg_filter)
            2010.07.09 variant with horizontal processing added
            2010.07.11 filter version of intersection variant added
            2010.07.14 output file made optional (for benchmarking)
            2010.07.15 variant based on an item occurrence table added
            2010.08.05 closedness check based on extensions improved
            2010.08.19 item selection file added as optional input
            2010.08.22 adapted to modified modules tabread and tract
            2010.10.15 adapted to modified interface of module report
            2010.11.24 adapted to modified error reporting (tract)
            2010.11.26 memory handling simplified (list base sizes etc.)
            2010.12.11 adapted to a generic error reporting function
            2010.12.20 adapted to function tbg_icnts() (filter problem)
            2011.03.14 bug in memory allocation in eclat() fixed
            2011.03.19 two range checks for malloc() calls added
            2011.03.20 optional integer transaction weights added
            2011.07.08 adapted to modified function tbg_recode()
            2011.07.27 bug in function eclat_diff() fixed (list length)
            2011.07.29 re-sorting switched off for closed/maximal
            2011.08.15 bit vector version (with masking/reduction) added
            2011.08.16 adapted algorithm variants to finding generators
            2011.08.17 bit vector version modified to use bit map table
            2011.08.28 output of item set counters per size added
            2011.08.31 occurrence deliver version eclat_ocd() added
            2011.09.02 closed/maximal filtering without repo. improved
            2011.09.16 using 16-items machine for trans. ranges added
            2011.09.20 bug in closed/maximal filtering fixed (no repo.)
            2011.09.27 bug in algorithm and mode checking fixed (hut)
            2011.10.01 packing and sorting order for transaction ranges
            2012.04.10 bug in function rec_odfx() fixed (isr_xable())
            2012.05.25 occurrence deliver with item reordering added
            2012.06.13 bug in function rec_odro() fixed (single trans.)
            2012.06.19 function rec_odro() redesigned (delayed 16-items)
            2012.06.20 function fpg_adjust() added (consistency check)
            2012.06.22 use of 16-items machine in rec_odro() improved
            2013.01.24 closed/maximal filtering with vertical database
            2013.02.04 bug in transaction sorting for eclat_trg() fixed
            2013.02.07 check of elim. item support in closed() added
            2013.03.07 direction parameter added to sorting functions
            2013.03.22 adapted to type changes in module tract (SUPP)
            2013.03.26 adapted to type changes in module tract (TID)
            2013.03.28 adapted to type changes in module tract (ITEM)
            2013.10.15 checks of return code of isr_report() added
            2013.10.18 optional pattern spectrum collection added
            2013.10.31 bug in function ecl_adjust fixed (check of mrep)
            2013.11.12 item selection file changed to option -R#
            2013.11.22 bug in function rec_odro() fixed (option -l0)
            2014.05.12 option -F# added (support border for filtering)
            2014.08.05 association rule generation/evaluation added
            2014.08.22 adapted to modified item set reporter interface
            2014.08.28 functions eclat_data() and eclat_repo() added
            2014.09.04 functions rec_odcm(), odclo() and odmax() added
            2014.09.08 item bit filtering added to closed() and odclo()
            2014.10.24 changed from LGPL license to MIT license
            2016.02.18 bug concerning ECL_TIDS fixed (exclude ECL_FIM16)
------------------------------------------------------------------------
  Reference for the Eclat algorithm:
  * M.J. Zaki, S. Parthasarathy, M. Ogihara, and W. Li.
    New Algorithms for Fast Discovery of Association Rules.
    Proc. 3rd Int. Conf. on Knowledge Discovery and Data Mining
    (KDD 1997, Newport Beach, CA), 283-296.
    AAAI Press, Menlo Park, CA, USA 1997
  Reference for the dEclat algorithm (diffsets, option -Ad):
  * M.J. Zaki and K. Gouda.
    Fast Vertical Mining Using Diffsets.
    Proc. 9th ACM SIGKDD Int. Conf. on Knowledge Discovery
    and Data Mining (KDD 2003, Washington, DC), 326-335.
    ACM Press, New York, NY, USA 2003
  References for the LCM algorithm (occurrence deliver, option -Ao):
  * T. Uno, T. Asai, Y. Uchida, and H. Arimura.
    LCM: An Efficient Algorithm for Enumerating
    Frequent Closed Item Sets.
    Proc. Workshop on Frequent Item Set Mining Implementations
    (FIMI 2003, Melbourne, FL).
    CEUR Workshop Proceedings 90, TU Aachen, Germany 2003
    http://www.ceur-ws.org/Vol-90/
  * T. Uno, M. Kiyomi and H. Arimura.
    LCM ver.2: Efficient Mining Algorithms
    for Frequent/Closed/Maximal Itemsets.
    Proc. Workshop Frequent Item Set Mining Implementations
    (FIMI 2004, Brighton, UK).
    CEUR Workshop Proceedings 126, Aachen, Germany 2004
    http://www.ceur-ws.org/Vol-126/
  * T. Uno, M. Kiyomi, and H. Arimura.
    LCM ver.3: Collaboration of Array, Bitmap and Prefix Tree
    for Frequent Itemset Mining
    Proc. 1st Int. Workshop Open Source Data Mining (OSDM 2005)
    ACM Press, New York, NY, USA 2005
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#ifndef ISR_PATSPEC
#define ISR_PATSPEC
#endif
#ifdef  ECL_MAIN
#ifndef PSP_REPORT
#define PSP_REPORT
#endif
#ifndef TA_READ
#define TA_READ
#endif
#endif
#ifdef ECL_ABORT
#include "sigint.h"
#endif
#include "eclat.h"
#include "fim16.h"
#ifdef ECL_MAIN
#include "error.h"
#endif
#ifdef STORAGE
#include "storage.h"
#endif

#define BITMAP_TABLE            /* use a table instead of shifting */

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "eclat"
#define DESCRIPTION "find frequent item sets with the eclat algorithm"
#define VERSION     "version 5.10 (2016.02.18)        " \
                    "(c) 2002-2016   Christian Borgelt"

/* --- error codes --- */
/* error codes   0 to  -4 defined in tract.h */
#define E_STDIN      (-5)       /* double assignment of stdin */
#define E_OPTION     (-6)       /* unknown option */
#define E_OPTARG     (-7)       /* missing option argument */
#define E_ARGCNT     (-8)       /* too few/many arguments */
#define E_TARGET     (-9)       /* invalid target type */
#define E_SIZE      (-10)       /* invalid item set size */
#define E_SUPPORT   (-11)       /* invalid item set support */
#define E_CONF      (-12)       /* invalid confidence */
#define E_MEASURE   (-13)       /* invalid evaluation measure */
#define E_AGGMODE   (-14)       /* invalid aggregation mode */
#define E_VARIANT   (-16)       /* invalid algorithm variant */
/* error codes -15 to -25 defined in tract.h */

#define DIFFSIZE(p,q) ((size_t)((int*)(p)-(int*)(q)) *sizeof(int))

#ifndef QUIET                   /* if not quiet version, */
#define MSG         fprintf     /* print messages */
#define XMSG        if (mode & ECL_VERBOSE) fprintf
#define CLOCK(t)    ((t) = clock())
#else                           /* if quiet version, */
#define MSG(...)    ((void)0)   /* suppress messages */
#define XMSG(...)   ((void)0)
#define CLOCK(t)    ((void)0)
#endif

#define SEC_SINCE(t)  ((double)(clock()-(t)) /(double)CLOCKS_PER_SEC)

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- trans. identifier list --- */
  ITEM     item;                /* item identifier (last item in set) */
  SUPP     supp;                /* support of the item (or item set) */
  TID      tids[1];             /* array of transaction identifiers */
} TIDLIST;                      /* (transaction identifier list) */

typedef unsigned int BITBLK;    /* --- bit vector block --- */

typedef struct {                /* --- bit vector --- */
  ITEM     item;                /* item identifier (last item in set) */
  SUPP     supp;                /* support of the item (or item set) */
  BITBLK   bits[1];             /* bit vector over transactions */
} BITVEC;                       /* (bit vector) */

typedef struct {                /* --- transaction id range --- */
  TID      min;                 /* minimum transaction identifier */
  TID      max;                 /* maximum transaction identifier */
  SUPP     wgt;                 /* weight of transactions in range */
} TIDRANGE;                     /* (transaction id range) */

typedef struct {                /* --- transaction range list --- */
  ITEM     item;                /* item identifier (last item in set) */
  SUPP     supp;                /* support of the item (or item set) */
  TIDRANGE trgs[1];             /* array of transaction id ranges */
} TRGLIST;                      /* (transaction id range list) */

typedef struct {                /* --- transaction list --- */
  ITEM     item;                /* item identifier (last item in set) */
  SUPP     supp;                /* support of the item (or item set) */
  TID      cnt;                 /* number of transactions */
  TRACT    *tracts[1];          /* array  of transactions */
} TALIST;                       /* (transaction list) */

typedef struct {                /* --- recursion data --- */
  int      target;              /* target type (e.g. closed/maximal) */
  int      mode;                /* operation mode (e.g. pruning) */
  SUPP     smin;                /* minimum support of an item set */
  ITEM     first;               /* start value for item loops */
  int      dir;                 /* direction   for item loops */
  SUPP     *muls;               /* multiplicity of transactions */
  SUPP     *marks;              /* markers (for item occurrences) */
  ITEM     *cand;               /* to collect candidates (closed()) */
  SUPP     *miss;               /* support still missing (maximal()) */
  BITTA    *btas;               /* array of bit-rep. transactions */
  SUPP     **tab;               /* item occurrence table */
  TRACT    **hash;              /* buffer for hash table */
  TIDLIST  **elim;              /* tra. id lists of eliminated items */
  FIM16    *fim16;              /* 16-items machine */
  TABAG    *tabag;              /* original transaction bag */
  ISREPORT *report;             /* item set reporter */
  ISTREE   *istree;             /* item set tree for eclat_tree() */
} RECDATA;                      /* (recursion data) */

typedef struct {                /* --- eclat execution data --- */
  int      mode;                /* processing mode */
  ISTREE   *istree;             /* item set tree for rule generation */
} ECLAT;                        /* (eclat execution data) */

typedef TID COMBFN  (TIDLIST *d, TIDLIST *s1, TIDLIST *s2, SUPP *w);
typedef int ECLATFN (TABAG *tabag, int target, SUPP smin, int mode,
                     ISREPORT *report);

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
#if !defined QUIET && defined ECL_MAIN
/* --- error messages --- */
static const char *errmsgs[] = {
  /* E_NONE      0 */  "no error",
  /* E_NOMEM    -1 */  "not enough memory",
  /* E_FOPEN    -2 */  "cannot open file %s",
  /* E_FREAD    -3 */  "read error on file %s",
  /* E_FWRITE   -4 */  "write error on file %s",
  /* E_STDIN    -5 */  "double assignment of standard input",
  /* E_OPTION   -6 */  "unknown option -%c",
  /* E_OPTARG   -7 */  "missing option argument",
  /* E_ARGCNT   -8 */  "wrong number of arguments",
  /* E_TARGET   -9 */  "invalid target type '%c'",
  /* E_SIZE    -10 */  "invalid item set size %"ITEM_FMT,
  /* E_SUPPORT -11 */  "invalid minimum support %g",
  /* E_CONF    -12 */  "invalid minimum confidence %g",
  /* E_MEASURE -13 */  "invalid evaluation measure '%c'",
  /* E_AGGMODE -14 */  "invalid aggregation mode '%c'",
  /* E_NOITEMS -15 */  "no (frequent) items found",
  /* E_VARIANT -16 */  "invalid eclat variant '%c'",
  /*           -17 */  "unknown error"
};
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#ifdef ECL_MAIN
#ifndef QUIET
static CCHAR    *prgname;       /* program name for error messages */
#endif
static TABREAD  *tread  = NULL; /* table/transaction reader */
static ITEMBASE *ibase  = NULL; /* item base */
static TABAG    *tabag  = NULL; /* transaction bag/multiset */
static ISREPORT *report = NULL; /* item set reporter */
static TABWRITE *twrite = NULL; /* table writer for pattern spectrum */
static double   *border = NULL; /* support border for filtering */
#endif

#ifdef BITMAP_TABLE
static int    bitcnt[256];      /* bit count table */
static BITBLK bitmap[256][256]; /* bit map   table */
#endif

/*----------------------------------------------------------------------
  Auxiliary Functions for Debugging
----------------------------------------------------------------------*/
#ifndef NDEBUG

static void indent (int k)
{ while (--k >= 0) printf("   "); }

/*--------------------------------------------------------------------*/

static void show_tid (const char *text, ITEMBASE *base,
                      TIDLIST **lists, ITEM k, int ind)
{                               /* --- show a cond. trans. database */
  ITEM i, j;                    /* item, loop variable */
  TID  *s;                      /* to traverse transaction ids */

  if (text && *text) {          /* print the given text */
    indent(ind); printf("%s\n", text); }
  for (j = 0; j < k; j++) {     /* traverse the items / tid lists */
    indent(ind);                /* indent the output line */
    i = lists[j]->item;         /* print the item name and id */
    if (i < 0) printf("packed  :");
    else       printf("%4s[%2"ITEM_FMT"]:", ib_name(base, i), i);
    for (s = lists[j]->tids; *s >= 0; s++)
      printf(" %"TID_FMT, *s);  /* print the transaction ids */
    printf(" (%"SUPP_FMT")\n", lists[j]->supp);
  }                             /* print the item support */
}  /* show_tid() */

/*--------------------------------------------------------------------*/

static void show_tab (const char *text, ITEMBASE *base,
                      SUPP **tab, TID n, ITEM k)
{                               /* --- show item counter table */
  ITEM i;                       /* loop variable for items */
  TID  r;                       /* loop variable for rows */

  if (text && *text)            /* if it is not empty, */
    printf("%s\n", text);       /* print the given text */
  printf("    ");               /* skip row id/tid column */
  for (r = 0; r < n; r++)       /* print the transaction header */
    printf(" %3"TID_FMT, r);    /* print the row number / tid */
  printf("\n");                 /* terminate the header line */
  for (i = 0; i < k; i++) {     /* traverse the table columns */
    printf("%4s[%2"ITEM_FMT"]:", ib_name(base, i), i);
    for (r = 0; r < n; r++) printf(" %3"SUPP_FMT, tab[i][r]);
    printf("\n");               /* print the item counters */
  }                             /* and terminate the line */
}  /* show_tab() */

/*--------------------------------------------------------------------*/

static void show_trg (const char *text, ITEMBASE *base,
                      TRGLIST **lists, ITEM k, int ind)
{                               /* --- show a cond. trans. database */
  ITEM     i, j;                /* item, loop variable */
  TIDRANGE *r;                  /* to traverse transaction id ranges */

  if (text && *text) {          /* print the given text */
    indent(ind); printf("%s\n", text); }
  for (j = 0; j < k; j++) {     /* traverse the items / range lists */
    indent(ind);                /* indent the output line */
    i = lists[j]->item;         /* get the item identifier */
    r = lists[j]->trgs;         /* and the transaction ranges */
    if (i < 0) {                /* if list for packed items */
      printf("packed:");        /* print special indicator */
      for ( ; r->min >= 0; r++){/* and the transaction ids */
        printf(" %"TID_FMT":%04x", r->min, (unsigned int)r->max);
        printf(":%"SUPP_FMT, r->wgt);
      } }
    else {                      /* if list for a normal item */
      printf("%s[%"ITEM_FMT"]:", ib_name(base, i), i);
      for ( ; r->min >= 0; r++){/* print item name and id */
        printf(" %"TID_FMT"-%"TID_FMT, r->min, r->max);
        printf(":%"SUPP_FMT, r->wgt);
      }                         /* print the transaction ranges */
    }
    printf(" (%"SUPP_FMT")\n", lists[j]->supp);
  }                             /* print the item support */
}  /* show_trg() */

#endif  /* #ifndef NDEBUG */
/*----------------------------------------------------------------------
  Eclat with Transaction Id List Intersection (basic version)
----------------------------------------------------------------------*/

static TID isect (TIDLIST *dst, TIDLIST *src1, TIDLIST *src2,SUPP *muls)
{                               /* --- intersect two trans. id lists */
  TID *s1, *s2, *d;             /* to traverse sources and dest. */

  assert(dst && src1 && src2    /* check the function arguments */
  &&    (src1->tids[0] >= 0) && (src2->tids[0] >= 0) && muls);
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = 0;                /* initialize the support */
  if (src1->supp > src2->supp) { s2 = src1->tids; s1 = src2->tids; }
  else                         { s1 = src1->tids; s2 = src2->tids; }
  d = dst->tids;                /* get sources and destination */
  while (1) {                   /* trans. id list intersection loop */
    if      (*s1 < *s2) s2++;   /* if one transaction id is larger, */
    else if (*s1 > *s2) s1++;   /* simply skip this transaction id */
    else if (*s1 <   0) break;  /* check for the sentinel */
    else { dst->supp += muls[*d++ = *s1++]; s2++; }
  }                             /* copy equal elements to destination */
  *d++ = (TID)-1;               /* store a sentinel at the list end */
  return (TID)(d -dst->tids);   /* return the size of the new list */
}  /* isect() */

/*--------------------------------------------------------------------*/

static int rec_base (TIDLIST **lists, ITEM k, size_t x, RECDATA *rd)
{                               /* --- eclat recursion with tid lists */
  int     r;                    /* error status */
  ITEM    i, m, z;              /* loop variables */
  SUPP    pex;                  /* minimum support for perfect exts. */
  TIDLIST *l, *d;               /* to traverse transaction id lists */
  TIDLIST **proj = NULL;        /* trans. id lists of proj. database */
  TID     *p;                   /* to organize the trans. id lists */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TIDLIST**)malloc((size_t)k *sizeof(TIDLIST*) +x);
    if (!proj) return -1;       /* allocate list and element arrays */
  }                             /* (memory for conditional databases) */
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    l = lists[k];               /* traverse the items / tid lists */
    r = isr_add(rd->report, l->item, l->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if (proj && (k > 0)) {      /* if another item can be added */
      pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
      proj[m = 0] = d = (TIDLIST*)(p = (TID*)(proj +k+1));
      for (i = 0; i < k; i++) { /* intersect with preceding lists */
        x = (size_t)isect(d, lists[i], l, rd->muls);
        if      (d->supp >= pex)      /* collect perfect extensions */
          isr_addpex(rd->report, d->item);
        else if (d->supp >= rd->smin) /* collect frequent extensions */
          proj[++m] = d = (TIDLIST*)(p = d->tids +x);
      }                         /* switch to the next output list */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_base(proj, m, DIFFSIZE(p,proj[0]), rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) free(proj);         /* delete the list and element arrays */
  return r;                     /* return the error status */
}  /* rec_base() */

/*--------------------------------------------------------------------*/

int eclat_base (TABAG *tabag, int target, SUPP smin, int mode,
                ISREPORT *report)
{                               /* --- eclat with trans. id lists */
  int        r = 0;             /* result of recursion/error status */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        n;                 /* number of transactions */
  size_t     x;                 /* number of item instances */
  SUPP       w;                 /* weight/support buffer */
  SUPP       pex;               /* minimum support for perfect exts. */
  TRACT      *t;                /* to traverse transactions */
  TIDLIST    **lists, *l;       /* to traverse transaction id lists */
  TID        *tids, *p, **next; /* to traverse transaction ids */
  const ITEM *s;                /* to traverse transaction items */
  const TID  *c;                /* item occurrence counters */
  RECDATA    rd;                /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  c = tbg_icnts(tabag, 0);      /* get the number of containing */
  if (!c) return -1;            /* transactions per item */
  lists = (TIDLIST**)malloc((size_t)k *sizeof(TIDLIST*)
                           +(size_t)k *sizeof(TID*)
                           +(size_t)n *sizeof(SUPP));
  if (!lists) return -1;        /* create initial tid list array */
  next    = (TID**)(lists+k);   /* and split off next position array */
  rd.muls = (SUPP*)(next +k);   /* and transaction multiplicity array */
  x = tbg_extent(tabag);        /* allocate the tid list elements */
  p = tids = (TID*)malloc((size_t)k *sizeof(TIDLIST) +x *sizeof(TID));
  if (!p) { free(lists); return -1; } /* allocate tid list elements */
  for (i = 0; i < k; i++) {     /* traverse the items / tid lists */
    lists[i] = l = (TIDLIST*)p; /* get next transaction id list */
    l->item  = i;               /* initialize the list item */
    l->supp  = 0;               /* and the support counter */
    next[i]  = p = l->tids;     /* note position of next trans. id */
    p += c[i]; *p++ = (TID)-1;  /* skip space for transaction ids */
  }                             /* and store a sentinel at the end */
  while (n > 0) {               /* traverse the transactions */
    t = tbg_tract(tabag, --n);  /* get the next transaction */
    rd.muls[n] = w = ta_wgt(t); /* and store its weight */
    for (s = ta_items(t); *s > TA_END; s++) {
      lists[*s]->supp += w;     /* traverse the transaction's items */
      *next[*s]++      = n;     /* sum the transaction weight and */
    }                           /* collect the transaction ids */
  }
  for (i = m = 0; i < k; i++) { /* traverse the items / tid lists */
    l = lists[i];               /* eliminate all infrequent items and */
    if (l->supp <  rd.smin) continue;   /* collect perfect extensions */
    if (l->supp >= pex) { isr_addpex(report, i); continue; }
    lists[m++] = l;             /* collect lists for frequent items */
  }                             /* (eliminate infrequent items) */
  if (m > 0) {                  /* if there are frequent items */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = rec_base(lists, m, DIFFSIZE(p,tids), &rd);
  }                             /* find freq. items sets recursively */
  if (r >= 0)                   /* if no error occurred, */
    r = isr_report(report);     /* report the empty item set */
  free(tids); free(lists);      /* delete the allocated arrays */
  return r;                     /* return the error status */
}  /* eclat_base() */

/*----------------------------------------------------------------------
  Eclat with Transaction Id List Intersection (optimized version)
----------------------------------------------------------------------*/

static int tid_cmp (const void *a, const void *b, void *data)
{                               /* --- compare support of tid lists */
  if (((TIDLIST*)b)->supp > ((TIDLIST*)a)->supp) return  1;
  if (((TIDLIST*)b)->supp < ((TIDLIST*)a)->supp) return -1;
  return 0;                     /* return sign of support difference */
}  /* tid_cmp() */

/*--------------------------------------------------------------------*/

static int tid_cmpx (const void *a, const void *b, void *data)
{                               /* --- compare support of tid lists */
  if (((TIDLIST*)a)->item < 0) return -1;
  if (((TIDLIST*)b)->item < 0) return +1;
  if (((TIDLIST*)b)->supp > ((TIDLIST*)a)->supp) return  1;
  if (((TIDLIST*)b)->supp < ((TIDLIST*)a)->supp) return -1;
  return 0;                     /* return sign of support difference */
}  /* tid_cmpx() */

/*--------------------------------------------------------------------*/

static TID filter (TIDLIST *dst, TIDLIST *src, SUPP *muls)
{                               /* --- filter a transaction id list */
  SUPP m;                       /* multiplicity of transaction */
  TID  *s, *d;                  /* to traverse source and dest. */

  assert(dst && src && muls);   /* check the function arguments */
  dst->item = src->item;        /* copy first item and init. support */
  dst->supp = 0;                /* traverse the source trans. id list */
  for (d = dst->tids, s = src->tids; *s >= 0; s++)
    if ((m = muls[*s]) > 0) {   /* collect the marked trans. ids and */
      dst->supp += m; *d++ = *s; }    /* sum the transaction weights */
  *d++ = (TID)-1;               /* store a sentinel at the list end */
  return (TID)(d -dst->tids);   /* return the size of the new list */
}  /* filter() */

/*--------------------------------------------------------------------*/

static int closed (TIDLIST *list, RECDATA *rd, ITEM n)
{                               /* --- check for a closed item set */
  TIDLIST    *elim;             /* to traverse eliminated items */
  const ITEM *p;                /* to traverse transaction items */
  TID        *s, *d;            /* to traverse transaction ids */
  ITEM       *t, *r;            /* to traverse items */
  ITEM       item;              /* item of the list to test */
  int        i, m;              /* loop variable, bit mask for items */

  assert(list && rd             /* check the function arguments */
  &&    (rd->mode & ECL_EXTCHK));
  if (rd->mode & ECL_VERT) {    /* if to use vertical representation */
    while (--n >= 0) {          /* traverse the eliminated items */
      elim = rd->elim[n];       /* skip items with lower support */
      if (elim->supp < list->supp) continue;
      s = list->tids; d = elim->tids;
      while (1) {               /* test for a perfect extension */
        if      (*s < *d) d++;  /* skip missing destination id */
        else if (*s > *d) break;/* if source id is missing, abort */
        else if (*s <  0) return 0;
        else { s++; d++; }      /* check for the sentinel and */
      }                         /* skip matching transaction ids */
    }                           /* (all tids found: perfect ext.) */
    return -1; }                /* return 'item set is closed' */
  else {                        /* if to use horiz. representation */
    item = list->item;          /* get the item of the list */
    if (item < 31) {            /* if item bits can be used */
      for (m = 0, i = item; ++i < 32; )
        if (!isr_uses(rd->report, (ITEM)i))
          m |= 1 << i;          /* collect bits of unused items */
      for (s = list->tids; m && (*s >= 0); s++)
        m &= ta_getmark(tbg_tract(rd->tabag, *s));
      if (m) return 0;          /* if perf. ext found, 'not closed' */
      item = 31;                /* otherwise check remaining items */
    }                           /* (check only items 32 and above) */
    p = ta_items(tbg_tract(rd->tabag, list->tids[0]));
    for (r = rd->cand; *p > item; p++)
      if (!isr_uses(rd->report, *p))
        *r++ = *p;              /* collect items from a transaction */
    if (r <= rd->cand) return -1;
    *r = TA_END;                /* store a sentinel at the end */
    for (s = list->tids+1; *s >= 0; s++) {
      t = r = rd->cand;         /* traverse the transaction ids */
      p = ta_items(tbg_tract(rd->tabag, *s));
      while (1) {               /* item list intersection loop */
        if      (*t <  0) break;/* check for the list sentinel */
        else if (*t < *p) p++;  /* if one item id is larger, */
        else if (*t > *p) t++;  /* simply skip this item id, */
        else { *r++ = *t++; p++; }
      }                         /* (collect perfect ext. candidates) */
      if (r <= rd->cand) return -1;
      *r = TA_END;              /* if intersection is empty, abort, */
    }                           /* otherwise store a sentinel */
    return 0;                   /* return 'item set is not closed' */
  }
}  /* closed() */

/*--------------------------------------------------------------------*/

static int maximal (TIDLIST *list, RECDATA *rd, ITEM n)
{                               /* --- check for a maximal item set */
  ITEM       i;                 /* loop variable for items */
  SUPP       w;                 /* weight/support buffer */
  const ITEM *p;                /* to traverse transaction items */
  TID        *s, *d;            /* to traverse sources and dest. */

  assert(list && rd             /* check the function arguments */
  &&    (rd->mode & ECL_EXTCHK));
  if (rd->mode & ECL_VERT) {    /* if to use vertical representation */
    while (--n >= 0) {          /* traverse the eliminated items */
      s = list->tids; d = rd->elim[n]->tids;
      for (w = 0; 1; ) {        /* test for a perfect extension */
        if      (*s < *d) d++;  /* if one transaction id is larger, */
        else if (*s > *d) s++;  /* skip missing destination id */
        else if (*s <  0) break;/* check for the sentinel and */
        else { w += rd->muls[*s++]; d++; }
      }                         /* sum weights of matching trans. ids */
      if (w >= rd->smin) return 0;
    } }                         /* check for a frequent extension */
  else {                        /* if to use horiz. representation */
    for (i = tbg_itemcnt(rd->tabag); --i > list->item; )
      rd->miss[i] = (isr_uses(rd->report, i)) ? list->supp+1 : rd->smin;
    for (s = list->tids; *s >= 0; s++) {
      w = rd->muls[*s];         /* traverse the transactions */
      for (p = ta_items(tbg_tract(rd->tabag, *s)); *p > list->item; p++)
        if ((rd->miss[*p] -= w) <= 0) return 0;
    }                           /* count support of candidate exts.; */
  }                             /* if frequent cand. found, abort */
  return -1;                    /* return 'set is maximal' */
}  /* maximal() */

/*--------------------------------------------------------------------*/

static int rec_tcm (TIDLIST **lists, ITEM k, size_t x, ITEM e,
                    RECDATA *rd)
{                               /* --- eclat recursion with tid lists */
  int     r;                    /* error status */
  ITEM    i, m, z;              /* loop variables */
  SUPP    max;                  /* maximum support of an ext. item */
  SUPP    pex;                  /* minimum support for perfect exts. */
  TIDLIST **proj = NULL;        /* trans. id lists of proj. database */
  TIDLIST *l, *d;               /* to traverse transaction id lists */
  TID     *p;                   /* to traverse transaction ids */
  ITEM    *t;                   /* to collect the tail items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & ECL_TAIL) {    /* if to use tail to prune w/ repo. */
    t = isr_buf(rd->report);    /* collect the tail items in buffer */
    for (m = 0, i = k; --i >= 0; ) t[m++] = lists[i]->item;
    r = isr_tail(rd->report, t, m);
    if (r) return r;            /* if tail need not be processed, */
  }                             /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TIDLIST**)malloc((size_t)k *sizeof(TIDLIST*) +x);
    if (!proj) return -1;       /* allocate list and element arrays */
  }                             /* (memory for conditional databases) */
  if ((k > 4)                   /* if there are enough items left, */
  &&  (rd->mode & ECL_REORDER)) /* re-sort the items w.r.t. support */
    ptr_qsort(lists, (size_t)k, 1, (rd->fim16) ?tid_cmpx:tid_cmp, NULL);
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    l = lists[k];               /* traverse the items / tid lists */
    if (!closed(l, rd, e))      /* if the current set is not closed, */
      continue;                 /* the item need not be processed */
    r = isr_addnc(rd->report, l->item, l->supp);
    if (r < 0) break;           /* add current item to the reporter */
    max = 0;                    /* init. maximal extension support */
    if (proj && (k > 0)) {      /* if another item can be added */
      pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
      proj[m = 0] = d = (TIDLIST*)(proj +k+1);
      if (k < 2) {              /* if there are only few items left */
        /* Benchmark tests showed that this version is faster only */
        /* if there is only one other tid list to intersect with.  */
        if (lists[i = 0]->item < 0) { /* if there are packed items */
          x = (size_t)isect(d, lists[i++], l, rd->muls);
          if (d->supp >= rd->smin) {  /* if they are frequent */
            proj[++m] = d = (TIDLIST*)(d->tids +x); }
        }                       /* add a tid list for packed items */
        for ( ; i < k; i++) {   /* traverse the preceding lists */
          x = (size_t)isect(d, lists[i], l, rd->muls);
          if (d->supp < rd->smin) /* intersect transaction id lists */
            continue;           /* eliminate infrequent items */
          if (d->supp >= pex) { /* collect perfect extensions */
            isr_addpex(rd->report, d->item); continue; }
          if (d->supp > max)    /* find maximal extension support */
            max = d->supp;      /* (for later closed/maximal check) */
          proj[++m] = d = (TIDLIST*)(d->tids +x);
        } }                     /* collect tid lists of freq. items */
      else {                    /* if there are many items left */
        for (p = l->tids; *p >= 0; p++) /* mark transaction ids */
          rd->marks[*p] = rd->muls[*p]; /* in the current list */
        if (lists[i = 0]->item < 0) {   /* if there are packed items */
          x = (size_t)filter(d, lists[i++], rd->marks);
          if (d->supp >= rd->smin) {    /* if they are frequent */
            proj[++m] = d = (TIDLIST*)(d->tids +x); }
        }                       /* add a tid list for packed items */
        for ( ; i < k; i++) {   /* traverse the preceding lists */
          x = (size_t)filter(d, lists[i], rd->marks);
          if (d->supp < rd->smin) /* intersect transaction id lists */
            continue;           /* eliminate infrequent items */
          if (d->supp >= pex) { /* collect perfect extensions */
            isr_addpex(rd->report, d->item); continue; }
          if (d->supp > max)    /* find maximal extension support */
            max = d->supp;      /* (for later closed/maximal check) */
          proj[++m] = d = (TIDLIST*)(d->tids +x);
        }                       /* collect tid lists of freq. items */
        for (p = l->tids; *p >= 0; p++)
          rd->marks[*p] = 0;    /* unmark transaction ids */
      }                         /* in the current list */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_tcm(proj, m, DIFFSIZE(d,proj[0]), e, rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }                           /* (or rather their trans. id lists) */
    if ((rd->target & ISR_CLOSED) ? (max < l->supp)
    :   ((max < rd->smin) && maximal(l, rd, e))) {
      r = isr_reportx(rd->report, l->tids, (TID)-l->supp);
      if (r < 0) break;         /* report the current item set */
    }                           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item and */
    if (rd->mode & ECL_VERT)    /* collect the eliminated items */
      rd->elim[e++] = l;        /* (for closed/maximal check) */
  }
  if (proj) free(proj);         /* delete the list and element arrays */
  return r;                     /* return the error status */
}  /* rec_tcm() */

/*--------------------------------------------------------------------*/

static int rec_tid (TIDLIST **lists, ITEM k, size_t x, RECDATA *rd)
{                               /* --- eclat recursion with tid lists */
  int     r;                    /* error status */
  ITEM    i, m, z;              /* loop variables, error status */
  SUPP    max;                  /* maximum support of an ext. item */
  SUPP    pex;                  /* minimum support for perfect exts. */
  TIDLIST **proj = NULL;        /* trans. id lists of proj. database */
  TIDLIST *l, *d;               /* to traverse transaction id lists */
  TID     *p;                   /* to traverse transaction ids */
  ITEM    *t;                   /* to collect the tail items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & ECL_TAIL) {    /* if to use tail to prune w/ repo. */
    t = isr_buf(rd->report);    /* collect the tail items in buffer */
    for (m = 0, i = k; --i >= 0; ) t[m++] = lists[i]->item;
    r = isr_tail(rd->report, t, m);
    if (r) return r;            /* if tail need not be processed, */
  }                             /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TIDLIST**)malloc((size_t)k *sizeof(TIDLIST*) +x);
    if (!proj) return -1;       /* allocate list and element arrays */
  }                             /* (memory for conditional databases) */
  if ((k > 4)                   /* if there are enough items left, */
  &&  (rd->mode & ECL_REORDER)) /* re-sort the items w.r.t. support */
    ptr_qsort(lists, (size_t)k, 1, (rd->fim16) ?tid_cmpx:tid_cmp, NULL);
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    l = lists[k];               /* traverse the items / tid lists */
    if (l->item < 0) {          /* if this list is for packed items */
      for (p = l->tids; *p >= 0; p++)
        m16_add(rd->fim16, rd->btas[*p], rd->muls[*p]);
      r = m16_mine(rd->fim16);  /* add bit-rep. transaction prefixes */
      if (r >= 0) continue;     /* to the 16-items machine and mine, */
      if (proj) free(proj);     /* then go to the next trans. id list */
      return r;                 /* otherwise free allocated memory */
    }                           /* and abort the function */
    r = isr_add(rd->report, l->item, l->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    max = 0;                    /* init. maximal extension support */
    if (proj && (k > 0)) {      /* if another item can be added */
      pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
      proj[m = 0] = d = (TIDLIST*)(proj +k+1);
      if (k < 2) {              /* if there are only few items left */
        /* Benchmark tests showed that this version is faster only */
        /* if there is only one other tid list to intersect with.  */
        if (lists[i = 0]->item < 0) { /* if there are packed items */
          x = (size_t)isect(d, lists[i++], l, rd->muls);
          if (d->supp >= rd->smin) {  /* if they are frequent */
            proj[++m] = d = (TIDLIST*)(d->tids +x); }
        }                       /* add a tid list for packed items */
        for ( ; i < k; i++) {   /* traverse the preceding lists */
          x = (size_t)isect(d, lists[i], l, rd->muls);
          if (d->supp < rd->smin) /* intersect transaction id lists */
            continue;           /* eliminate infrequent items */
          if (d->supp >= pex) { /* collect perfect extensions */
            isr_addpex(rd->report, d->item); continue; }
          if (d->supp > max)    /* find maximal extension support */
            max = d->supp;      /* (for later closed/maximal check) */
          proj[++m] = d = (TIDLIST*)(d->tids +x);
        } }                     /* collect tid lists of freq. items */
      else {                    /* if there are many items left */
        for (p = l->tids; *p >= 0; p++) /* mark transaction ids */
          rd->marks[*p] = rd->muls[*p]; /* in the current list */
        if (lists[i = 0]->item < 0) {   /* if there are packed items */
          x = (size_t)filter(d, lists[i++], rd->marks);
          if (d->supp >= rd->smin) {    /* if they are frequent */
            proj[++m] = d = (TIDLIST*)(d->tids +x); }
        }                       /* add a tid list for packed items */
        for ( ; i < k; i++) {   /* traverse the preceding lists */
          x = (size_t)filter(d, lists[i], rd->marks);
          if (d->supp < rd->smin) /* intersect transaction id lists */
            continue;           /* eliminate infrequent items */
          if (d->supp >= pex) { /* collect perfect extensions */
            isr_addpex(rd->report, d->item); continue; }
          if (d->supp > max)    /* find maximal extension support */
            max = d->supp;      /* (for later closed/maximal check) */
          proj[++m] = d = (TIDLIST*)(d->tids +x);
        }                       /* collect tid lists of freq. items */
        for (p = l->tids; *p >= 0; p++)
          rd->marks[*p] = 0;    /* unmark transaction ids */
      }                         /* in the current list */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_tid(proj, m, DIFFSIZE(d,proj[0]), rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }
    r = isr_reportx(rd->report, l->tids, (TID)-l->supp);
    if (r < 0) break;           /* report the current item set */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) free(proj);         /* delete the list and element arrays */
  return r;                     /* return the error status */
}  /* rec_tid() */

/*--------------------------------------------------------------------*/

int eclat_tid (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- eclat with trans. id lists */
  int        r = 0;             /* result of recursion/error status */
  ITEM       i, k, m, e;        /* loop variable, number of items */
  TID        n;                 /* number of transactions */
  size_t     x, z;              /* number of item instances */
  SUPP       w;                 /* weight/support buffer */
  SUPP       max;               /* maximum support of an item */
  SUPP       pex;               /* minimum support for perfect exts. */
  TRACT      *t;                /* to traverse transactions */
  TIDLIST    **lists, *l;       /* to traverse transaction id lists */
  TID        *tids, *p, **next; /* to traverse transaction ids */
  const ITEM *s;                /* to traverse transaction items */
  const TID  *c;                /* item occurrence counters */
  RECDATA    rd;                /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  k = tbg_itemcnt(tabag);       /* get and check the number of items */
  if (k <= 0) return isr_report(report);
  n = tbg_cnt(tabag);           /* get the number of transactions */
  c = tbg_icnts(tabag, 0);      /* and the number of containing */
  if (!c) return -1;            /* transactions per item */
  e = (mode & ECL_VERT) ? k   : 0;
  m = (mode & ECL_HORZ) ? k+1 : 0;
  x = (mode & ECL_FIM16) ? (size_t)n *sizeof(BITTA) : 0;
  z = (sizeof(ITEM) > sizeof(SUPP)) ? sizeof(ITEM) : sizeof(SUPP);
  lists = (TIDLIST**)malloc((size_t)(k+e) *sizeof(TIDLIST*)
                           +(size_t) k    *sizeof(TID*)
                           +(size_t)(n+n) *sizeof(SUPP)
                           +(size_t) m    *z +x);
  if (!lists) return -1;        /* create initial tid list array and */
  rd.elim  = lists +k;          /* split off the additional arrays */
  next     = (TID**) (rd.elim +e);
  rd.muls  = (SUPP*) (next    +k);
  rd.miss  = (SUPP*) (rd.muls +n); /* buffer for maximal() */
  rd.cand  = (ITEM*)  rd.miss;     /* buffer for closed() */
  rd.marks = (sizeof(ITEM) > sizeof(SUPP))
           ? (SUPP*)(rd.cand+m) : rd.miss+m;
  rd.btas  = (BITTA*)(rd.marks+n);
  memset(rd.marks, 0, (size_t)n *sizeof(TID));
  for (x = 0, i = 0; i < k; i++)/* traverse the items and sum */
    x += (size_t)c[i];          /* the number of item occurrences */
  /* Do not use tbg_extent(), because it does not take packed items */
  /* properly into account and thus may yield too big a value.      */
  if (x < (size_t)n) x = (size_t)n; /* ensure enough transaction ids */
  p = tids = (TID*)malloc((size_t)k *sizeof(TIDLIST) +x *sizeof(TID));
  if (!p) { free(lists); return -1; } /* allocate tid list elements */
  for (i = 0; i < k; i++) {     /* traverse the items / tid lists */
    lists[i] = l = (TIDLIST*)p; /* get next transaction id list */
    l->item  = i;               /* initialize the list item */
    l->supp  = 0;               /* and the support counter */
    next[i]  = p = l->tids;     /* note position of next trans. id */
    p += c[i]; *p++ = (TID)-1;  /* skip space for transaction ids */
  }                             /* and store a sentinel at the end */
  while (n > 0) {               /* traverse the transactions */
    t = tbg_tract(tabag, --n);  /* get the next transaction */
    rd.muls[n] = w = ta_wgt(t); /* and store its weight */
    for (s = ta_items(t); *s > TA_END; s++) {
      if ((i = *s) < 0) {       /* traverse the transaction's items */
        rd.btas[n] = (BITTA)i; i = 0; }
      lists[i]->supp += w;      /* traverse the transaction's items */
      *next[i]++      = n;      /* sum the transaction weight and */
    }                           /* collect the transaction ids */
  }
  rd.fim16 = NULL;              /* default: no 16-items machine */
  l = lists[i = 0];             /* get the list for packed items */
  if ((mode & ECL_FIM16)        /* if to use a 16-items machine */
  &&  (l->supp >= rd.smin)) {   /* and there are packed items */
    rd.fim16 = m16_create(rd.dir, rd.smin, report);
    if (!rd.fim16) { free(tids); free(lists); return -1; }
    l->item = -1; i = 1;        /* mark list for the packed items */
  }                             /* and add it to the reduced array */
  max = 0;                      /* init. the maximal item support */
  for (m = i; i < k; i++) {     /* traverse the items / tid lists */
    l = lists[i];               /* eliminate all infrequent items and */
    if (l->supp <  rd.smin) continue;   /* collect perfect extensions */
    if (l->supp >= pex) { isr_addpex(report, i); continue; }
    if (l->supp >  max)         /* find the maximal item support */
      max = l->supp;            /* (for later closed/maximal check) */
    lists[m++] = l;             /* collect lists for frequent items */
  }                             /* (eliminate infrequent items) */
  if (m > 0) {                  /* if there are frequent items */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = (mode & ECL_EXTCHK)     /* dep. on how to filter closed/max. */
      ? rec_tcm(lists, m, DIFFSIZE(p,tids), 0, &rd)
      : rec_tid(lists, m, DIFFSIZE(p,tids), &rd);
  }                             /* find freq. item sets recursively */
  if (r >= 0) {                 /* if no error occurred */
    i = target & (ISR_CLOSED|ISR_MAXIMAL);
    w = (i & ISR_MAXIMAL) ? rd.smin : tbg_wgt(tabag);
    if (!i || (max < w)) {      /* if to report the empty set */
      if (!isr_tidfile(report)) /* if not to report transaction ids, */
        r = isr_report(report); /* report the empty item set */
      else {                    /* if to report transaction ids */
        for (n = tbg_cnt(tabag); n > 0; n--) tids[n] = n;
        r = isr_reportx(report, tids, (TID)n);
      }                         /* report the empty item set */
    }                           /* with all transaction ids */
  }
  if (rd.fim16)                 /* if a 16-items machine was used, */
    m16_delete(rd.fim16);       /* delete the 16-items machine */
  free(tids); free(lists);      /* delete the allocated arrays */
  return r;                     /* return the error status */
}  /* eclat_tid() */

/*----------------------------------------------------------------------
  Eclat with Bit Vectors
----------------------------------------------------------------------*/

static int bit_cmp (const void *a, const void *b, void *data)
{                               /* --- compare support of tid lists */
  if (((BITVEC*)b)->supp > ((BITVEC*)a)->supp) return  1;
  if (((BITVEC*)b)->supp < ((BITVEC*)a)->supp) return -1;
  return 0;                     /* return sign of support difference */
}  /* bit_cmp() */

/*--------------------------------------------------------------------*/
#ifdef BITMAP_TABLE

static void bit_init (void)
{                               /* --- init. bit count/map tables */
  int i, k, b;                  /* loop variables, bit index */

  if (bitcnt[1] != 0) return;   /* check for an initialized table */
  for (i = 0; ++i < 256; )      /* traverse all byte values */
    for (k = i; k; k >>= 1)     /* traverse the bits in the value */
      bitcnt[i] += k & 1;       /* store their number in the table */
  memset(bitmap[0], 0, sizeof(bitmap[0]));
  for (k = 0; k < 256; ) { bitmap[1][k++] = 0; bitmap[1][k++] = 1; }
  for (i = 1; ++i < 255; ) {    /* traverse the matrix rows (masks) */
    for (b = 8; --b >= 0; ) {   /* traverse set bits of the mask */
      if (((i >> b) & 1) == 0) continue;
      for (k = 0; k < 256; ) {  /* traverse the matrix columns */
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
        bitmap[i][k] = (bitmap[i][k] << 1) | ((k >> b) & 1); k++;
      }                         /* collect the bits of the source */
    }                           /* that are under the mask bits */
  }                             /* for faster bit vector reduction */
  for (k = 0; k < 256; k++) bitmap[255][k] = (BITBLK)k;
}  /* bit_init() */

/*--------------------------------------------------------------------*/

static void bit_isect (BITVEC *dst, BITVEC *src1, BITVEC *src2, TID n)
{                               /* --- intersect two bit vectors */
  BITBLK *s1, *s2, *d;          /* to traverse sources and dest. */
  BITBLK s, m, o, x;            /* source, mask, and output blocks */
  int    b, c;                  /* number of bits in output */

  assert(dst && src1 && src2);  /* check the function arguments */
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = 0;                /* initialize the support */
  d = dst->bits; s1 = src1->bits; s2 = src2->bits;
  for (o = 0, b = 0; n > 0; n--) { /* traverse the bit vector blocks */
    s = *s1++; m = *s2++;       /* traverse the bytes of each block */
    for ( ; m != 0; s >>= 8, m >>= 8) {
      dst->supp += (SUPP)bitcnt[x = bitmap[m & 0xff][s & 0xff]];
      o |= x << b;         b += c = bitcnt[m & 0xff];
      if (b < 32) continue;     /* add output bits for current byte */
      b -= 32; *d++ = o;        /* if a bit block is full, store it */
      o = x >> (c-b-1) >> 1;    /* store remaining bits in buffer, */
    }                           /* but note that x >> 32 == x >> 0, */
  }                             /* so simply o = x >> (c-b) fails */
  if (b > 0) *d = o;            /* store the last bit vector block */
}  /* bit_isect() */

/*--------------------------------------------------------------------*/
#else

#define bit_init()              /* no initialization needed */

static void bit_isect (BITVEC *dst, BITVEC *src1, BITVEC *src2, TID n)
{                               /* --- intersect two bit vectors */
  BITBLK *s1, *s2, *d;          /* to traverse sources and dest. */
  BITBLK s, m, o;               /* source, mask, and output block */
  int    b;                     /* number of bits in output */

  assert(dst && src1 && src2);  /* check the function arguments */
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = 0;                /* initialize the support */
  d = dst->bits; s1 = src1->bits; s2 = src2->bits;
  for (o = 0, b = 0; n > 0; n--) { /* traverse the bit vector blocks */
    for (s = *s1++, m = *s2++; m != 0; ) {
      if (m & 1) {              /* if first of four mask bits is set */
        dst->supp += (SUPP)(s & 1); o |= (s & 1) << b;
        if (++b >= 32) { *d++ = o; o = 0; b = 0; }
      }                         /* copy the source bit to the output */
      s >>= 1; m >>= 1;         /* get the next source and mask bit */
      if (m & 1) {              /* if first of four mask bits is set */
        dst->supp += (SUPP)(s & 1); o |= (s & 1) << b;
        if (++b >= 32) { *d++ = o; o = 0; b = 0; }
      }                         /* copy the source bit to the output */
      s >>= 1; m >>= 1;         /* get the next source and mask bit */
      if (m & 1) {              /* if first of four mask bits is set */
        dst->supp += (SUPP)(s & 1); o |= (s & 1) << b;
        if (++b >= 32) { *d++ = o; o = 0; b = 0; }
      }                         /* copy the source bit to the output */
      s >>= 1; m >>= 1;         /* get the next source and mask bit */
      if (m & 1) {              /* if first of four mask bits is set */
        dst->supp += (SUPP)(s & 1); o |= (s & 1) << b;
        if (++b >= 32) { *d++ = o; o = 0; b = 0; }
      }                         /* copy the source bit to the output */
      s >>= 1; m >>= 1;         /* get the next source and mask bit */
    }                           /* collect the source bits */
  }                             /* for which a mask bit is set */
  if (b > 0) *d = o;            /* store the last bit block */
}  /* bit_isect() */

#endif
/*--------------------------------------------------------------------*/

static int rec_bit (BITVEC **vecs, ITEM k, TID n, RECDATA *rd)
{                               /* --- eclat recursion with bit vecs. */
  int    r;                     /* error status */
  ITEM   i, m, z;               /* loop variables */
  SUPP   pex;                   /* minimum support for perf. exts. */
  TID    len;                   /* length of (reduced) bit vectors */
  BITVEC **proj = NULL;         /* bit vectors of projected database */
  BITVEC *v, *d;                /* to traverse bit vectors */
  BITBLK *p;                    /* to traverse bit vector blocks */
  ITEM   *t;                    /* to collect the tail items */

  assert(vecs && (k > 0) && rd);/* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & ECL_TAIL) {    /* if to use tail to prune w/ repo. */
    t = isr_buf(rd->report);    /* collect the tail items in buffer */
    for (m = 0, i = k; --i >= 0; ) t[m++] = vecs[i]->item;
    r = isr_tail(rd->report, t, m);
    if (r) return r;            /* if tail need not be processed, */
  }                             /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (BITVEC**)malloc((size_t)k                *sizeof(BITVEC*)
                          + (size_t)k                *sizeof(BITVEC)
                          +((size_t)k*(size_t)(n-1)) *sizeof(BITBLK));
    if (!proj) return -1;       /* allocate bit vectors and array */
  }                             /* (memory for conditional databases) */
  if ((k > 4)                   /* if there are enough items left, */
  &&  (rd->mode & ECL_REORDER)) /* re-sort the items w.r.t. support */
    ptr_qsort(vecs, (size_t)k, +1, bit_cmp, NULL);
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    v = vecs[k];                /* traverse the remaining items */
    r = isr_add(rd->report, v->item, v->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if (proj && (k > 0)) {      /* if another item can be added */
      len = (TID)(v->supp+31) >> 5;    /* get new vector length */
      pex = (rd->mode & ECL_PERFECT) ? v->supp : SUPP_MAX;
      proj[m = 0] = d = (BITVEC*)(p = (BITBLK*)(proj +k+1));
      for (i = 0; i < k; i++) { /* traverse preceding vectors */
        bit_isect(d, vecs[i], v, n);
        if (d->supp < rd->smin) /* intersect transaction bit vectors */
          continue;             /* eliminate infrequent items */
        if (d->supp >= pex) {   /* collect perfect extensions */
          isr_addpex(rd->report, d->item); continue; }
        proj[++m] = d = (BITVEC*)(p = d->bits +len);
      }                         /* collect the remaining bit vectors */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_bit(proj, m, len, rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) free(proj);         /* delete bit vectors and array */
  return r;                     /* return the error status */
}  /* rec_bit() */

/*--------------------------------------------------------------------*/

int eclat_bit (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- eclat with bit vectors */
  int        r = 0;             /* result of recursion/error status */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        n;                 /* number of transactions */
  TID        x;                 /* number of item instances */
  SUPP       pex;               /* minimum support for perfect exts. */
  TRACT      *t;                /* to traverse transactions */
  BITVEC     **vecs, *v;        /* to traverse bit vectors */
  BITBLK     *p;                /* to traverse bit vector blocks */
  const ITEM *s;                /* to traverse transaction items */
  RECDATA    rd;                /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  bit_init();                   /* initialize the bit count table */
  x = (n + 31) >> 5;            /* and compute the bit vector size */
  vecs = (BITVEC**)malloc((size_t)k                *sizeof(BITVEC*)
                        + (size_t)k                *sizeof(BITVEC)
                        +((size_t)k*(size_t)(x-1)) *sizeof(BITBLK));
  if (!vecs) return -1;         /* create initial bit vector array */
  p = (BITBLK*)(vecs+k);        /* and get the bit vector memory */
  for (i = 0; i < k; i++) {     /* traverse the items / bit vectors */
    vecs[i] = v = (BITVEC*)p;   /* get/create the next bit vector */
    v->item = i;                /* initialize the bit vector item */
    v->supp = 0;                /* and the support counter */
    memset(v->bits, 0, (size_t)x *sizeof(BITBLK));
    p = v->bits +x;             /* clear all transaction bits and */
  }                             /* skip them to get the next vector */
  while (n > 0) {               /* traverse the transactions */
    t = tbg_tract(tabag, --n);  /* retrieve the next transaction */
    assert(ta_wgt(t) == 1);     /* transaction weight must be 1 */
    for (s = ta_items(t); *s > TA_END; s++) {
      v = vecs[*s];             /* traverse the transaction's items */
      v->supp += 1;             /* sum/count the transaction weight */
      v->bits[n >> 5] |= (BITBLK)(1 << (n & 0x1f));
    }                           /* set the bit for the current trans. */
  }                             /* to indicate that item is contained */
  for (i = m = 0; i < k; i++) { /* traverse the items / bit vectors */
    v = vecs[i];                /* eliminate all infrequent items and */
    if (v->supp <  rd.smin) continue;   /* collect perfect extensions */
    if (v->supp >= pex) { isr_addpex(report, i); continue; }
    vecs[m++] = v;              /* collect vectors for frequent items */
  }                             /* (eliminate infrequent items) */
  if (m > 0) {                  /* if there are frequent items */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = rec_bit(vecs, m, x, &rd);
  }                             /* find freq. items sets recursively */
  if (r >= 0)                   /* if no error occurred, */
    r = isr_report(report);     /* report the empty item set */
  free(vecs);                   /* delete the allocated bit vectors */
  return r;                     /* return the error status */
}  /* eclat_bit() */

/*----------------------------------------------------------------------
  Eclat with an Occurrence Indicator Table
----------------------------------------------------------------------*/

static int rec_tab (TIDLIST **lists, ITEM k, size_t x, RECDATA *rd)
{                               /* --- eclat recursion with table */
  int     r;                    /* error status */
  ITEM    i, m, z;              /* loop variables */
  SUPP    pex;                  /* minimum support for perfect exts. */
  TIDLIST *l, *d;               /* to traverse transaction id lists */
  TIDLIST **proj = NULL;        /* trans. id lists of proj. database */
  ITEM    *t;                   /* to collect the tail items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & ECL_TAIL) {    /* if to use tail to prune w/ repo. */
    t = isr_buf(rd->report);    /* collect the tail items in buffer */
    for (m = 0, i = k; --i >= 0; ) t[m++] = lists[i]->item;
    r = isr_tail(rd->report, t, m);
    if (r) return r;            /* if tail need not be processed, */
  }                             /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TIDLIST**)malloc((size_t)k *sizeof(TIDLIST*) +x);
    if (!proj) return -1;       /* allocate list and element arrays */
  }                             /* (memory for projected database) */
  if ((k > 4)                   /* if there are enough items left, */
  &&  (rd->mode & ECL_REORDER)) /* re-sort the items w.r.t. support */
    ptr_qsort(lists, (size_t)k, +1, tid_cmp, NULL);
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    l = lists[k];               /* traverse the items / tid lists */
    r = isr_add(rd->report, l->item, l->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if (proj && (k > 0)) {      /* if another item can be added */
      pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
      proj[m = 0] = d = (TIDLIST*)(proj +k+1);
      for (i = 0; i < k; i++) { /* traverse the preceding lists */
        x = (size_t)filter(d, lists[i], rd->tab[l->item]);
        if (d->supp < rd->smin) /* filter transaction id list */
          continue;             /* eliminate infrequent items */
        if (d->supp >= pex) {   /* collect perfect extensions */
          isr_addpex(rd->report, d->item); continue; }
        proj[++m] = d = (TIDLIST*)(d->tids +x);
      }                         /* collect tid lists of freq. items */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_tab(proj, m, DIFFSIZE(d,proj[0]), rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }
    r = isr_reportx(rd->report, l->tids, (TID)-l->supp);
    if (r < 0) break;           /* report the current item set */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) free(proj);         /* delete the list and element arrays */
  return r;                     /* return the error status */
}  /* rec_tab() */

/*--------------------------------------------------------------------*/

int eclat_tab (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- eclat with occurrence table */
  int        r = 0;             /* result of recursion/error status */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        n;                 /* number of transactions */
  size_t     x;                 /* number of item instances */
  SUPP       w;                 /* weight/support buffer */
  SUPP       max;               /* maximum support of an item */
  SUPP       pex;               /* minimum support for perfect exts. */
  SUPP       *d;                /* to traverse occurrence table rows */
  TRACT      *t;                /* to traverse transactions */
  TIDLIST    **lists, *l;       /* to traverse transaction id lists */
  TID        *tids, *p, **next; /* to traverse transaction ids */
  const ITEM *s;                /* to traverse transaction items */
  const TID  *c;                /* item occurrence counters */
  RECDATA    rd;                /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  x = tbg_extent(tabag);        /* get the number of item instances */
  c = tbg_icnts(tabag, 0);      /* and the number of containing */
  if (!c) return -1;            /* transactions per item */
  if ((SIZE_MAX/sizeof(SUPP) -x) / (size_t)(n+4) < (size_t)k)
    return -1;                  /* check the table size */
  lists = (TIDLIST**)malloc((size_t) k              *sizeof(TIDLIST*)
                           +(size_t) k              *sizeof(TID*)
                           +(size_t) k              *sizeof(SUPP*)
                           +(size_t)(k+1)*(size_t)n *sizeof(SUPP));
  if (!lists) return -1;        /* create initial tid list array */
  next     = (TID**) (lists +k);/* and split off arrays */
  rd.tab   = (SUPP**)(next  +k);/* get item occ. table header */
  rd.muls  = (SUPP*) (rd.tab+k);/* split off trans. weight array */
  d = (SUPP*)memset(rd.muls +n, 0, (size_t)k*(size_t)n *sizeof(SUPP));
  if (x < (size_t)n) x = (size_t)n; /* ensure enough transaction ids */
  p = tids = (TID*)malloc((size_t)k *sizeof(TIDLIST) +x *sizeof(TID));
  if (!p) { free(lists); return -1; }
  for (i = 0; i < k; i++) {     /* traverse the items / tid lists */
    rd.tab[i] = d; d += n;      /* organize the table rows */
    lists[i] = l = (TIDLIST*)p; /* get/create the next trans. id list */
    l->item  = i;               /* initialize the list item */
    l->supp  = 0;               /* and the support counter */
    next[i]  = p = l->tids;     /* note position of next trans. id */
    p += c[i]; *p++ = (TID)-1;  /* skip space for transaction ids */
  }                             /* and store a sentinel at the end */
  while (n > 0) {               /* traverse the transactions */
    t = tbg_tract(tabag, --n);  /* get the next transaction */
    rd.muls[n] = w = ta_wgt(t); /* and store its weight */
    for (s = ta_items(t); *s > TA_END; s++) {
      rd.tab[*s][n]    = w;     /* traverse the transaction's items */
      lists[*s]->supp += w;     /* and set the item occurrence flags */
      *next[*s]++      = n;     /* sum the transaction weight and */
    }                           /* collect the transaction ids */
  }
  max = 0;                      /* init. the maximal item support */
  for (i = m = 0; i < k; i++) { /* traverse the items / tid lists */
    l = lists[i];               /* eliminate all infrequent items and */
    if (l->supp <  rd.smin) continue;   /* collect perfect extensions */
    if (l->supp >= pex) { isr_addpex(report, i); continue; }
    if (l->supp >  max)         /* find the maximal item support */
      max = l->supp;            /* (for later closed/maximal check) */
    lists[m++] = l;             /* collect lists for frequent items */
  }                             /* (eliminate infrequent items) */
  if (m > 0) {                  /* if there are frequent items */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = rec_tab(lists, m, DIFFSIZE(p,tids), &rd);
  }                             /* find freq. item sets recursively */
  if (r >= 0) {                 /* if no error occurred */
    i = target & (ISR_CLOSED|ISR_MAXIMAL);
    w = (i & ISR_MAXIMAL) ? rd.smin : tbg_wgt(tabag);
    if (!i || (max < w)) {      /* if to report the empty set */
      if (!isr_tidfile(report)) /* if not to report transaction ids, */
        r = isr_report(report); /* report the empty item set */
      else {                    /* if to report transaction ids */
        for (n = tbg_cnt(tabag); n > 0; n--) tids[n] = n;
        r = isr_reportx(report, tids, (TID)n);
      }                         /* report the empty item set */
    }                           /* with all transaction ids */
  }
  free(tids); free(lists);      /* delete the allocated arrays */
  return r;                     /* return the error status */
}  /* eclat_tab() */

/*----------------------------------------------------------------------
  Eclat with an Occurrence Indicator Table (Simplified)
----------------------------------------------------------------------*/

static int rec_simp (TID *tids, SUPP n, ITEM k, RECDATA *rd)
{                               /* --- eclat recursion (table based) */
  int  r;                       /* error status */
  ITEM z;                       /* loop variable */
  SUPP s, w;                    /* item set support, weight buffer */
  SUPP pex;                     /* trans. count for perfect exts. */
  TID  *dst, *d, *p;            /* to traverse transaction ids */
  SUPP *row;                    /* to traverse occurrence table rows */

  assert(tids                   /* check the function arguments */
  &&    (n > 0) && (k > 0) && rd);
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  pex = (rd->mode & ECL_PERFECT) ? n : SUPP_MAX;
  dst = tids +(TID)n+1;         /* get destination for intersections */
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir){ /* traverse the remaining items */
    row = rd->tab[k]; s = 0;    /* filter tids with item's table row */
    for (d = dst, p = tids; *p >= 0; p++)
      if ((w = row[*p]) > 0) {  /* compute the item set support */
        s += w; *d++ = *p; }    /* and the reduced trans. id list */
    if (s < rd->smin) continue; /* skip infrequent items and */
    if ((w = (SUPP)(d-dst)) >= pex) { /* collect perfect extensions */
      isr_addpex(rd->report, k); continue; }
    *d = -1;                    /* store a sentinel at the list end */
    r  = isr_add(rd->report, k, s);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if ((k > 0)                 /* if another item can be added */
    &&  isr_xable(rd->report,1) /* and upper size limit not reached */
    &&  ((r = rec_simp(dst, w, k, rd)) < 0))
      break;                    /* recursively find freq. item sets */
    r = isr_reportx(rd->report, tids, (TID)-s);
    if (r < 0) break;           /* report the current item set */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  return r;                     /* return the error status */
}  /* rec_simp() */

/*----------------------------------------------------------------------
Note that no memory is allocated in the above function; all processing
is done in the single memory block that is allocated in the function
below. The size of this memory block is O(n*k), where n is the number
of items and k the number of transactions. Additional memory is only
allocated in the item set reporter if closed or maximal item sets are
to be found, since this requires setting up an item set repository.
----------------------------------------------------------------------*/

int eclat_simp (TABAG *tabag, int target, SUPP smin, int mode,
                ISREPORT *report)
{                               /* --- eclat with occurrence table */
  int        r = 0;             /* result of recursion/error status */
  ITEM       i, k;              /* loop variable, number of items */
  TID        n, m;              /* number of transactions */
  size_t     x;                 /* number of item instances */
  SUPP       w;                 /* weight/support buffer */
  TID        *tids;             /* transaction identifier array */
  SUPP       *p;                /* to traverse occurrence table rows */
  const ITEM *s;                /* to traverse transaction items */
  TRACT      *t;                /* to traverse the transactions */
  RECDATA    rd;                /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = -1;               /* (supports only downward currently) */
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  if (tbg_wgt(tabag) < rd.smin) /* check the total transaction weight */
    return 0;                   /* against the minimum support */
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  x = tbg_extent(tabag);        /* get the number of item instances */
  if ((SIZE_MAX/sizeof(TID) -x-(size_t)n-1) / (size_t)(n+2) < (size_t)k)
    return -1;                  /* check the database/table size */
  x += (size_t)n+1+(size_t)k;   /* compute the database/table size */
  rd.tab = (SUPP**)malloc((size_t)k           *sizeof(SUPP*)
                        + (size_t)k*(size_t)n *sizeof(SUPP)
                        +         x           *sizeof(TID));
  if (!rd.tab) return -1;       /* allocate working memory */
  p = (SUPP*)memset(rd.tab +k, 0, (size_t)k*(size_t)n *sizeof(SUPP));
  for (i = 0; i < k; i++) {     /* init and organize the table rows */
    rd.tab[i] = p; p += n; }    /* (one table row per item) */
  tids = (TID*)p;               /* get the transaction id array */
  for (m = 0; m < n; m++) {     /* traverse the transactions */
    tids[m] = m;                /* set the initial (full) tid set */
    t = tbg_tract(tabag, m);    /* get the next transaction */
    w = ta_wgt(t);              /* and note its weight */
    for (s = ta_items(t); *s > TA_END; s++)
      rd.tab[*s][m] = w;        /* set the item occurrence flags */
  }                             /* (item *s occurs in transaction i) */
  tids[n] = (TID)-1;            /* store a sentinel at the end */
  if (isr_xable(report, 1)) {   /* if item set may be extended */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = rec_simp(tids, (SUPP)n, k, &rd);
  }                             /* recursively find freq. item sets */
  if (r >= 0)                   /* if no error occurred, */
    r = isr_report(report);     /* report the empty item set */
  free(rd.tab);                 /* delete the allocated table/arrays */
  return r;                     /* return the error status */
}  /* eclat_simp() */

/*----------------------------------------------------------------------
  Eclat with Transaction Ranges
----------------------------------------------------------------------*/

static void build_trg (TRGLIST **lists, TIDRANGE **next,
                       TABAG *tabag, TID min, TID max, ITEM off)
{                               /* --- build the trans. range lists */
  ITEM     i;                   /* loop variable */
  TID      k;                   /* loop variable */
  SUPP     w;                   /* weight buffer */
  ITEM     item;                /* to traverse items at offset */
  TRGLIST  *l;                  /* to access the trans. range lists */
  TIDRANGE *r;                  /* to access the transaction ranges */
  TRACT    *t;                  /* to traverse the transactions */

  assert(lists && tabag         /* check the function arguments */
  &&    (min >= 0) && (max < (TID)tbg_cnt(tabag)) && (off >= 0));

  /* --- skip short transactions --- */
  while ((min <= max)           /* traverse the transactions */
  &&     (ta_items(tbg_tract(tabag, min))[off] <= TA_END))
    ++min;                      /* skip trans. that are too short */
  if (min > max) return;        /* check for an empty trans. range */

  /* --- handle packed items --- */
  if (off <= 0) {               /* if at first item in transactions */
    l = lists[0];               /* get the list for packed items */
    for (k = min; min <= max; min++) {
      t = tbg_tract(tabag,min); /* traverse the transactions */
      i = ta_items(t)[off];     /* get the first item from them */
      if (i >= 0) break;        /* if it is not packed, abort loop */
      r = next[0]++;            /* get the current range in list */
      r->min = min;             /* store the transaction id and */
      r->max = (TID)(BITTA)i;   /* the bit repr. of the items */
      l->supp += r->wgt = ta_wgt(t);
    }                           /* store and sum transaction weight */
    if (min > k) {              /* if the trans. range is not empty */
      build_trg(lists, next, tabag, k, min-1, off+1);
      if (min > max) return;    /* recursively build trans. ranges, */
    }                           /* check whether an empty range */
  }                             /* is left to be processed */

  /* --- handle normal items --- */
  t = tbg_tract(tabag, min);    /* get the first transaction */
  i = item = ta_items(t)[off];  /* and from it the first item */
  do {                          /* traverse the longer transactions */
    w = ta_wgt(t);              /* init. the transaction weight */
    for (k = min; ++min <= max; ) {  /* while not at end of section */
      t = tbg_tract(tabag,min); /* get the next transaction and */
      i = ta_items(t)[off];     /* from it the item at the offset */
      if (i != item) break;     /* if the item differs, abort loop */
      w += ta_wgt(t);           /* otherwise sum the trans. weight */
    }                           /* (collect trans. with same item) */
    l = lists[item];            /* get list for the current item */
    r = next[item]++; item = i; /* and create a new trans. id range */
    l->supp += r->wgt = w;      /* store the transaction weights */
    build_trg(lists, next, tabag, r->min = k, r->max = min-1, off+1);
  } while (min <= max);         /* create the children recursively */
}  /* build_trg() */            /* while the range is not empty */

/*--------------------------------------------------------------------*/

static TID isect_trg (TRGLIST *dst, TRGLIST *src1, TRGLIST *src2)
{                               /* --- intersect two range lists */
  TIDRANGE *s1, *s2, *d, *p;    /* to traverse sources and dest. */

  assert(dst && src1 && src2);  /* check the function arguments */
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = 0;                /* initialize the support */
  s1 = src1->trgs; s2 = src2->trgs; d = dst->trgs-1; p = NULL;
  while (1) {                   /* range list intersection loop */
    if      (s1->max < s2->min) {    /* skip transaction ranges */
      if ((++s1)->min < 0) break; }  /* that do not overlap */
    else if (s2->max < s1->min) {    /* check for the sentinel */
      if ((++s2)->min < 0) break; }  /* after advancing the range */
    else {                      /* if the transaction ranges overlap */
      if (s1 == p)              /* if there was a previous overlap, */
        d->wgt += s2->wgt;      /* only add the additional support */
      else {                    /* if this is a new overlap, */
        p = s1; ++d;            /* get/create a new trans. range */
        d->min = s1->min;       /* note the minimum and the */
        d->max = s1->max;       /* maximum transaction identifier */
        d->wgt = s2->wgt;       /* and the corresponding support */
      }
      dst->supp += s2->wgt;     /* sum the support for the item */
      if ((++s2)->min < 0) break;
    }                           /* skip the processed trans. range */
  }                             /* and check for the sentinel */
  (++d)->min = -1;              /* store a sentinel at the list end */
  return (TID)(++d -dst->trgs); /* return the size of the new list */
}  /* isect_trg() */

/*--------------------------------------------------------------------*/

static TID filter_trg (TRGLIST *dst, TRGLIST *src1, TRGLIST *src2)
{                               /* --- filter tids with a range list */
  TIDRANGE *s1, *s2, *d;        /* to traverse sources and dest. */

  assert(dst && src1 && src2);  /* check the function arguments */
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = 0;                /* initialize the support */
  s1 = src1->trgs; s2 = src2->trgs; d = dst->trgs-1;
  while (1) {                   /* transaction id filtering loop */
    if      (s1->min < s2->min) {    /* skip transaction ids */
      if ((++s1)->min < 0) break; }  /* that are not in range */
    else if (s1->min > s2->max) {    /* check for the sentinel */
      if ((++s2)->min < 0) break; }  /* after advancing the range */
    else {                      /* if transaction id is in range */
      *++d = *s1;               /* copy the entry to the dest. */
      dst->supp += s1->wgt;     /* sum the support for the item */
      if ((++s1)->min < 0) break;
    }                           /* check for the sentinel */
  }
  (++d)->min = -1;              /* store a sentinel at the list end */
  return (TID)(++d -dst->trgs); /* return the size of the new list */
}  /* filter_trg() */

/*--------------------------------------------------------------------*/

static int rec_trg (TRGLIST **lists, ITEM k, size_t x, RECDATA *rd)
{                               /* --- eclat recursion with ranges */
  int      r;                   /* error status */
  ITEM     i, m, z;             /* loop variables */
  SUPP     pex;                 /* minimum support for perfect exts. */
  TRGLIST  **proj = NULL;       /* range lists of projected database */
  TRGLIST  *l, *d;              /* to traverse trans. range lists */
  TIDRANGE *p;                  /* to traverse transaction ranges */
  ITEM     *t;                  /* to collect the tail items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & ECL_TAIL) {    /* if to use tail to prune w/ repo. */
    t = isr_buf(rd->report);    /* collect the tail items in buffer */
    for (m = 0, i = k; --i >= 0; ) t[m++] = lists[i]->item;
    r = isr_tail(rd->report, t, m);
    if (r) return r;            /* if tail need not be processed, */
  }                             /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TRGLIST**)malloc((size_t)k *sizeof(TRGLIST*) +x);
    if (!proj) return -1;       /* allocate list and element arrays */
  }                             /* (memory for projected database) */
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    l = lists[k];               /* traverse the items / range lists */
    if (l->item < 0) {          /* if this list is for packed items */
      for (p = l->trgs; p->min >= 0; p++)
        m16_add(rd->fim16, (BITTA)p->max, p->wgt);
      r = m16_mine(rd->fim16);  /* add bit-rep. transaction prefixes */
      if (r >= 0) continue;     /* to the 16-items machine and mine, */
      if (proj) free(proj);     /* then go to the next tid range list */
      return r;                 /* otherwise free allocated memory */
    }                           /* and abort the function */
    r = isr_add(rd->report, l->item, l->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if (proj && (k > 0)) {      /* if another item can be added */
      pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
      proj[m = 0] = d = (TRGLIST*)(proj +k+1);
      if (lists[i = 0]->item < 0) { /* if there are packed items */
        x = (size_t)filter_trg(d, lists[i++], l);
        if (d->supp >= rd->smin)    /* if they are frequent */
          proj[++m] = d = (TRGLIST*)(d->trgs +x);
      }                         /* add a range list for packed items */
      for ( ; i < k; i++) {     /* traverse the preceding lists */
        x = (size_t)isect_trg(d, lists[i], l);
        if (d->supp < rd->smin) /* intersect tid range lists */
          continue;             /* eliminate infrequent items */
        if (d->supp >= pex) {   /* collect perfect extensions */
          isr_addpex(rd->report, d->item); continue; }
        proj[++m] = d = (TRGLIST*)(d->trgs +x);
      }                         /* collect the trans. range lists */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_trg(proj, m, DIFFSIZE(d,proj[0]), rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) free(proj);         /* delete the list and element arrays */
  return r;                     /* return the error status */
}  /* rec_trg() */

/*--------------------------------------------------------------------*/

int eclat_trg (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- eclat with transaction ranges */
  int       r = 0;              /* result of recursion/error status */
  ITEM      i, k, m;            /* loop variable, number of items */
  TID       n;                  /* number of transactions */
  size_t    x;                  /* number of item instances */
  SUPP      pex;                /* minimum support for perfect exts. */
  TRGLIST   **lists, *l;        /* to traverse trans. range lists */
  TIDRANGE  *trgs, *p, **next;  /* to traverse transaction ranges */
  const TID *c;                 /* item occurrence counters */
  RECDATA   rd;                 /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  c = tbg_icnts(tabag, 0);      /* get the number of containing */
  if (!c) return -1;            /* transactions per item */
  lists = (TRGLIST**)malloc((size_t)k *sizeof(TRGLIST*)
                           +(size_t)k *sizeof(TIDRANGE*));
  if (!lists) return -1;        /* create initial range lists array */
  next = (TIDRANGE**)(lists+k); /* and split off next range array */
  for (x = 0, i = 0; i < k; i++)/* traverse the items and sum */
    x += (size_t)c[i];          /* the number of item occurrences */
  /* Do not use tbg_extent(), because it does not take packed items */
  /* properly into account and thus may yield too big a value.      */
  p = trgs = (TIDRANGE*)malloc((size_t)k *sizeof(TRGLIST)
                                      +x *sizeof(TIDRANGE));
  if (!p) { free(lists); return -1; }
  for (i = 0; i < k; i++) {     /* allocate range list elements */
    lists[i] = l = (TRGLIST*)p; /* and organize the range lists */
    l->item  = i;               /* initialize the list item */
    l->supp  = 0;               /* and the support counter */
    next[i]  = p = l->trgs;     /* note position of next trans. id */
    p += c[i];                  /* skip space for transaction ids */
    (p++)->min = (TID)-1;       /* and store a sentinel at the end */
  }
  build_trg(lists, next, tabag, 0, (TID)(n-1), 0);
  rd.fim16 = NULL;              /* build the transaction ranges */
  l = lists[i = 0];             /* get the list for packed items */
  if ((l->supp >= rd.smin)      /* if there are packed items */
  &&  (mode & ECL_FIM16)) {     /* and to use a 16-items machine */
    rd.fim16 = m16_create(rd.dir, rd.smin, report);
    if (!rd.fim16) { free(trgs); free(lists); return -1; }
    next[i++]->min = (TID)-1;   /* store a sentinel at the list end */
    l->item        = -1;        /* mark list for the packed items */
  }                             /* and store it in the reduced array */
  for (m = i; i < k; i++) {     /* traverse the trans. range lists */
    l = lists[i];               /* eliminate all infrequent items and */
    if (l->supp <  rd.smin) continue;   /* collect perfect extensions */
    if (l->supp >= pex) { isr_addpex(report, i); continue; }
    next[i]->min = (TID)-1;     /* store a sentinel at the list end */
    lists[m++] = l;             /* collect lists for frequent items */
  }                             /* (eliminate infrequent items) */
  if (m > 0) {                  /* if there are frequent items */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = rec_trg(lists, m, DIFFSIZE(p,trgs), &rd);
  }                             /* find freq. items sets recursively */
  if (r >= 0)                   /* if no error occurred, */
    r = isr_report(report);     /* report the empty item set */
  if (rd.fim16)                 /* if a 16-items machine was used, */
    m16_delete(rd.fim16);       /* delete the 16-items machine */
  free(trgs); free(lists);      /* delete the allocated arrays */
  return r;                     /* return the error status */
}  /* eclat_trg() */

/*----------------------------------------------------------------------
  Eclat with Occurrence Deliver (LCM-style)
----------------------------------------------------------------------*/

static int odclo (TALIST *list, ITEM item, RECDATA *rd)
{                               /* --- check for a closed item set */
  TID        k;                 /* loop variable */
  const ITEM *p, *q;            /* to traverse transaction items */
  ITEM       *s, *d;            /* to traverse collected items */
  int        i, m;              /* loop variable, bit mask for items */

  assert(list && rd             /* check the function arguments */
  &&    (item >= 0) && (list->cnt >= 1));
  if (item < 31) {              /* if item bits can be used */
    for (m = 0, i = item; ++i < 32; )
      if (!isr_uses(rd->report, (ITEM)i))
        m |= 1 << i;            /* collect bits of unused items */
    for (k = list->cnt; m && (--k >= 0); )
      m &= ta_getmark(list->tracts[k]);
    if (m) return 0;            /* if perf. ext found, 'not closed' */
    item = 31;                  /* otherwise check remaining items */
  }                             /* (check only items 32 and above) */
  p = ta_items(list->tracts[0]);/* get first and last transaction */
  q = ta_items(list->tracts[list->cnt-1]);
  while ((UITEM)*p <= (UITEM)item) p++;  /* find item added last */
  while ((UITEM)*q <= (UITEM)item) q++;  /* in both transactions */
  for (d = rd->cand; (*p >= 0) && (*q >= 0); ) {
    if      ((UITEM)*p < (UITEM)*q) p++; /* intersection loop: */
    else if ((UITEM)*p > (UITEM)*q) q++; /* skip smaller items */
    else { if (!isr_uses(rd->report, *p)) *d++ = *p; p++; q++; }
  }                             /* collect perfect ext. candidates */
  if (d <= rd->cand) return -1; /* if intersection is empty, abort, */
  *d = TA_END;                  /* terminate the candidate list */
  for (k = list->cnt-1; --k > 0; ) {
    s = d = rd->cand;           /* traverse the transactions */
    p = ta_items(list->tracts[k]);
    while (*p <= item) p++;     /* find the item added last */
    while (1) {                 /* item list intersection loop */
      if      (*s < 0) break;   /* check for the list sentinel */
      else if ((UITEM)*s < (UITEM)*p) s++;
      else if ((UITEM)*s > (UITEM)*p) p++;
      else { *d++ = *s++; p++; }/* skip smaller item identifier and */
    }                           /* collect perfect ext. candidates */
    if (d <= rd->cand) return -1;
    *d = TA_END;                /* if intersection is empty, abort, */
  }                             /* otherwise store a sentinel */
  return 0;                     /* return 'item set is not closed' */
}  /* odclo() */

/*--------------------------------------------------------------------*/

static int odmax (TALIST *list, ITEM item, RECDATA *rd)
{                               /* --- check for a maximal item set */
  TID        k;                 /* loop variable */
  ITEM       n;                 /* number of considered items */
  SUPP       w;                 /* transaction weight */
  TRACT      *t;                /* to traverse the transactions */
  const ITEM *p;                /* to traverse the items */

  assert(list && rd             /* check the function arguments */
  &&    (item >= 0) && (list->cnt >= 1));
  for (n = tbg_itemcnt(rd->tabag); --n > item; )
    rd->miss[n] = (isr_uses(rd->report, n)) ? list->supp+1 : rd->smin;
  for (k = list->cnt; --k >= 0; ) {
    t = list->tracts[k];        /* traverse the transactions */
    w = ta_wgt(t);              /* and the items and each trans. */
    for (p = ta_items(t); *p != TA_END; p++)
      if ((*p > item) && ((rd->miss[*p] -= w) <= 0))
        return 0;               /* count support of candidate exts.; */
  }                             /* if frequent cand. found, abort */
  return -1;                    /* return 'item set is maximal' */
}  /* odmax() */

/*--------------------------------------------------------------------*/

static SUPP rec_odcm (TALIST **lists, ITEM k, RECDATA *rd)
{                               /* --- occ. deliver closed/maximal */
  int        r;                 /* error status */
  ITEM       i, m;              /* loop variables */
  TID        n;                 /* loop variable for transactions */
  SUPP       w;                 /* weight/support buffer */
  SUPP       max;               /* maximum support of an ext. item */
  SUPP       pex;               /* minimum support for perfect exts. */
  TALIST     *l, *p;            /* to traverse transaction lists */
  TRACT      *t;                /* to traverse transactions */
  const ITEM *s;                /* to traverse items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  l = lists[k]; m = 0;          /* get the trans. list to process */
  for (n = 0; n < l->cnt; n++){ /* traverse the transactions */
    t = l->tracts[n];           /* get the next transaction */
    w = ta_wgt(t);              /* and traverse its items */
    for (s = ta_items(t); (UITEM)*s < (UITEM)k; s++) {
      p = lists[*s]; p->supp += w; p->tracts[p->cnt++] = t; }
  }                             /* deliver the item occurrences */
  pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
  for (i = 0; i < k; i++) {     /* traverse the items / trans. lists */
    p = lists[i];               /* get the next transaction list */
    if (p->supp <  rd->smin) {  /* eliminate infrequent items */
      p->supp = 0; p->cnt = 0; continue; }
    if (p->supp >= pex) {       /* collect perfect extension items */
      p->supp = 0; p->cnt = 0; isr_addpex(rd->report, i); continue; }
    m++;                        /* count the frequent items */
  }                             /* (to see whether loop is needed) */
  if (m <= 0) return 0;         /* if no frequent items found, abort */
  m = isr_xable(rd->report, 2) ? 0 : ITEM_MAX;
  for (max = w = 0, i = 0; i < k; i++) {
    p = lists[i];               /* traverse the items / trans. lists, */
    if (p->supp <= 0) continue; /* but skip all eliminated items */
    if (p->supp > max)          /* find maximal extension support */
      max = p->supp;            /* (for later closed/maximal check) */
    if (odclo(p, i, rd)) {      /* if the current set may be closed */
      r = isr_add(rd->report, i, p->supp);
      if (r < 0) break;         /* add current item to the reporter */
      w = 0;                    /* default: no perfect extension */
      if (i > m) {              /* if to compute a projection */
        w = rec_odcm(lists, i, rd);
        if (w < 0) { r = -1; break; }
      }                         /* recursively find freq. item sets */
      if ((rd->target & ISR_CLOSED) ? (w < p->supp)
      :   ((w < rd->smin) && odmax(p, i, rd))) {
        r = isr_report(rd->report);
        if (r < 0) break;       /* if current set is closed/maximal, */
      }                         /* report the current item set */
      isr_remove(rd->report,1); /* remove the current item */
    }                           /* from the item set reporter */
    p->supp = 0; p->cnt = 0;    /* reinitialize the transaction list */
  }
  return (r < 0) ? (SUPP)r : max; /* return error status/max. support */
}  /* rec_odcm() */

/*--------------------------------------------------------------------*/

static int rec_odfx (TALIST **lists, ITEM k, RECDATA *rd)
{                               /* --- occ. deliver w/o reordering */
  int        r;                 /* error status */
  ITEM       i, m;              /* loop variables */
  TID        n;                 /* loop variable for transactions */
  SUPP       w;                 /* weight/support buffer */
  SUPP       pex;               /* minimum support for perfect exts. */
  TALIST     *l, *p;            /* to traverse transaction lists */
  TRACT      *t;                /* to traverse transactions */
  const ITEM *s;                /* to traverse items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  l = lists[k];                 /* collate equal transactions */
  taa_collate(l->tracts, l->cnt, k);
  for (n = 0; n < l->cnt; n++){ /* traverse the transactions, */
    t = l->tracts[n];           /* but skip collated transactions */
    if ((w = ta_wgt(t)) <= 0) continue;
    s = ta_items(t);            /* if there are packed items, */
    if (ispacked(*s))           /* add them to the 16-items machine */
      m16_add(rd->fim16, (BITTA)*s++, w);
    for ( ; (UITEM)*s < (UITEM)k; s++) {
      p = lists[*s]; p->supp += w; p->tracts[p->cnt++] = t; }
  }                             /* deliver the item occurrences */
  pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
  for (m = 0, i = rd->first; i < k; i++) {
    p = lists[i];               /* traverse the items / trans. lists */
    if (p->supp <  rd->smin) {  /* eliminate infrequent items */
      p->supp = 0; p->cnt = 0; continue; }
    if (p->supp >= pex) {       /* collect perfect extension items */
      p->supp = 0; p->cnt = 0; isr_addpex(rd->report, i); continue; }
    m++;                        /* count the frequent items */
  }                             /* (to see whether loop is needed) */
  r = (rd->fim16)               /* if there is a 16-items machine, */
    ? m16_mine(rd->fim16) : 0;  /* execute the 16-items machine */
  if (m <= 0) {                 /* if no frequent items found, abort */
    taa_uncoll(l->tracts, l->cnt); return r; }
  m = isr_xable(rd->report, 2) ? 0 : ITEM_MAX;
  for (i = rd->first; i < k; i++) {
    p = lists[i];               /* traverse the items / trans. lists, */
    if (p->supp <= 0) continue; /* but skip all eliminated items */
    r = isr_add(rd->report, i, p->supp);
    if (r < 0) break;           /* add current item to the reporter */
    if (r > 0) {                /* if the item needs processing */
      if (i > m) {              /* if to compute a projection */
        r = rec_odfx(lists, i, rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* and check for a recursion error */
      r = isr_report(rd->report);
      if (r < 0) break;         /* report the current item set */
      isr_remove(rd->report,1); /* remove the current item */
    }                           /* from the item set reporter */
    p->supp = 0; p->cnt = 0;    /* reinitialize the transaction list */
  }
  taa_uncoll(l->tracts,l->cnt); /* uncollate the transactions */
  return r;                     /* and return the error status */
}  /* rec_odfx() */

/*--------------------------------------------------------------------*/

static int rec_odro (TALIST **lists, ITEM k, RECDATA *rd)
{                               /* --- occ. deliver with reordering */
  int        r;                 /* error status */
  ITEM       i, m, b;           /* loop variables */
  TID        n;                 /* number of transactions */
  size_t     x;                 /* number of item instances */
  SUPP       w;                 /* weight/support buffer */
  SUPP       pex;               /* minimum support for perfect exts. */
  TALIST     *l, *p;            /* to traverse transaction lists */
  TRACT      *t;                /* to traverse transactions */
  const ITEM *s;                /* to traverse items */
  SUPP       *supp;             /* item support array */
  ITEM       *map, *inv;        /* item identifier maps */
  TALIST     **dst;             /* destination for reduction */
  void       *mem = NULL;       /* memory for reduction */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  supp = (SUPP*)memset(rd->muls, 0, (size_t)k *sizeof(SUPP));
  l    = lists[k];              /* initialize the item support array */
  for (x = 0, n = 0; n < l->cnt; n++) {
    t = l->tracts[n];           /* traverse the transactions */
    w = ta_wgt(t);              /* get the transaction weight */
    for (s = ta_items(t); (UITEM)*s < (UITEM)k; s++)
      supp[*s] += w;            /* determine the support of the items */
    x += (size_t)(s -ta_items(t));
  }                             /* compute the size of the database */
  pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
  for (inv = rd->cand, i = m = 0; i < k; i++) {
    if (supp[i] <  rd->smin) {  /* traverse items and their support */
      supp[i] = -1; continue; } /* eliminate infrequent items an */
    if (supp[i] >= pex) {       /* collect perfect extension items */
      supp[i] = -1; isr_addpex(rd->report, lists[i]->item); continue; }
    inv[m++] = i;               /* collect the remaining items */
  }                             /* (map from new to old identifiers) */
  if (m <= 0) return 0;         /* if no frequent items found, abort */
  i = inv[m-1];                 /* get the highest frequent item */
  if (++i < k) k = i;           /* and compute limit for item loop */
  i2s_sort(inv, (size_t)m, -1, supp);
  map = inv+m;                  /* sort items by their support */
  if ((m <= 16) && rd->fim16) { /* if at most 16-items left */
    for (i = 0; i < k; i++) map[i] = -1;
    for (i = 0; i < m; i++) {   /* build a map from the old */
      map[inv[i]] = i;          /* to the new item identifiers */
      m16_setmap(rd->fim16, i, lists[inv[i]]->item);
    }                           /* set the item identifier map */
    for (n = 0; n < l->cnt; n++) {
      t = l->tracts[n]; b = 0;  /* traverse the transactions */
      for (s = ta_items(t); (UITEM)*s < (UITEM)k; s++)
        if ((i = map[*s]) >= 0) b |= 1 << i;
      m16_add(rd->fim16, (BITTA)b, ta_wgt(t));
    }                           /* add bit-represented transactions */
    return m16_mine(rd->fim16); /* to the 16-items machine and */
  }                             /* mine frequent item sets */
  for (i = 0; i < k; i++) {     /* copy support to trans. lists */
    if (supp[i] > 0) lists[i]->supp = supp[i];
    else           { lists[i]->supp = 0; map[i] = -1; }
  }                             /* set map for eliminated items */
  dst = lists;                  /* get the trans. lists to process */
  if ((l->cnt >= 6)             /* if there are enough transactions */
  &&  (m >= ((rd->fim16) ? 20 : 6))) {     /* and enough items left */
    dst = (TALIST**)malloc((size_t)m *sizeof(TALIST*));
    if (!dst) return -1;        /* allocate memory for projection */
    for (i = 0; i < m; i++) {   /* copy the transaction lists that */
      dst[i] = lists[inv[i]];   /* are needed for the projection */
      map[inv[i]] = i;          /* and build a map from the old */
    }                           /* to the new item identifiers */
    mem = malloc(taa_dstsize(l->cnt, x)); /* allocate memory for */
    if (!mem) { free(dst); return -1; }   /* destination trans. */
    l->cnt = taa_reduce(l->tracts, l->cnt, k, map, rd->hash, &mem);
    k = m;                      /* reduce the transactions */
  }                             /* (remove items, collate trans.) */
  if (rd->fim16                 /* if to use a 16-items machine */
  && (rd->dir > 0)) {           /* and forward processing direction */
    for (n = 0; n < l->cnt; n++) {
      t = l->tracts[n]; b = 0;  /* traverse the transactions */
      for (s = ta_items(t); (UITEM)*s < (UITEM)16; s++)
        b |= 1 << *s;           /* add trans. to 16-items machine */
      m16_add(rd->fim16, (BITTA)b, ta_wgt(t));
      for ( ; (UITEM)*s < (UITEM)k; s++) {
        p = dst[*s]; p->tracts[p->cnt++] = t; }
    }                           /* deliver the item occurrences */
    for (i = 0; i < 16; i++) {  /* traverse the first 16 items */
      l = dst[i]; l->supp = 0; l->cnt = 0; /* and clear support */
      m16_setmap(rd->fim16, i, l->item);
    }                           /* set the item identifier map */
    r = m16_mine(rd->fim16);    /* mine with 16-items machine */
    if (r < 0) return r; }      /* and check for an error */
  else {                        /* if not to use a 16-items machine */
    for (n = 0; n < l->cnt; n++) {
      t = l->tracts[n];         /* traverse the transactions */
      for (s = ta_items(t); (UITEM)*s < (UITEM)k; s++) {
        p = dst[*s]; p->tracts[p->cnt++] = t; }
    }                           /* deliver the item occurrences */
    i = 0;                      /* to the transaction lists and */
  }                             /* get first item index to process */
  m = isr_xable(rd->report, 2) ? 0 : ITEM_MAX;
  for (r = 0; i < k; i++) {     /* traverse the items/trans. lists, */
    l = dst[i];                 /* but skip all eliminated items */
    if (l->supp <= 0) { l->cnt = 0; continue; }
    r = isr_add(rd->report, l->item, l->supp);
    if (r < 0) break;           /* add current item to the reporter */
    if (r > 0) {                /* if the item needs processing */
      if (i > m) {              /* if to compute a projection */
        r = rec_odro(dst, i, rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* and check for a recursion error */
      r = isr_report(rd->report);
      if (r < 0) break;         /* report the current item set */
      isr_remove(rd->report,1); /* remove the current item */
    }                           /* from the item set reporter */
    l->supp = 0; l->cnt = 0;    /* reinitialize the transaction list */
  }
  if (mem) {                    /* delete projection lists */
    free(mem); free(dst); }     /* and destination memory */
  return r;                     /* return the error status */
}  /* rec_odro() */

/*--------------------------------------------------------------------*/

int eclat_ocd (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- eclat with occurrence deliver */
  int       r = 0;              /* result of recursion/error status */
  ITEM      i, k;               /* loop variable, number of items */
  TID       n, m;               /* number of transactions */
  size_t    x, h;               /* extent, hash table size */
  SUPP      pex;                /* minimum support for perfect exts. */
  TALIST    **lists, *l;        /* to traverse transaction lists */
  TRACT     **tras, **p;        /* to traverse transactions */
  const TID *c;                 /* item occurrence counters */
  RECDATA   rd;                 /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  c = tbg_icnts(tabag, 0);      /* get the number of containing */
  if (!c) return -1;            /* transactions per item */
  lists = (TALIST**)malloc((size_t)(k+1) *sizeof(TALIST*));
  if (!lists) return -1;        /* create the trans. id list array */
  for (x = 0, i = 0; i < k; i++)/* traverse the items and sum */
    x += (size_t)c[i];          /* the numbers of item occurrences */
  /* Do not use tbg_extent(), because it does not take packed items */
  /* properly into account and thus may yield too large a value.    */
  h = (size_t)taa_tabsize(n);   /* get the hash table size and */
  x = (size_t)n +x -(size_t)(k+1);        /* the database size */
  p = tras = (TRACT**)malloc((size_t)(k+1)   *sizeof(TALIST)
                           +         (x+h)   *sizeof(TRACT*)
                           + (size_t) k      *sizeof(SUPP)
                           + (size_t)(k+k+1) *sizeof(ITEM));
  if (!p) { free(lists); return -1; }
  for (i = 0; i < k; i++) {     /* allocate the list elements and */
    lists[i] = l = (TALIST*)p;  /* traverse the items / trans. lists */
    l->item  = i;               /* set the item identifier */
    l->supp  = 0;               /* clear the item support */
    l->cnt   = 0;               /* and the transaction counter */
    p = l->tracts +c[i];        /* skip space for transactions */
  }                             /* and a sentinel at the end */
  lists[k] = l = (TALIST*)p;    /* set last list (all transactions) */
  l->item  = k; l->cnt = n;     /* for a dummy item (> all items) */
  l->supp  = tbg_wgt(tabag);    /* with the full database support */
  for (m = 0; m < n; m++)       /* copy the transactions */
    l->tracts[m] = tbg_tract(tabag, m);
  rd.hash  = (TRACT**)memset(l->tracts+n, 0, h *sizeof(TRACT*));
  rd.muls  = (SUPP*)  memset(rd.hash  +h, 0, (size_t)k *sizeof(SUPP));
  rd.miss  = rd.muls;           /* get the auxiliary arrays */
  rd.cand  = (ITEM*)(rd.muls+k);/* and initialize them */
  rd.fim16 = NULL; rd.first = 0;/* default: no 16-items machine */
  if (mode & ECL_FIM16) {       /* if to use a 16-items machine */
    rd.fim16 = m16_create(rd.dir, rd.smin, report);
    if (!rd.fim16) { free(tras); free(lists); return -1; }
    rd.first = tbg_packcnt(tabag);
  }                             /* get the number of packed items */
  rd.report = report;           /* initialize the recursion data */
  rd.tabag  = tabag;            /* (store reporter and transactions) */
  r = (mode & ECL_EXTCHK)       /* exceute the eclat recursion with */
    ? (int)rec_odcm(lists, k, &rd)     /* explicit extension checks */
    : (mode & ECL_REORDER)      /* execute the eclat recursion */
    ? rec_odro(lists, k, &rd)   /* with    item reordering */
    : rec_odfx(lists, k, &rd);  /* without item reordering */
  if (r >= 0)                   /* if no error occurred, */
    r = isr_report(report);     /* report the empty item set */
  if (rd.fim16)                 /* if a 16-items machine was used, */
    m16_delete(rd.fim16);       /* delete the 16-items machine */
  free(tras); free(lists);      /* deallocate the transaction array */
  return r;                     /* return the error status */
}  /* eclat_ocd() */

/*----------------------------------------------------------------------
  Eclat with Diffsets
----------------------------------------------------------------------*/

static TID cmpl (TIDLIST *dst, TIDLIST *src1, TIDLIST *src2, SUPP *muls)
{                               /* --- complement two trans. id lists */
  TID *s1, *s2, *d;             /* to traverse sources and dest. */

  assert(dst && src1 && src2 && muls); /* check function arguments */
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = src1->supp;       /* initialize the support */
  d = dst->tids; s1 = src1->tids; s2 = src2->tids;
  while (1) {                   /* trans. id list difference loop */
    if      (*s1 > *s2) dst->supp -= muls[*s1++];
    else if (*s1 < *s2) *d++ = *s2++;
    else if (*s1 < 0) break;    /* collect elements of second source */
    else { s1++; s2++; }        /* that are not in the first source */
  }                             /* (form complement of first source) */
  *d++ = -1;                    /* store a sentinel at the list end */
  return (TID)(d -dst->tids);   /* return the size of the new lists */
}  /* cmpl() */

/*--------------------------------------------------------------------*/

static TID diff (TIDLIST *dst, TIDLIST *src1, TIDLIST *src2, SUPP *muls)
{                               /* --- subtract two trans. id lists */
  TID *s1, *s2, *d;             /* to traverse sources and dest. */

  assert(dst && src1 && src2 && muls); /* check function arguments */
  dst->item = src1->item;       /* copy the first item and */
  dst->supp = src1->supp;       /* initialize the support */
  d = dst->tids; s1 = src1->tids; s2 = src2->tids;
  while (1) {                   /* trans. id list difference loop */
    if      (*s1 > *s2) *d++ = *s1++;
    else if (*s1 < *s2) dst->supp -= muls[*s2++];
    else if (*s1 < 0) break;    /* remove all elements of the second */
    else { s1++; s2++; }        /* source from the first source */
  }                             /* (form difference of tid lists) */
  *d++ = -1;                    /* store a sentinel at the list end */
  return (TID)(d -dst->tids);   /* return the size of the new lists */
}  /* diff() */

/*--------------------------------------------------------------------*/

static int rec_diff (TIDLIST **lists, ITEM k, TID x,
                     COMBFN comb, RECDATA *rd)
{                               /* --- eclat recursion with diffsets */
  int     r;                    /* error status */
  ITEM    i, m, z;              /* loop variables */
  TID     c;                    /* size of combined lists */
  SUPP    pex;                  /* minimum support for perfect exts. */
  TIDLIST *l, *d;               /* to traverse transaction id lists */
  TIDLIST **proj = NULL;        /* trans. id lists of proj. database */
  ITEM    *t;                   /* to collect the tail items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & ECL_TAIL) {    /* if to use tail to prune w/ repo. */
    t = isr_buf(rd->report);    /* collect the tail items in buffer */
    for (m = 0, i = k; --i >= 0; ) t[m++] = lists[i]->item;
    r = isr_tail(rd->report, t, m);
    if (r) return r;            /* if tail need not be processed, */
  }                             /* abort the recursion */
  if ((k > 1)                   /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TIDLIST**)malloc((size_t)k           *sizeof(TIDLIST*)
                            +(size_t)k           *sizeof(TIDLIST)
                            +(size_t)k*(size_t)x *sizeof(TID));
    if (!proj) return -1;       /* allocate list and element arrays */
  }                             /* (memory for projected database) */
  if ((k > 4)                   /* if there are enough items left, */
  &&  (rd->mode & ECL_REORDER)) /* re-sort the items w.r.t. support */
    ptr_qsort(lists, (size_t)k, +1, tid_cmp, NULL);
  if (rd->dir > 0) { z =  k; k  = 0; }
  else             { z = -1; k -= 1; }
  for (r = 0; k != z; k += rd->dir) {
    l = lists[k];               /* traverse the items / tid lists */
    r = isr_add(rd->report, l->item, l->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if (proj && (k > 0)) {      /* if another item can be added */
      pex = (rd->mode & ECL_PERFECT) ? l->supp : SUPP_MAX;
      proj[m = 0] = d = (TIDLIST*)(proj +k+1); x = 0;
      for (i = 0; i < k; i++) { /* traverse the preceding lists */
        c = comb(d, lists[i], l, rd->muls);
        if (d->supp < rd->smin) /* combine transaction id lists */
          continue;             /* eliminate infrequent items */
        if (d->supp >= pex) {   /* collect perfect extensions */
          isr_addpex(rd->report, d->item); continue; }
        proj[++m] = d = (TIDLIST*)(d->tids +c);
        if (c > x) x = c;       /* collect the trans. id lists and */
      }                         /* determine their maximum length */
      if (m > 0) {              /* if the projection is not empty */
        r = rec_diff(proj, m, x, diff, rd);
        if (r < 0) break;       /* recursively find freq. item sets */
      }                         /* in the created projection */
    }
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) free(proj);         /* delete the list and element arrays */
  return r;                     /* return the error status */
}  /* rec_diff() */

/*--------------------------------------------------------------------*/

int eclat_diff (TABAG *tabag, int target, SUPP smin, int mode,
                ISREPORT *report)
{                               /* --- eclat with difference sets */
  int        r = 0;             /* result of recursion/error status */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        n, z;              /* (maximum) number of transactions */
  size_t     x;                 /* number of item instances */
  SUPP       w;                 /* weight/support buffer */
  SUPP       pex;               /* minimum support for perfect exts. */
  TRACT      *t;                /* to traverse transactions */
  TIDLIST    **lists, *l;       /* to traverse transaction id lists */
  TID        *tids, *p, **next; /* to traverse transaction ids */
  const ITEM *s;                /* to traverse transaction items */
  const TID  *c;                /* item occurrence counters */
  RECDATA    rd;                /* recursion data */

  assert(tabag && report);      /* check the function arguments */
  rd.target = target;           /* store target type, search mode */
  rd.mode   = mode;             /* and item processing direction */
  rd.dir    = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin   = (smin > 0) ? smin : 1;   /* check and adapt the support */
  pex       = tbg_wgt(tabag);   /* check the total transaction weight */
  if (rd.smin > pex) return 0;  /* and get support for perfect exts. */
  if (!(mode & ECL_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  c = tbg_icnts(tabag, 0);      /* get the number of containing */
  if (!c) return -1;            /* transactions per item */
  lists = (TIDLIST**)malloc((size_t)k *sizeof(TIDLIST*)
                           +(size_t)k *sizeof(TID*)
                           +(size_t)n *sizeof(SUPP));
  if (!lists) return -1;        /* create initial tid list array */
  next     = (TID**)(lists+k);  /* and split off next position array */
  rd.muls  = (SUPP*)(next +k);  /* and transaction multiplicity array */
  x = tbg_extent(tabag);        /* get the number of item occurrences */
  p = tids = (TID*)malloc((size_t)k *sizeof(TIDLIST) +x *sizeof(TID));
  if (!p) { free(lists); return -1; }  /* allocate tid list elements */
  for (i = 0; i < k; i++) {     /* traverse the items / tid lists */
    lists[i] = l = (TIDLIST*)p; /* get/create the next trans. id list */
    l->item  = i;               /* initialize the list item */
    l->supp  = 0;               /* and the support counter */
    next[i]  = p = l->tids;     /* note position of next trans. id */
    p += c[i]; *p++ = (TID)-1;  /* skip space for transaction ids */
  }                             /* and store a sentinel at the end */
  while (n > 0) {               /* traverse the transactions */
    t = tbg_tract(tabag, --n);  /* get the next transaction */
    rd.muls[n] = w = ta_wgt(t); /* and store its weight */
    for (s = ta_items(t); *s > TA_END; s++) {
      lists[*s]->supp += w;     /* traverse the transaction's items */
      *next[*s]++      = n;     /* sum the transaction weight and */
    }                           /* collect the transaction ids */
  }
  z = 0;                        /* init. the maximal list length */
  for (i = m = 0; i < k; i++) { /* traverse the items / tid lists */
    l = lists[i];               /* eliminate all infrequent items and */
    if (l->supp <  rd.smin) continue;   /* collect perfect extensions */
    if (l->supp >= pex) { isr_addpex(report, i); continue; }
    n = (TID)(next[i] -l->tids);
    if (n > z) z = n;           /* find maximum trans. id list length */
    lists[m++] = l;             /* collect lists for frequent items */
  }                             /* (eliminate infrequent items) */
  if (m > 0) {                  /* if there are frequent items */
    rd.report = report;         /* initialize the recursion data */
    rd.tabag  = tabag;          /* (store reporter and transactions) */
    r = rec_diff(lists, m, z, cmpl, &rd);
  }                             /* find freq. items sets recursively */
  if (r >= 0)                   /* if no error occurred, */
    r = isr_report(report);     /* report the empty item set */
  free(tids); free(lists);      /* delete the allocated arrays */
  return r;                     /* return the error status */
}  /* eclat_diff() */

/*----------------------------------------------------------------------
  Eclat with Occurrence Deliver (for rules)
----------------------------------------------------------------------*/

static int rec_tree (TALIST **lists, ITEM k, RECDATA *rd)
{                               /* --- occ. deliver w/o reordering */
  int        r = 0;             /* error status */
  ITEM       i, m;              /* loop variables */
  TID        n;                 /* loop variable for transactions */
  SUPP       w;                 /* weight/support buffer */
  TALIST     *l, *p;            /* to traverse transaction lists */
  TRACT      *t;                /* to traverse transactions */
  const ITEM *s;                /* to traverse items */

  assert(lists && (k > 0) && rd);  /* check the function arguments */
  #ifdef ECL_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  l = lists[k];                 /* collate equal transactions */
  taa_collate(l->tracts, l->cnt, k);
  for (n = 0; n < l->cnt; n++){ /* traverse the transactions, */
    t = l->tracts[n];           /* but skip collated transactions */
    if ((w = ta_wgt(t)) <= 0) continue;
    for (s = ta_items(t); (UITEM)*s < (UITEM)k; s++) {
      p = lists[*s]; p->supp += w; p->tracts[p->cnt++] = t; }
  }                             /* deliver the item occurrences */
  for (i = m = 0; i < k; i++) { /* traverse the items */
    p = lists[i];               /* get corresp. transaction list */
    if (p->supp < rd->smin) { p->supp = 0; p->cnt = 0; }
    else m += 1;                /* eliminate infrequent items */
  }                             /* and count the frequent items */
  if (m <= 0) {                 /* if no frequent items found, abort */
    taa_uncoll(l->tracts, l->cnt); return 0; }
  for (i = 0; i < k; i++) {     /* traverse the items and */
    p = lists[i];               /* get corresp. transaction list */
    if (p->supp >= rd->smin) ist_setsupp(rd->istree, i, p->supp);
  }                             /* set the item set support */
  m = ist_xable(rd->istree, 1) ? 0 : ITEM_MAX;
  if ((m <= 0) && (ist_addchn(rd->istree) != 0))
    return -1;                  /* add children to current node */
  for (i = 0; i < k; i++) {     /* traverse the items */
    p = lists[i];               /* get corresp. transaction list */
    if (p->supp <= 0) continue; /* skip all eliminated items */
    if ((i > m)                 /* go down in the item set tree */
    &&  (ist_down(rd->istree, i) >= 0)) {
      r = rec_tree(lists, i, rd);
      if (r < 0) break;         /* recursively find freq. item sets */
      ist_up(rd->istree);       /* and check for a recursion error, */
    }                           /* then go back up in the tree */
    p->supp = 0; p->cnt = 0;    /* reinit. the transaction list */
  }
  taa_uncoll(l->tracts,l->cnt); /* uncollate the transactions */
  return r;                     /* and return the error status */
}  /* rec_tree() */

/*--------------------------------------------------------------------*/

int eclat_tree (TABAG *tabag, SUPP smin, int mode, ISTREE *istree)
{                               /* --- search for frequent item sets */
  int       r = 0;              /* result of recursion/error status */
  ITEM      i, k;               /* loop variable, number of items */
  TID       n, m;               /* number of transactions */
  size_t    x, h;               /* extent, hash table size */
  TALIST    **lists, *l;        /* to traverse transaction lists */
  TRACT     **tras, **p;        /* to traverse transactions */
  const TID *c;                 /* item occurrence counters */
  RECDATA   rd;                 /* recursion data */

  assert(tabag && istree);      /* check the function arguments */
  rd.mode = mode;               /* store search mode and item dir. */
  rd.dir  = +1;                 /* (only upward item loops possible) */
  rd.smin = (smin > 0) ? smin : 1;    /* check and adapt the support */
  k = tbg_itemcnt(tabag);       /* get the number of (frequent) items */
  if (k <= 0) return 0;         /* if there are none, abort */
  c = tbg_icnts(tabag, 0);      /* get the number of containing */
  if (!c) return -1;            /* transactions per item */
  lists = (TALIST**)malloc((size_t)(k+1) *sizeof(TALIST*));
  if (!lists) return -1;        /* create the trans. id list array */
  for (x = 0, i = 0; i < k; i++)/* traverse the items and sum */
    x += (size_t)c[i];          /* the numbers of item occurrences */
  /* Do not use tbg_extent(), because it does not take packed items */
  /* properly into account and thus may yield too big a value.      */
  n = tbg_cnt(tabag);           /* get the number of transactions */
  h = (size_t)taa_tabsize(n);   /* get the hash table size and */
  x = (size_t)n +x -(size_t)(k+1);        /* the database size */
  p = tras = (TRACT**)malloc((size_t)(k+1) *sizeof(TALIST)
                           +         (x+h) *sizeof(TRACT*)
                           + (size_t) k    *sizeof(SUPP)
                           + (size_t)(k+k) *sizeof(ITEM));
  if (!p) { free(lists); return -1; }
  for (i = 0; i < k; i++) {     /* allocate the list elements and */
    lists[i] = l = (TALIST*)p;  /* traverse the items / trans. lists */
    l->item  = i;               /* set the item identifier */
    l->supp  = 0;               /* clear the item support */
    l->cnt   = 0;               /* and the transaction counter */
    p = l->tracts +c[i];        /* skip space for transactions */
  }                             /* and a sentinel at the end */
  lists[k] = l = (TALIST*)p;    /* set last list (all transactions) */
  l->item  = k; l->cnt = n;     /* for a dummy item (> all items) */
  l->supp  = tbg_wgt(tabag);    /* with the full database support */
  for (m = 0; m < n; m++)       /* copy the transactions */
    l->tracts[m] = tbg_tract(tabag, m);
  rd.hash   = (TRACT**)memset(l->tracts+n, 0, h *sizeof(TRACT*));
  rd.muls   = (SUPP*)  memset(rd.hash  +h, 0, (size_t)k *sizeof(SUPP));
  rd.cand   = (ITEM*)(rd.muls+k);    /* get the auxiliary arrays */
  rd.istree = istree;           /* initialize the recursion data */
  rd.tabag  = tabag;            /* (store tree and transactions) */
  r = rec_tree(lists, k, &rd);  /* execute the eclat recursion */
  free(tras); free(lists);      /* deallocate the transaction array */
  return r;                     /* return the error status */
}  /* eclat_tree() */

/*----------------------------------------------------------------------
  Eclat (generic)
----------------------------------------------------------------------*/

static ECLATFN* eclatvars[] = { /* --- table of eclat variants */
  eclat_base,                   /* trans. id lists (basic) */
  eclat_tid,                    /* trans. id lists (improved) */
  eclat_bit,                    /* bit vector over transactions */
  eclat_tab,                    /* item occurrence table */
  eclat_simp,                   /* simplified version with table */
  eclat_trg,                    /* transaction identifier ranges */
  eclat_ocd,                    /* occurrence deliver (LCM-style) */
  eclat_diff,                   /* difference sets (diffsets) */
};

/*--------------------------------------------------------------------*/

int eclat_data (TABAG *tabag, int target, SUPP smin, ITEM zmin,
                int eval, int algo, int mode, int sort)
{                               /* --- prepare data for Eclat */
  ITEM    m;                    /* number of items */
  ITEM    pack;                 /* number of items to pack */
  int     dir;                  /* order of items in transactions */
  #ifndef QUIET                 /* if to print messages */
  TID     n;                    /* number of transactions */
  SUPP    w;                    /* total transaction weight */
  clock_t t;                    /* timer for measurements */
  #endif                        /* (only needed for messages) */

  assert(tabag);                /* check the function arguments */

  /* --- make parameters consistent --- */
  if      (target & ISR_RULES)   target = ISR_RULES;
  else if (target & ISR_GENERAS) target = ISR_GENERAS;
  else if (target & ISR_MAXIMAL) target = ISR_MAXIMAL;
  else if (target & ISR_CLOSED)  target = ISR_CLOSED;
  else                           target = ISR_ALL;
  eval &= ~ECL_INVBXS;          /* get target and remove flags */
  if ((mode & ECL_TIDS) && (algo != ECL_LISTS) && (algo != ECL_TABLE))
    algo = ECL_LISTS;           /* trans. identifiers require lists */
  if ((target & (ISR_CLOSED|ISR_MAXIMAL))
  && (algo == ECL_OCCDLV)) {    /* special closed/maximal treatment */
    mode |= ECL_EXTCHK; mode &= ~(ECL_FIM16|ECL_REORDER); }
  if ((algo != ECL_LISTS) && (algo != ECL_OCCDLV))
    mode &= ~ECL_EXTCHK;        /* extension checks possible? */
  dir = ((algo == ECL_RANGES) || (algo == ECL_OCCDLV))
      ? +1 : -1;                /* get order of items in trans */
  if (((algo != ECL_LISTS)&&(algo != ECL_RANGES)&&(algo != ECL_OCCDLV))
  ||  (mode  &  ECL_EXTCHK))    /* restrict use of packed items */
    mode &= ~ECL_FIM16;         /* by algorithm and ext. checks */
  if ((target & ISR_RULES) || ((eval > RE_NONE) && (eval < RE_FNCNT))) {
    mode &= ~ECL_FIM16; dir = +1; }
  pack = mode & ECL_FIM16;      /* get number of items to pack */
  if (pack > 16) pack = 16;     /* pack at most 16 items */
  if ((algo == ECL_OCCDLV) && (mode & ECL_REORDER))
    pack = 0;                   /* delayed packing if reordering */
  if (mode & ECL_REORDER)       /* simplified sorting if reordering */
    sort = (sort < 0) ? -1 : (sort > 0) ? +1 : 0;

  /* --- sort and recode items --- */
  CLOCK(t);                     /* start timer, print log message */
  XMSG(stderr, "filtering, sorting and recoding items ... ");
  m = tbg_recode(tabag, smin, -1, -1, -sort);
  if (m <  0) return E_NOMEM;   /* recode items and transactions */
  if (m <= 0) return E_NOITEMS; /* and check the number of items */
  XMSG(stderr, "[%"ITEM_FMT" item(s)]", m);
  XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- sort and reduce transactions --- */
  CLOCK(t);                     /* start timer, print log message */
  XMSG(stderr, "sorting and reducing transactions ... ");
  if (!(target & ISR_RULES)     /* filter transactions if possible */
  &&  ((eval <= RE_NONE) || (eval >= RE_FNCNT)))
    tbg_filter(tabag, zmin, NULL, 0);
  tbg_itsort(tabag, dir, 0);    /* sort items in transactions */
  if (mode & ECL_EXTCHK) {      /* if checking exts. for clo./max. */
    tbg_sortsz(tabag, -1, 0);   /* sort transactions by size and */
    tbg_reduce(tabag, 0);       /* reduce transactions to unique ones */
    tbg_bitmark(tabag); }       /* set item bits in markers */
  else if ((algo == ECL_RANGES) /* if to use transaction ranges */
  &&       (pack > 0)) {        /* together with a 16-items machine */
    tbg_pack(tabag, pack);      /* pack the most frequent items */
    tbg_sort(tabag,1,TA_EQPACK);/* sort trans. lexicographically and */
    tbg_reduce(tabag, 0); }     /* reduce transactions to unique ones */
  else if (!(mode & ECL_TIDS)   /* if not to report transaction ids */
  &&       (algo != ECL_BITS)){ /* and not to use bit vectors */
    tbg_sort(tabag, dir, 0);    /* sort trans. lexicographically and */
    tbg_reduce(tabag, 0);       /* reduce transactions to unique ones */
    if (pack > 0)               /* if to use a 16-items machine, */
      tbg_pack(tabag, pack);    /* pack the most frequent items */
  }                             /* (bit-represented transactions) */
  #ifndef QUIET                 /* if to print messages */
  n = tbg_cnt(tabag);           /* get the number of transactions */
  w = tbg_wgt(tabag);           /* and the new transaction weight */
  XMSG(stderr, "[%"TID_FMT, n); /* print number of transactions */
  if (w != (SUPP)n) { XMSG(stderr, "/%"SUPP_FMT, w); }
  XMSG(stderr, " transaction(s)] done [%.2fs].\n", SEC_SINCE(t));
  #endif
  return 0;                     /* return 'ok' */
}  /* eclat_data() */

/*--------------------------------------------------------------------*/

int eclat_repo (ISREPORT *report, int target,
                int eval, double thresh, int algo, int mode)
{                               /* --- prepare reporter for Eclat */
  int mrep;                     /* mode for item set reporter */

  assert(report);               /* check the function arguments */

  /* --- make parameters consistent --- */
  if      (target & ISR_RULES)   target = ISR_RULES;
  else if (target & ISR_GENERAS) target = ISR_GENERAS;
  else if (target & ISR_MAXIMAL) target = ISR_MAXIMAL;
  else if (target & ISR_CLOSED)  target = ISR_CLOSED;
  else                           target = ISR_ALL;
  if ((mode & ECL_TIDS) && (algo != ECL_LISTS) && (algo != ECL_TABLE))
    algo = ECL_LISTS;           /* trans. identifiers require lists */
  if (target & (ISR_CLOSED|ISR_MAXIMAL)) {
    mode &= ~ECL_REORDER;       /* cannot reorder for closed/maximal */
    if (algo == ECL_OCCDLV) mode |= ECL_EXTCHK; }
  else if (target & ISR_GENERAS){/* cannot use simple table for gen. */
    if (algo == ECL_SIMPLE) algo = ECL_TABLE; }
  if ((algo == ECL_RANGES) || (algo == ECL_SIMPLE))
    mode &= ~ECL_REORDER;       /* not all variants allow reordering */
  mrep = 0;                     /* init. the reporting mode */
  if ((target & ISR_GENERAS) && (mode & ECL_REORDER))
    mrep |= ISR_SORT;           /* reordering requires set sorting */
  if ((algo != ECL_LISTS) && (algo != ECL_OCCDLV))
    mode &= ~ECL_EXTCHK;        /* extension checks possible? */
  eval &= ~ECL_INVBXS;          /* remove flags from measure code */
  if ((mode & ECL_EXTCHK) || (target & ISR_RULES)
  || ((eval > RE_NONE) && (eval < RE_FNCNT)))
    mrep |= ISR_NOFILTER;       /* no filtering if done in Eclat */

  /* --- configure item set reporter --- */
  if (eval == ECL_LDRATIO)      /* set additional evaluation measure */
    isr_seteval(report, isr_logrto, NULL, +1, thresh);
  return (isr_settarg(report, target, mrep, -1)) ? E_NOMEM : 0;
}  /* eclat_repo() */           /* set up the item set reporter */

/*--------------------------------------------------------------------*/

static int cleanup (ECLAT *data)
{                               /* --- clean up on error */
  if (data->mode & ECL_NOCLEAN) return E_NOMEM;
  if (data->istree) ist_delete(data->istree);
  return E_NOMEM;               /* free all allocated memory */
}  /* cleanup() */              /* return an error indicator */

/*--------------------------------------------------------------------*/

int eclat (TABAG *tabag, int target, SUPP smin, SUPP body, double conf,
           int eval, int agg, double thresh, ITEM prune,
           int algo, int mode, int order, ISREPORT *report)
{                               /* --- eclat algorithm */
  int      r;                   /* result of function call */
  ITEM     m;                   /* number of items */
  int      e;                   /* evaluation without flags */
  ITEM     zmin, zmax;          /* number of items in set/rule */
  ITEMBASE *base;               /* underlying item base */
  ECLAT    a = { 0, NULL };     /* eclat execution data */
  #ifndef QUIET                 /* if to print messages */
  clock_t  t;                   /* timer for measurements */
  #endif                        /* (only needed for messages) */

  assert(tabag && report);      /* check the function arguments */
  a.mode = mode;                /* note the processing mode */

  /* --- make parameters consistent --- */
  if      (target & ISR_RULES)   target = ISR_RULES;
  else if (target & ISR_GENERAS) target = ISR_GENERAS;
  else if (target & ISR_MAXIMAL) target = ISR_MAXIMAL;
  else if (target & ISR_CLOSED)  target = ISR_CLOSED;
  else                           target = ISR_ALL;
  if ((mode & ECL_TIDS) && (algo != ECL_LISTS) && (algo != ECL_TABLE))
    algo = ECL_LISTS;           /* trans. identifiers require lists */
  if (target & (ISR_CLOSED|ISR_MAXIMAL)) {
    mode &= ~ECL_REORDER;       /* cannot reorder for closed/maximal */
    if (algo == ECL_OCCDLV) mode |= ECL_EXTCHK; }
  else if (target & ISR_GENERAS){ /* if to filter for generators, */
    mode |= ECL_PERFECT;        /* need perfect extension pruning */
    if (algo == ECL_SIMPLE) algo = ECL_TABLE;
  }                             /* cannot use simple table variant */
  if ((algo != ECL_LISTS) && (algo != ECL_OCCDLV))
    mode &= ~ECL_EXTCHK;        /* extension checks possible? */
  if ((algo != ECL_LISTS) && (algo != ECL_RANGES)
  &&  (algo != ECL_OCCDLV))     /* not all algorithm variants */
    mode &= ~ECL_FIM16;         /* support a 16-items machine */
  if (mode & ECL_TIDS)          /* if to write transaction ids, */
    mode &= ~ECL_FIM16;         /* k-items machine cannot be used */
  e = eval & ~ECL_INVBXS;       /* remove flags from measure code */
  if (((target & ISR_RULES) || ((e > RE_NONE) && (e < RE_FNCNT)))
  ||  (mode  & ECL_EXTCHK))     /* restrict use of packed items */
    mode &= ~ECL_FIM16;         /* by algorithm and ext. checks */
  if ((algo == ECL_RANGES) || (algo == ECL_SIMPLE))
    mode &= ~ECL_REORDER;       /* not all variants allow reordering */
  if (!(target & ISR_MAXIMAL) || (mode & ECL_FIM16))
    mode &= ~ECL_TAIL;          /* tail pruning only for maximal */
  if (e <= RE_NONE)             /* if there is no evaluation, */
    prune = ITEM_MIN;           /* do not prune with evaluation */

  /* --- find frequent item sets --- */
  base = tbg_base(tabag);       /* get the underlying item base */
  if (!(target & ISR_RULES)     /* if to find plain item sets */
  &&  ((e <= RE_NONE) || (e >= RE_FNCNT))) {
    CLOCK(t);                   /* start timer, print log message */
    XMSG(stderr, "writing %s ... ", isr_name(report));
    r = eclatvars[algo](tabag, target, smin, mode, report);
    if (r < 0) return E_NOMEM;  /* search for frequent item sets */
    XMSG(stderr, "[%"SIZE_FMT" set(s)]", isr_repcnt(report));
    XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t)); }
  else {                        /* if rules or rule-based evaluation */
    CLOCK(t);                   /* start timer, print log message */
    XMSG(stderr, "finding frequent item sets ... ");
    a.istree = ist_create(base, IST_REVERSE, smin, body, conf);
    if (!a.istree) return E_NOMEM;
    zmin = isr_zmin(report);    /* create an item set tree */
    zmax = isr_zmax(report);    /* and configure it */
    if ((target & (ISR_CLOSED|ISR_MAXIMAL)) && (zmax < ITEM_MAX))
      zmax += 1;                /* adapt the maximum size */
    if (zmax > (m = tbg_max(tabag))) zmax = m;
    ist_setsize(a.istree, zmin, zmax);
    r = eclat_tree(tabag, smin, mode, a.istree);
    if (r) return cleanup(&a);  /* search for frequent item sets */
    XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
    if ((prune >  ITEM_MIN)     /* if to filter with evaluation */
    &&  (prune <= 0)) {         /* (backward and weak forward) */
      CLOCK(t);                 /* start the timer for filtering */
      XMSG(stderr, "filtering with evaluation ... ");
      ist_filter(a.istree, prune); /* mark non-qualifying item sets */
      XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
    }                           /* filter with evaluation */
    if (target & (ISR_CLOSED|ISR_MAXIMAL|ISR_GENERAS)) {
      CLOCK(t);                 /* start the timer for filtering */
      XMSG(stderr, "filtering for %s item sets ... ",
           (target & ISR_GENERAS) ? "generator" :
           (target & ISR_MAXIMAL) ? "maximal" : "closed");
      ist_clomax(a.istree, target | ((prune > ITEM_MIN) ? IST_SAFE :0));
      XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
    }                           /* filter closed/maximal/generators */
    CLOCK(t);                   /* start timer, print log message */
    XMSG(stderr, "writing %s ... ", isr_name(report));
    if (e != ECL_LDRATIO)       /* set additional evaluation measure */
      ist_seteval(a.istree, eval, agg, thresh, prune);
    ist_init(a.istree, order);  /* initialize the extraction */
    r = ist_report(a.istree, report, target);
    cleanup(&a);                /* report item sets/rules, */
    if (r < 0) return E_NOMEM;  /* then clean up the work memory */
    XMSG(stderr, "[%"SIZE_FMT" %s(s)]", isr_repcnt(report),
                 (target == ISR_RULES) ? "rule" : "set");
    XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* print a log message */
  return 0;                     /* return 'ok' */
}  /* eclat() */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/
#ifdef ECL_MAIN

static void help (void)
{                               /* --- print add. option information */
  #ifndef QUIET
  fprintf(stderr, "\n");        /* terminate startup message */
  printf("additional evaluation measures (option -e#)\n");
  printf("frequent item sets:\n");
  printf("  x   no measure (default)\n");
  printf("  b   binary logarithm of support quotient            (+)\n");
  printf("association rules:\n");
  printf("  x   no measure (default)\n");
  printf("  o   rule support (original def.: body & head)       (+)\n");
  printf("  c   rule confidence                                 (+)\n");
  printf("  d   absolute confidence difference to prior         (+)\n");
  printf("  l   lift value (confidence divided by prior)        (+)\n");
  printf("  a   absolute difference of lift value to 1          (+)\n");
  printf("  q   difference of lift quotient to 1                (+)\n");
  printf("  v   conviction (inverse lift for negated head)      (+)\n");
  printf("  e   absolute difference of conviction to 1          (+)\n");
  printf("  r   difference of conviction quotient to 1          (+)\n");
  printf("  k   conditional probability ratio                   (+)\n");
  printf("  j   importance (binary log. of cond. prob. ratio)   (+)\n");
  printf("  z   certainty factor (relative confidence change)   (+)\n");
  printf("  n   normalized chi^2 measure                        (+)\n");
  printf("  p   p-value from (unnormalized) chi^2 measure       (-)\n");
  printf("  y   normalized chi^2 measure with Yates' correction (+)\n");
  printf("  t   p-value from Yates-corrected chi^2 measure      (-)\n");
  printf("  i   information difference to prior                 (+)\n");
  printf("  g   p-value from G statistic/information difference (-)\n");
  printf("  f   Fisher's exact test (table probability)         (-)\n");
  printf("  h   Fisher's exact test (chi^2 measure)             (-)\n");
  printf("  m   Fisher's exact test (information gain)          (-)\n");
  printf("  s   Fisher's exact test (support)                   (-)\n");
  printf("All measures for association rules are also applicable\n");
  printf("to item sets and are then aggregated over all possible\n");
  printf("association rules with a single item in the consequent.\n");
  printf("The aggregation mode can be set with the option -a#.\n");
  printf("Measures marked with (+) must meet or exceed the threshold,\n");
  printf("measures marked with (-) must not exceed the threshold\n");
  printf("in order for the rule or item set to be reported.\n");
  printf("\n");
  printf("evaluation measure aggregation modes (option -a#)\n");
  printf("  x   no aggregation (use first value)\n");
  printf("  m   minimum of individual measure values\n");
  printf("  n   maximum of individual measure values\n");
  printf("  a   average of individual measure values\n");
  printf("\n");
  printf("eclat algorithm variants (option -A#)\n");
  printf("  a   automatic choice based on data properties (default)\n");
  printf("  e   transaction id lists intersection (basic)\n");
  printf("  i   transaction id lists intersection (improved)\n");
  printf("  b   transaction id lists represented as bit vectors\n");
  printf("  t   filtering with item occurrence table (standard)\n");
  printf("  s   filtering with item occurrence table (simplified)\n");
  printf("  r   transaction id range lists intersection\n");
  printf("  o   occurrence deliver from transaction lists (default)\n");
  printf("  d   transaction id difference sets (diffsets/dEclat)\n");
  printf("With algorithm variant 'o' closed or maximal item sets can\n");
  printf("only be found with extensions checks. Hence option -y0 is\n");
  printf("automatically added to options -tc or -tm if -Ao is given.\n");
  printf("Rules as the target type or using a rule-based evaluation\n");
  printf("measure enforce algorithm variant 'o'.\n");
  printf("\n");
  printf("information output format characters (option -v#)\n");
  printf("  %%%%  a percent sign\n");
  printf("  %%i  number of items (item set size)\n");
  printf("  %%a  absolute item set  support\n");
  printf("  %%s  relative item set  support as a fraction\n");
  printf("  %%S  relative item set  support as a percentage\n");
  printf("  %%b  absolute body set  support\n");
  printf("  %%x  relative body set  support as a fraction\n");
  printf("  %%X  relative body set  support as a percentage\n");
  printf("  %%h  absolute head item support\n");
  printf("  %%y  relative head item support as a fraction\n");
  printf("  %%Y  relative head item support as a percentage\n");
  printf("  %%c  rule confidence as a fraction\n");
  printf("  %%C  rule confidence as a percentage\n");
  printf("  %%l  lift value of a rule (confidence/prior)\n");
  printf("  %%L  lift value of a rule as a percentage\n");
  printf("  %%e  additional evaluation measure\n");
  printf("  %%E  additional evaluation measure as a percentage\n");
  printf("All format characters can be preceded by the number\n");
  printf("of significant digits to be printed (at most 32 digits),\n");
  printf("even though this value is ignored for integer numbers.\n");
  #endif                        /* print help information */
  exit(0);                      /* abort the program */
}  /* help() */

/*--------------------------------------------------------------------*/

static ITEM getbdr (char *s, char **end, double **border)
{                               /* --- get the support border */
  ITEM   i, k;                  /* loop variables */
  double *b;                    /* support border */

  assert(s && end && border);   /* check the function arguments */
  for (i = k = 0; s[i]; i++)    /* traverse the string and */
    if (s[i] == ':') k++;       /* count the number separators */
  *border = b = (double*)malloc((size_t)++k *sizeof(double));
  if (!b) return -1;            /* allocate a support border */
  for (i = 0; i < k; i++) {     /* traverse the parameters */
    b[i] = strtod(s, end);      /* get the next parameter and */
    if (*end == s) break;       /* check for an empty parameter */
    s = *end; if (*s++ != ':') break;
  }                             /* check for a colon */
  if (++i < k)                  /* shrink support array if possible */
    *border = (double*)realloc(b, (size_t)i *sizeof(double));
  return i;                     /* return number of support values */
}  /* getbdr() */

/*--------------------------------------------------------------------*/

static int setbdr (ISREPORT *report, SUPP w, ITEM zmin,
                   double **border, ITEM n)
{                               /* --- set the support border */
  double s;                     /* to traverse the support values */

  assert(report                 /* check the function arguments */
  &&    (w > 0) && (zmin >= 0) && border && (*border || (n <= 0)));
  while (--n >= 0) {            /* traverse the support values */
    s = (*border)[n];           /* transform to absolute count */
    s = ceilsupp((s >= 0) ? 0.01 *s *(double)w *(1-DBL_EPSILON) : -s);
    if (isr_setbdr(report, n+zmin, (RSUPP)s) < 0) return -1;
  }                             /* set support in item set reporter */
  if (*border) { free(*border); *border = NULL; }
  return 0;                     /* return 'ok' */
}  /* setbdr() */

/*--------------------------------------------------------------------*/

#ifndef NDEBUG                  /* if debug version */
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (twrite) twr_delete(twrite, 1); \
  if (report) isr_delete(report, 0); \
  if (tabag)  tbg_delete(tabag,  0); \
  if (tread)  trd_delete(tread,  1); \
  if (ibase)  ib_delete (ibase);     \
  if (border) free(border);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, counters */
  char    *s;                   /* to traverse the options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_inp  = NULL;      /* name of the input  file */
  CCHAR   *fn_out  = NULL;      /* name of the output file */
  CCHAR   *fn_sel  = NULL;      /* name of item selection file */
  CCHAR   *fn_tid  = NULL;      /* name of transaction ids file */
  CCHAR   *fn_psp  = NULL;      /* name of pattern spectrum file */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *comment = NULL;      /* comment characters */
  CCHAR   *hdr     = "";        /* record header  for output */
  CCHAR   *sep     = " ";       /* item separator for output */
  CCHAR   *imp     = " <- ";    /* implication sign for ass. rules */
  CCHAR   *dflt    = " (%S)";   /* default format for check */
  CCHAR   *info    = dflt;      /* format for information output */
  int     target   = 's';       /* target type (e.g. closed/maximal) */
  ITEM    zmin     = 1;         /* minimum rule/item set size */
  ITEM    zmax     = ITEM_MAX;  /* maximum rule/item set size */
  double  supp     = 10;        /* minimum support of an item set */
  SUPP    smin     = 1;         /* minimum support of an item set */
  double  smax     = 100;       /* maximum support of an item set */
  SUPP    body     = 10;        /* minimum support of a rule body */
  int     orig     = 0;         /* flag for rule support definition */
  double  conf     = 80;        /* minimum confidence (in percent) */
  int     eval     = 'x';       /* additional evaluation measure */
  int     eflgs    = 0;         /* evaluation measure flags */
  int     agg      = 'x';       /* aggregation mode for eval. measure */
  double  thresh   = 10;        /* threshold for evaluation measure */
  ITEM    prune    = ITEM_MIN;  /* (min. size for) evaluation pruning */
  int     sort     = 2;         /* flag for item sorting and recoding */
  int     algo     = 'a';       /* variant of eclat algorithm */
  int     mode     = ECL_DEFAULT;  /* search mode (e.g. pruning) */
  int     pack     = 16;        /* number of bit-packed items */
  int     cmfilt   = -1;        /* mode for closed/maximal filtering */
  int     mtar     = 0;         /* mode for transaction reading */
  int     scan     = 0;         /* flag for scanable item output */
  int     bdrcnt   = 0;         /* number of support values in border */
  int     stats    = 0;         /* flag for item set statistics */
  int     pfids    = 1;         /* flag for preformatted trans. ids */
  PATSPEC *psp;                 /* collected pattern spectrum */
  ITEM    m;                    /* number of items */
  TID     n;                    /* number of transactions */
  SUPP    w;                    /* total transaction weight */
  #ifndef QUIET                 /* if not quiet version */
  clock_t t;                    /* timer for measurements */

  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no argument is given */
    printf("usage: %s [options] infile [outfile]\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("-t#      target type                              "
                    "(default: %c)\n", target);
    printf("         (s: frequent, c: closed, m: maximal item sets,\n");
    printf("          g: generators, r: association rules)\n");
    printf("-m#      minimum number of items per set/rule     "
                    "(default: %"ITEM_FMT")\n", zmin);
    printf("-n#      maximum number of items per set/rule     "
                    "(default: no limit)\n");
    printf("-s#      minimum support of an item set/rule      "
                    "(default: %g%%)\n", supp);
    printf("-S#      maximum support of an item set/rule      "
                    "(default: %g%%)\n", smax);
    printf("         (positive: percentage, "
                     "negative: absolute number)\n");
    printf("-o       use original rule support definition     "
                    "(body & head)\n");
    printf("-c#      minimum confidence of an assoc. rule     "
                    "(default: %g%%)\n", conf);
    printf("-e#      additional evaluation measure            "
                    "(default: none)\n");
    printf("-a#      aggregation mode for evaluation measure  "
                    "(default: none)\n");
    printf("-d#      threshold for add. evaluation measure    "
                    "(default: %g%%)\n", thresh);
    printf("-z       invalidate eval. below expected support  "
                    "(default: evaluate all)\n");
    printf("-p#      (min. size for) pruning with evaluation  "
                    "(default: no pruning)\n");
    printf("         (< 0: weak forward, > 0 strong forward, "
                     "= 0: backward pruning)\n");
    printf("-q#      sort items w.r.t. their frequency        "
                    "(default: %d)\n", sort);
    printf("         (1: ascending, -1: descending, 0: do not sort,\n"
           "          2: ascending, -2: descending w.r.t. "
                    "transaction size sum)\n");
    printf("-A#      variant of the eclat algorithm to use    "
                    "(default: 'a')\n");
    printf("-x       do not prune with perfect extensions     "
                    "(default: prune)\n");
    printf("-l#      number of items for k-items machine      "
                    "(default: %d)\n", pack);
    printf("         (only for algorithm variants i,r,o,   "
                    "options -Ai/-Ar/-Ao)\n");
    printf("-i       do not sort items w.r.t. cond. support   "
                    "(default: sort)\n");
    printf("         (only for algorithm variants i,b,t,d, "
                    "options -Ai/-Ab/-At/-Ad)\n");
    printf("-y#      check extensions for closed/maximal sets "
                    "(default: repository)\n");
    printf("         (0: horizontal, > 0: vertical representation)\n");
    printf("         (only with improved tid lists variant, "
                    "option -Ai)\n");
    printf("-u       do not use head union tail (hut) pruning "
                    "(default: use hut)\n");
    printf("         (only for maximal item sets, option -tm, "
                    "not with option -Ab)\n");
    printf("-F#:#..  support border for filtering item sets   "
                    "(default: none)\n");
    printf("         (list of minimum support values, "
                    "one per item set size,\n");
    printf("         starting at the minimum size, "
                    "as given with option -m#)\n");
    printf("-R#      read item selection/appearance indicators\n");
    printf("-P#      write a pattern spectrum to a file\n");
    printf("-Z       print item set statistics "
                    "(number of item sets per size)\n");
    printf("-g       write output in scanable form "
                    "(quote certain characters)\n");
    printf("-h#      record header  for output                "
                    "(default: \"%s\")\n", hdr);
    printf("-k#      item separator for output                "
                    "(default: \"%s\")\n", sep);
    printf("-I#      implication sign for association rules   "
                    "(default: \"%s\")\n", imp);
    printf("-v#      output format for item set information   "
                    "(default: \"%s\")\n", info);
    printf("-w       transaction weight in last field         "
                    "(default: only items)\n");
    printf("-r#      record/transaction separators            "
                    "(default: \"\\n\")\n");
    printf("-f#      field /item        separators            "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                       "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                       "
                    "(default: \"#\")\n");
    printf("-T#      file to write transaction identifiers to "
                    "(default: none)\n");
    printf("-E       do not preformat transactions ids        "
                    "(default: do)\n");
    printf("-!       print additional option information\n");
    printf("infile   file to read transactions from           "
                    "[required]\n");
    printf("outfile  file to write item sets/assoc.rules to   "
                    "[optional]\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */
  #endif  /* #ifndef QUIET */
  /* free option characters: j [A-Z]\[ACFIPRSTZ] */

  /* --- evaluate arguments --- */
  for (i = 1; i < argc; i++) {  /* traverse the arguments */
    s = argv[i];                /* get an option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (*s) {              /* traverse the options */
        switch (*s++) {         /* evaluate the options */
          case '!': help();                          break;
          case 't': target = (*s) ? *s++ : 's';      break;
          case 'm': zmin   = (ITEM)strtol(s, &s, 0); break;
          case 'n': zmax   = (ITEM)strtol(s, &s, 0); break;
          case 's': supp   =       strtod(s, &s);    break;
          case 'S': smax   =       strtod(s, &s);    break;
          case 'o': orig   = 1;                      break;
          case 'c': conf   =       strtod(s, &s);    break;
          case 'e': eval   = (*s) ? *s++ : 0;        break;
          case 'a': agg    = (*s) ? *s++ : 0;        break;
          case 'd': thresh =       strtod(s, &s);    break;
          case 'z': eflgs |= ECL_INVBXS;             break;
          case 'p': prune  = (ITEM)strtol(s, &s, 0); break;
          case 'q': sort   = (int) strtol(s, &s, 0); break;
          case 'A': algo   = (*s) ? *s++ : 0;        break;
          case 'x': mode  &= ~ECL_PERFECT;           break;
          case 'l': pack   = (int) strtol(s, &s, 0); break;
          case 'i': mode  &= ~ECL_REORDER;           break;
          case 'y': cmfilt = (int) strtol(s, &s, 0); break;
          case 'u': mode  &= ~ECL_TAIL;              break;
          case 'F': bdrcnt = getbdr(s, &s, &border); break;
          case 'R': optarg = &fn_sel;                break;
          case 'P': optarg = &fn_psp;                break;
          case 'Z': stats  = 1;                      break;
          case 'g': scan   = 1;                      break;
          case 'h': optarg = &hdr;                   break;
          case 'k': optarg = &sep;                   break;
          case 'I': optarg = &imp;                   break;
          case 'v': optarg = &info;                  break;
          case 'w': mtar  |= TA_WEIGHT;              break;
          case 'r': optarg = &recseps;               break;
          case 'f': optarg = &fldseps;               break;
          case 'b': optarg = &blanks;                break;
          case 'C': optarg = &comment;               break;
          case 'T': optarg = &fn_tid;                break;
          case 'E': pfids  = 0;                      break;
          default : error(E_OPTION, *--s);           break;
        }                       /* set the option variables */
        if (optarg && *s) { *optarg = s; optarg = NULL; break; }
      } }                       /* get an option argument */
    else {                      /* -- if argument is no option */
      switch (k++) {            /* evaluate non-options */
        case  0: fn_inp = s;      break;
        case  1: fn_out = s;      break;
        default: error(E_ARGCNT); break;
      }                         /* note filenames */
    }
  }
  if (optarg)       error(E_OPTARG);     /* check option arguments */
  if (k      < 1)   error(E_ARGCNT);     /* and number of arguments */
  if (zmin   < 0)   error(E_SIZE, zmin); /* check the size limits */
  if (zmax   < 0)   error(E_SIZE, zmax); /* and the minimum support */
  if (supp   > 100) error(E_SUPPORT, supp);
  if (bdrcnt < 0)   error(E_NOMEM);
  if ((conf  < 0) || (conf > 100))
    error(E_CONF, conf);        /* check the minimum confidence */
  if ((!fn_inp || !*fn_inp) && (fn_sel && !*fn_sel))
    error(E_STDIN);             /* stdin must not be used twice */
  switch (target) {             /* check and translate target type */
    case 's': target = ISR_ALL;              break;
    case 'c': target = ISR_CLOSED;           break;
    case 'm': target = ISR_MAXIMAL;          break;
    case 'g': target = ISR_GENERAS;          break;
    case 'r': target = ISR_RULES;            break;
    default : error(E_TARGET, (char)target); break;
  }                             /* (get target type code) */
  switch (eval) {               /* check and translate measure */
    case 'x': eval = RE_NONE;                break;
    case 'o': eval = RE_SUPP;                break;
    case 'c': eval = RE_CONF;                break;
    case 'd': eval = RE_CONFDIFF;            break;
    case 'l': eval = RE_LIFT;                break;
    case 'a': eval = RE_LIFTDIFF;            break;
    case 'q': eval = RE_LIFTQUOT;            break;
    case 'v': eval = RE_CVCT;                break;
    case 'e': eval = RE_CVCTDIFF;            break;
    case 'r': eval = RE_CVCTQUOT;            break;
    case 'k': eval = RE_CPROB;               break;
    case 'j': eval = RE_IMPORT;              break;
    case 'z': eval = RE_CERT;                break;
    case 'n': eval = RE_CHI2;                break;
    case 'p': eval = RE_CHI2PVAL;            break;
    case 'y': eval = RE_YATES;               break;
    case 't': eval = RE_YATESPVAL;           break;
    case 'i': eval = RE_INFO;                break;
    case 'g': eval = RE_INFOPVAL;            break;
    case 'f': eval = RE_FETPROB;             break;
    case 'h': eval = RE_FETCHI2;             break;
    case 'm': eval = RE_FETINFO;             break;
    case 's': eval = RE_FETSUPP;             break;
    case 'b': eval = ECL_LDRATIO;            break;
    default : error(E_MEASURE, (char)eval);  break;
  }  /* free: u w */            /* (get evaluation measure code) */
  eval |= eflgs;                /* add evaluation measure flags */
  switch (agg) {                /* check and translate agg. mode */
    case 'x': agg  = ECL_NONE;               break;
    case 'm': agg  = ECL_MIN;                break;
    case 'n': agg  = ECL_MAX;                break;
    case 'a': agg  = ECL_AVG;                break;
    default : error(E_AGGMODE, (char)agg);   break;
  }                             /* (get aggregation mode code) */
  switch (algo) {               /* check and translate alg. variant */
    case 'a': algo = ECL_AUTO;               break;
    case 'e': algo = ECL_BASIC;              break;
    case 'i': algo = ECL_LISTS;              break;
    case 't': algo = ECL_TABLE;              break;
    case 'r': algo = ECL_RANGES;             break;
    case 'b': algo = ECL_BITS;               break;
    case 's': algo = ECL_SIMPLE;             break;
    case 'o': algo = ECL_OCCDLV;             break;
    case 'd': algo = ECL_DIFFS;              break;
    default : error(E_VARIANT, (char)algo);  break;
  }                             /* (get eclat algorithm code) */
  if ((cmfilt >= 0) && (target & (ISR_CLOSED|ISR_MAXIMAL)))
    mode |= (cmfilt > 0) ? ECL_VERT : ECL_HORZ;
  if (fn_tid) {                 /* if to write transaction ids. */
    if (strcmp(fn_tid, "-") == 0) fn_tid = "";
    mode |= ECL_TIDS;           /* turn "-" into "" for consistency */
  }                             /* set transaction identifier flag */
  if (pack > 0)                 /* add packed items to search mode */
    mode |= (pack < 16) ? pack : 16;
  if (target & ISR_RULES) fn_psp = NULL;
  else conf = 100;              /* no pattern spectrum for rules */
  if (info == dflt) {           /* if default info. format is used, */
    if (target != ISR_RULES)    /* set default according to target */
         info = (supp < 0) ? " (%a)"     : " (%S)";
    else info = (supp < 0) ? " (%b, %C)" : " (%X, %C)";
  }                             /* select absolute/relative support */
  thresh *= 0.01;               /* scale the evaluation threshold */
  MSG(stderr, "\n");            /* terminate the startup message */

  /* --- read item selection/appearance indicators --- */
  ibase = ib_create(0, 0);      /* create an item base */
  if (!ibase) error(E_NOMEM);   /* to manage the items */
  tread = trd_create();         /* create a transaction reader */
  if (!tread) error(E_NOMEM);   /* and configure the characters */
  trd_allchs(tread, recseps, fldseps, blanks, "", comment);
  if (fn_sel) {                 /* if an item selection is given */
    CLOCK(t);                   /* start timer, open input file */
    if (trd_open(tread, NULL, fn_sel) != 0)
      error(E_FOPEN, trd_name(tread));
    MSG(stderr, "reading %s ... ", trd_name(tread));
    m = (target == ISR_RULES)   /* depending on the target type */
      ? ib_readapp(ibase,tread) /* read the item appearances */
      : ib_readsel(ibase,tread);/* or a simple item selection */
    if (m < 0) error((int)-m, ib_errmsg(ibase, NULL, 0));
    trd_close(tread);           /* close the input file */
    MSG(stderr, "[%"ITEM_FMT" item(s)]", ib_cnt(ibase));
    MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* print a log message */

  /* --- read transaction database --- */
  tabag = tbg_create(ibase);    /* create a transaction bag */
  if (!tabag) error(E_NOMEM);   /* to store the transactions */
  CLOCK(t);                     /* start timer, open input file */
  if (trd_open(tread, NULL, fn_inp) != 0)
    error(E_FOPEN, trd_name(tread));
  MSG(stderr, "reading %s ... ", trd_name(tread));
  k = tbg_read(tabag, tread, mtar);
  if (k < 0) error(-k, tbg_errmsg(tabag, NULL, 0));
  trd_delete(tread, 1);         /* read the transaction database, */
  tread = NULL;                 /* then delete the table reader */
  m = ib_cnt(ibase);            /* get the number of items, */
  n = tbg_cnt(tabag);           /* the number of transactions, */
  w = tbg_wgt(tabag);           /* the total transaction weight */
  MSG(stderr, "[%"ITEM_FMT" item(s), %"TID_FMT, m, n);
  if (w != (SUPP)n) { MSG(stderr, "/%"SUPP_FMT, w); }
  MSG(stderr, " transaction(s)] done [%.2fs].", SEC_SINCE(t));
  if ((m <= 0) || (n <= 0))     /* check for at least one item */
    error(E_NOITEMS);           /* and at least one transaction */
  MSG(stderr, "\n");            /* terminate the log message */
  conf *= 0.01;                 /* scale the minimum confidence */
  supp = (supp >= 0) ? 0.01 *supp *(double)w *(1-DBL_EPSILON) : -supp;
  smax = (smax >= 0) ? 0.01 *smax *(double)w *(1+DBL_EPSILON) : -smax;
  body = (SUPP)ceilsupp(supp);  /* compute absolute support values */
  smin = (SUPP)ceilsupp((!(target & ISR_RULES) || orig)
       ? supp : ceilsupp(supp) *conf *(1-DBL_EPSILON));
  if (algo == ECL_AUTO) {       /* if automatic variant choice */
    m    = ib_frqcnt(tbg_base(tabag), smin);
    algo = ((target & (ISR_CLOSED|ISR_MAXIMAL))
        && ((double)tbg_extent(tabag) /((double)m*(double)w) > 0.02))
         ? ECL_LISTS : ECL_OCCDLV;
  }                             /* choose the eclat variant */

  /* --- find frequent item sets/association rules --- */
  mode |= ECL_VERBOSE|ECL_NOCLEAN;
  k = eclat_data(tabag, target, smin, zmin, eval, algo, mode, sort);
  if (k) error(k);              /* prepare data for Eclat */
  report = isr_create(ibase);   /* create an item set reporter */
  if (!report) error(E_NOMEM);  /* and configure it */
  isr_setsize(report,        zmin, zmax);
  isr_setsupp(report, (RSUPP)smin, (RSUPP)floorsupp(smax));
  if (setbdr(report, w, zmin, &border, bdrcnt) != 0)
    error(E_NOMEM);             /* set limits and support border */
  if (fn_psp && (isr_addpsp(report, NULL) < 0))
    error(E_NOMEM);             /* set a pattern spectrum if req. */
  if (isr_setfmt(report, scan, hdr, sep, imp, info) != 0)
    error(E_NOMEM);             /* set the output format strings */
  k = isr_open(report, NULL, fn_out);      /* open item set file */
  if (k) error(k, isr_name(report));
  if (fn_tid) {                 /* if to write transaction ids */
    if (isr_tidopen(report, NULL, fn_tid) != 0)
      error(E_FOPEN, isr_tidname(report));
    if (isr_tidpfmt(report, (pfids) ? n : 0) != 0)
      error(E_NOMEM);           /* open transaction id file and */
  }                             /* preformat transaction ids */
  if ((eclat_repo(report, target, eval, thresh, algo, mode) < 0)
  ||  (isr_setup(report) < 0))  /* prepare reporter for Eclat and */
    error(E_NOMEM);             /* set up the item set reporter */
  k = eclat(tabag, target, smin, body, conf,
            eval, agg, thresh, prune, algo, mode, 0, report);
  if (k) error(k);              /* find frequent item sets */
  if (stats)                    /* print item set statistics */
    isr_prstats(report, stdout, 0);
  if (isr_close   (report) != 0)/* close item set output file */
    error(E_FWRITE, isr_name(report));
  if (isr_tidclose(report) != 0)/* close trans. id output file */
    error(E_FWRITE, isr_tidname(report));

  /* --- write pattern spectrum --- */
  if (fn_psp) {                 /* if to write a pattern spectrum */
    CLOCK(t);                   /* start timer, create table write */
    psp    = isr_getpsp(report);/* get the pattern spectrum */
    twrite = twr_create();      /* create a table writer and */
    if (!twrite) error(E_NOMEM);/* open the output file */
    if (twr_open(twrite, NULL, fn_psp) != 0)
      error(E_FOPEN,  twr_name(twrite));
    MSG(stderr, "writing %s ... ", twr_name(twrite));
    if (psp_report(psp, twrite, 1.0) != 0)
      error(E_FWRITE, twr_name(twrite));
    twr_delete(twrite, 1);      /* write the pattern spectrum */
    twrite = NULL;              /* and delete the table writer */
    MSG(stderr, "[%"SIZE_FMT" signature(s)]", psp_sigcnt(psp));
    MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* write a log message */

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */

#endif  /* #ifdef ECL_MAIN ... */
