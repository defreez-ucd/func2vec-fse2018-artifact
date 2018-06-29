/*----------------------------------------------------------------------
  File    : eclat.h
  Contents: eclat algorithm for finding frequent item sets
  Author  : Christian Borgelt
  History : 2011.08.22 file created
            2011.08.31 occurrence deliver variant ECL_OCCDLV added
            2012.04.17 interface for external call added (e.g. python)
            2014.08.19 adapted to modified item set reporter interface
            2014.08.21 parameter 'body' added to function eclat()
            2014.08.28 functions eclat_data() and eclat_repo() added
----------------------------------------------------------------------*/
#ifndef __ECLAT__
#define __ECLAT__
#include "tract.h"
#ifndef ISR_CLOMAX
#define ISR_CLOMAX
#endif
#include "report.h"
#include "ruleval.h"
#include "istree.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- evaluation measures --- */
/* most definitions in ruleval.h */
#define ECL_LDRATIO RE_FNCNT    /* binary log. of support quotient */
#define ECL_INVBXS  IST_INVBXS  /* inval. eval. below exp. supp. */

/* --- aggregation modes --- */
#define ECL_NONE    IST_NONE    /* no aggregation (use first value) */
#define ECL_FIRST   IST_FIRST   /* no aggregation (use first value) */
#define ECL_MIN     IST_MIN     /* minimum of measure values */
#define ECL_MAX     IST_MAX     /* maximum of measure values */
#define ECL_AVG     IST_AVG     /* average of measure values */

/* --- algorithm variants --- */
#define ECL_BASIC   0           /* tid lists intersection (basic) */
#define ECL_LISTS   1           /* tid lists intersection (improved) */
#define ECL_BITS    2           /* bit vectors over transactions */
#define ECL_TABLE   3           /* item occurrence table (standard) */
#define ECL_SIMPLE  4           /* item occurrence table (simplified) */
#define ECL_RANGES  5           /* tid range lists intersection */
#define ECL_OCCDLV  6           /* occurrence deliver (LCM-style) */
#define ECL_DIFFS   7           /* tid difference sets (diffsets) */
#define ECL_AUTO    8           /* automatic choice based on data */

/* --- operation modes --- */
#define ECL_FIM16   0x001f      /* use 16 items machine (bit rep.) */
#define ECL_PERFECT 0x0020      /* perfect extension pruning */
#define ECL_REORDER 0x0040      /* reorder items in cond. databases */
#define ECL_TAIL    0x0080      /* use head union tail pruning */
#define ECL_HORZ    0x0100      /* horizontal extensions tests */
#define ECL_VERT    0x0200      /* vertical   extensions tests */
#define ECL_TIDS    0x0400      /* flag for trans. identifier output */
#define ECL_EXTCHK  (ECL_HORZ|ECL_VERT)
#define ECL_DEFAULT (ECL_PERFECT|ECL_REORDER|ECL_TAIL|ECL_FIM16)
#ifdef NDEBUG
#define ECL_NOCLEAN 0x8000      /* do not clean up memory */
#else                           /* in function eclat() */
#define ECL_NOCLEAN 0           /* in debug version */
#endif                          /* always clean up memory */
#define ECL_VERBOSE INT_MIN     /* verbose message output */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern int eclat_data (TABAG *tabag, int target, SUPP smin, ITEM zmin,
                       int eval, int algo, int mode, int sort);
extern int eclat_repo (ISREPORT *report, int target,
                       int eval, double thresh, int algo, int mode);
extern int eclat      (TABAG *tabag, int target, SUPP smin, SUPP body,
                       double conf, int eval, int agg, double thresh,
                       ITEM prune, int algo, int mode,
                       int order, ISREPORT *report);
#endif
