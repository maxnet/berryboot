/* A Bison parser, made from ext/standard/parsedate.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	tAGO	257
# define	tDAY	258
# define	tDAY_UNIT	259
# define	tDAYZONE	260
# define	tDST	261
# define	tHOUR_UNIT	262
# define	tID	263
# define	tMERIDIAN	264
# define	tMINUTE_UNIT	265
# define	tMONTH	266
# define	tMONTH_UNIT	267
# define	tSEC_UNIT	268
# define	tSNUMBER	269
# define	tUNUMBER	270
# define	tYEAR_UNIT	271
# define	tZONE	272


/*
**  Originally written by Steven M. Bellovin <smb@research.att.com> while
**  at the University of North Carolina at Chapel Hill.  Later tweaked by
**  a couple of people on Usenet.  Completely overhauled by Rich $alz
**  <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990.
**
**  This code is in the public domain and has no copyright.
*/

#ifdef PHP_WIN32
#include <malloc.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef PHP_WIN32
# include "win32/time.h"
#endif

#include <stdlib.h> // free
#include <string.h>

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif

#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISUPPER(c) (IN_CTYPE_DOMAIN (c) && isupper (c))
#define ISDIGIT_LOCALE(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))

/* ISDIGIT differs from ISDIGIT_LOCALE, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   Posix 1003.2-1992 section 2.5.2.1 page 50 lines 1556-1558 says that
   only '0' through '9' are digits.  Prefer ISDIGIT to ISDIGIT_LOCALE unless
   it's important to use the locale's definition of `digit' even when the
   host does not conform to Posix.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

#if defined (STDC_HEADERS) || defined (USG)
# include <string.h>
#endif

#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
# define __attribute__(x)
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

/* Some old versions of bison generate parsers that use bcopy.
   That loses on systems that don't provide the function, so we have
   to redefine it here.  */
#if !defined (HAVE_BCOPY) && defined (HAVE_MEMCPY) && !defined (bcopy)
# define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror, etc),
   as well as gratuitiously global symbol names, so we can have multiple
   yacc generated parsers in the same program.  Note that these are only
   the variables produced by yacc.  If other parser generators (bison,
   byacc, etc) produce additional global names that conflict at link time,
   then those parser generators need to be fixed instead of adding those
   names to this list. */

//#define yyparse php_gd_parse
//#define yylex   php_gd_lex

static int yyerror ();

#define EPOCH		1970
#define HOUR(x)		((x) * 60)

#define MAX_BUFF_LEN    128   /* size of buffer to read the date into */

/*
**  An entry in the lexical lookup table.
*/
typedef struct _TABLE {
    const char	*name;
    int		type;
    int		value;
} TABLE;


/*
**  Meridian:  am, pm, or 24-hour style.
*/
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;

struct date_yy {
	const char	*yyInput;
	int	yyDayOrdinal;
	int	yyDayNumber;
	int	yyHaveDate;
	int	yyHaveDay;
	int	yyHaveRel;
	int	yyHaveTime;
	int	yyHaveZone;
	int	yyTimezone;
	int	yyDay;
	int	yyHour;
	int	yyMinutes;
	int	yyMonth;
	int	yySeconds;
	int	yyYear;
	MERIDIAN	yyMeridian;
	int	yyRelDay;
	int	yyRelHour;
	int	yyRelMinutes;
	int	yyRelMonth;
	int	yyRelSeconds;
	int	yyRelYear;
};

typedef union _date_ll {
    int			Number;
    enum _MERIDIAN	Meridian;
} date_ll;

#define YYPARSE_PARAM parm
#define YYLEX_PARAM parm
#define YYSTYPE date_ll
#define YYLTYPE void

#ifndef YYSTYPE
# define YYSTYPE int
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		70
#define	YYFLAG		-32768
#define	YYNTBASE	22

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 272 ? yytranslate[x] : 32)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    20,     2,     2,    21,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    19,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     4,     6,     8,    10,    12,    14,    16,
      19,    24,    29,    34,    41,    48,    55,    57,    59,    62,
      64,    67,    70,    74,    83,    89,    93,    97,   101,   104,
     109,   112,   116,   119,   121,   124,   127,   129,   132,   135,
     137,   140,   143,   145,   148,   151,   153,   156,   159,   161,
     164,   167,   169,   171,   172
};
static const short yyrhs[] =
{
      -1,    22,    23,     0,    24,     0,    25,     0,    27,     0,
      26,     0,    28,     0,    30,     0,    16,    10,     0,    16,
      19,    16,    31,     0,    16,    19,    16,    28,     0,    16,
      19,    16,    15,     0,    16,    19,    16,    19,    16,    31,
       0,    16,    19,    16,    19,    16,    28,     0,    16,    19,
      16,    19,    16,    15,     0,    18,     0,     6,     0,    18,
       7,     0,     4,     0,     4,    20,     0,    16,     4,     0,
      16,    21,    16,     0,    12,    16,    16,    19,    16,    19,
      16,    16,     0,    16,    21,    16,    21,    16,     0,    16,
      15,    15,     0,    16,    12,    15,     0,    12,    16,    16,
       0,    12,    16,     0,    12,    16,    20,    16,     0,    16,
      12,     0,    16,    12,    16,     0,    29,     3,     0,    29,
       0,    16,    17,     0,    15,    17,     0,    17,     0,    16,
      13,     0,    15,    13,     0,    13,     0,    16,     5,     0,
      15,     5,     0,     5,     0,    16,     8,     0,    15,     8,
       0,     8,     0,    16,    11,     0,    15,    11,     0,    11,
       0,    16,    14,     0,    15,    14,     0,    14,     0,    16,
       0,     0,    10,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   168,   169,   172,   175,   178,   181,   184,   187,   190,
     199,   208,   216,   228,   237,   246,   265,   268,   271,   277,
     281,   285,   291,   295,   306,   324,   330,   336,   341,   349,
     354,   362,   369,   383,   386,   389,   392,   395,   398,   401,
     404,   407,   410,   413,   416,   419,   422,   425,   428,   431,
     434,   437,   442,   477,   481
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "tAGO", "tDAY", "tDAY_UNIT", "tDAYZONE", 
  "tDST", "tHOUR_UNIT", "tID", "tMERIDIAN", "tMINUTE_UNIT", "tMONTH", 
  "tMONTH_UNIT", "tSEC_UNIT", "tSNUMBER", "tUNUMBER", "tYEAR_UNIT", 
  "tZONE", "':'", "','", "'/'", "spec", "item", "time", "zone", "day", 
  "date", "rel", "relunit", "number", "o_merid", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,    22,    22,    23,    23,    23,    23,    23,    23,    24,
      24,    24,    24,    24,    24,    24,    25,    25,    25,    26,
      26,    26,    27,    27,    27,    27,    27,    27,    27,    27,
      27,    27,    28,    28,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
      29,    29,    30,    31,    31
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     0,     2,     1,     1,     1,     1,     1,     1,     2,
       4,     4,     4,     6,     6,     6,     1,     1,     2,     1,
       2,     2,     3,     8,     5,     3,     3,     3,     2,     4,
       2,     3,     2,     1,     2,     2,     1,     2,     2,     1,
       2,     2,     1,     2,     2,     1,     2,     2,     1,     2,
       2,     1,     1,     0,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       1,     0,    19,    42,    17,    45,    48,     0,    39,    51,
       0,    52,    36,    16,     2,     3,     4,     6,     5,     7,
      33,     8,    20,    28,    41,    44,    47,    38,    50,    35,
      21,    40,    43,     9,    46,    30,    37,    49,     0,    34,
       0,     0,    18,    32,    27,     0,    26,    31,    25,    53,
      22,     0,    29,    54,    12,     0,     0,    11,    10,     0,
       0,    53,    24,     0,    15,    14,    13,     0,    23,     0,
       0
};

static const short yydefgoto[] =
{
       1,    14,    15,    16,    17,    18,    19,    20,    21,    58
};

static const short yypact[] =
{
  -32768,     0,   -10,-32768,-32768,-32768,-32768,     6,-32768,-32768,
      56,    15,-32768,    17,-32768,-32768,-32768,-32768,-32768,-32768,
      28,-32768,-32768,   -13,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,   -14,-32768,-32768,    18,-32768,
      21,    23,-32768,-32768,    31,    26,-32768,-32768,-32768,    30,
      34,    36,-32768,-32768,    56,    63,    46,-32768,-32768,    47,
      53,    43,-32768,    49,    56,-32768,-32768,    50,-32768,    75,
  -32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,-32768,-32768,-32768,   -40,-32768,-32768,    20
};


#define	YYLAST		81


static const short yytable[] =
{
      69,    46,    47,    44,     2,     3,     4,    45,     5,    57,
      22,     6,     7,     8,     9,    10,    11,    12,    13,    30,
      31,    65,    23,    32,    42,    33,    34,    35,    36,    37,
      38,    43,    39,    48,    40,     3,    41,    49,     5,    50,
      53,     6,    52,     8,     9,    54,    55,    12,     3,    56,
      51,     5,    60,    53,     6,    59,     8,     9,    64,    55,
      12,    24,    61,    62,    25,    67,    68,    26,    31,    27,
      28,    32,    63,    29,    34,    70,    36,    37,     0,     0,
      39,    66
};

static const short yycheck[] =
{
       0,    15,    16,    16,     4,     5,     6,    20,     8,    49,
      20,    11,    12,    13,    14,    15,    16,    17,    18,     4,
       5,    61,    16,     8,     7,    10,    11,    12,    13,    14,
      15,     3,    17,    15,    19,     5,    21,    16,     8,    16,
      10,    11,    16,    13,    14,    15,    16,    17,     5,    19,
      19,     8,    16,    10,    11,    21,    13,    14,    15,    16,
      17,     5,    16,    16,     8,    16,    16,    11,     5,    13,
      14,     8,    19,    17,    11,     0,    13,    14,    -1,    -1,
      17,    61
};
#define YYPURE 1

/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif
#endif



/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;

static int yylex (YYSTYPE *lvalp, void *parm);
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 3:
{
	    ((struct date_yy *)parm)->yyHaveTime++;
	}
    break;
case 4:
{
	    ((struct date_yy *)parm)->yyHaveZone++;
	}
    break;
case 5:
{
	    ((struct date_yy *)parm)->yyHaveDate++;
	}
    break;
case 6:
{
	    ((struct date_yy *)parm)->yyHaveDay++;
	}
    break;
case 7:
{
	    ((struct date_yy *)parm)->yyHaveRel++;
	}
    break;
case 9:
{
	    ((struct date_yy *)parm)->yyHour = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyMinutes = 0;
	    ((struct date_yy *)parm)->yySeconds = 0;
	    ((struct date_yy *)parm)->yyMeridian = yyvsp[0].Meridian;
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U M]\n");
#endif
	}
    break;
case 10:
{
	    ((struct date_yy *)parm)->yyHour = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yyMinutes = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yySeconds = 0;
	    ((struct date_yy *)parm)->yyMeridian = yyvsp[0].Meridian;
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U:U M]\n");
#endif
	}
    break;
case 11:
{
	    ((struct date_yy *)parm)->yyHour = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yyMinutes = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyMeridian = MER24;
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U:U rel]\n");
#endif
	}
    break;
case 12:
{
	    ((struct date_yy *)parm)->yyHour = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yyMinutes = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	    ((struct date_yy *)parm)->yyHaveZone++;
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[0].Number < 0
			  ? -yyvsp[0].Number % 100 + (-yyvsp[0].Number / 100) * 60
			  : - (yyvsp[0].Number % 100 + (yyvsp[0].Number / 100) * 60));
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U:U S]\n");
#endif
	}
    break;
case 13:
{
	    ((struct date_yy *)parm)->yyHour = yyvsp[-5].Number;
	    ((struct date_yy *)parm)->yyMinutes = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yySeconds = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyMeridian = yyvsp[0].Meridian;
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U:U:U M]\n");
#endif
	}
    break;
case 14:
{
	    /* ISO 8601 format.  hh:mm:ss[+-][0-9]{2}([0-9]{2})?.  */
	    ((struct date_yy *)parm)->yyHour = yyvsp[-5].Number;
	    ((struct date_yy *)parm)->yyMinutes = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yySeconds = yyvsp[-1].Number;
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U:U:U rel]\n");
#endif
	}
    break;
case 15:
{
	    /* ISO 8601 format.  hh:mm:ss[+-][0-9]{2}([0-9]{2})?.  */
	    ((struct date_yy *)parm)->yyHour = yyvsp[-5].Number;
	    ((struct date_yy *)parm)->yyMinutes = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yySeconds = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	    ((struct date_yy *)parm)->yyHaveZone++;
		if (yyvsp[0].Number <= -100 || yyvsp[0].Number >= 100) {
			((struct date_yy *)parm)->yyTimezone =  
				-yyvsp[0].Number % 100 + (-yyvsp[0].Number / 100) * 60;
		} else {
			((struct date_yy *)parm)->yyTimezone =  -yyvsp[0].Number * 60;
		}
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
		printf("[U:U:U S]\n");
#endif
	}
    break;
case 16:
{
	    ((struct date_yy *)parm)->yyTimezone = yyvsp[0].Number;
	}
    break;
case 17:
{
	    ((struct date_yy *)parm)->yyTimezone = yyvsp[0].Number - 60;
	}
    break;
case 18:
{
	    ((struct date_yy *)parm)->yyTimezone = yyvsp[-1].Number - 60;
	}
    break;
case 19:
{
	    ((struct date_yy *)parm)->yyDayOrdinal = 1;
	    ((struct date_yy *)parm)->yyDayNumber = yyvsp[0].Number;
	}
    break;
case 20:
{
	    ((struct date_yy *)parm)->yyDayOrdinal = 1;
	    ((struct date_yy *)parm)->yyDayNumber = yyvsp[-1].Number;
	}
    break;
case 21:
{
	    ((struct date_yy *)parm)->yyDayOrdinal = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyDayNumber = yyvsp[0].Number;
	}
    break;
case 22:
{
	    ((struct date_yy *)parm)->yyMonth = yyvsp[-2].Number;
	    ((struct date_yy *)parm)->yyDay = yyvsp[0].Number;
	}
    break;
case 23:
{
		((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
		((struct date_yy *)parm)->yyMonth = yyvsp[-7].Number;
		((struct date_yy *)parm)->yyDay = yyvsp[-6].Number;

		((struct date_yy *)parm)->yyHour = yyvsp[-5].Number;
		((struct date_yy *)parm)->yyMinutes = yyvsp[-3].Number;
		((struct date_yy *)parm)->yySeconds = yyvsp[-1].Number;

		((struct date_yy *)parm)->yyHaveTime = 1;
	}
    break;
case 24:
{
	  /* Interpret as YYYY/MM/DD if $1 >= 1000, otherwise as MM/DD/YY.
	     The goal in recognizing YYYY/MM/DD is solely to support legacy
	     machine-generated dates like those in an RCS log listing.  If
	     you want portability, use the ISO 8601 format.  */
	  if (yyvsp[-4].Number >= 1000)
	    {
	      ((struct date_yy *)parm)->yyYear = yyvsp[-4].Number;
	      ((struct date_yy *)parm)->yyMonth = yyvsp[-2].Number;
	      ((struct date_yy *)parm)->yyDay = yyvsp[0].Number;
	    }
	  else
	    {
	      ((struct date_yy *)parm)->yyMonth = yyvsp[-4].Number;
	      ((struct date_yy *)parm)->yyDay = yyvsp[-2].Number;
	      ((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
	    }
	}
    break;
case 25:
{
	    /* ISO 8601 format.  yyyy-mm-dd.  */
	    ((struct date_yy *)parm)->yyYear = yyvsp[-2].Number;
	    ((struct date_yy *)parm)->yyMonth = -yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyDay = -yyvsp[0].Number;
	}
    break;
case 26:
{
	    /* e.g. 17-JUN-1992.  */
	    ((struct date_yy *)parm)->yyDay = yyvsp[-2].Number;
	    ((struct date_yy *)parm)->yyMonth = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyYear = -yyvsp[0].Number;
	}
    break;
case 27:
{
	    ((struct date_yy *)parm)->yyMonth = yyvsp[-2].Number;
	    ((struct date_yy *)parm)->yyDay = yyvsp[-1].Number;
		((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
	}
    break;
case 28:
{
	    ((struct date_yy *)parm)->yyMonth = yyvsp[-1].Number;
	    if (yyvsp[0].Number > 1000) {
		((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
	    } else {
		((struct date_yy *)parm)->yyDay = yyvsp[0].Number;
	    }
	}
    break;
case 29:
{
	    ((struct date_yy *)parm)->yyMonth = yyvsp[-3].Number;
	    ((struct date_yy *)parm)->yyDay = yyvsp[-2].Number;
	    ((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
	}
    break;
case 30:
{
	    ((struct date_yy *)parm)->yyMonth = yyvsp[0].Number;
	    if (yyvsp[-1].Number > 1000) {
		((struct date_yy *)parm)->yyYear = yyvsp[-1].Number;
	    } else {
		((struct date_yy *)parm)->yyDay = yyvsp[-1].Number;
	    }
	}
    break;
case 31:
{
	    ((struct date_yy *)parm)->yyMonth = yyvsp[-1].Number;
	    ((struct date_yy *)parm)->yyDay = yyvsp[-2].Number;
	    ((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
	}
    break;
case 32:
{
	    ((struct date_yy *)parm)->yyRelSeconds =
			-((struct date_yy *)parm)->yyRelSeconds;
	    ((struct date_yy *)parm)->yyRelMinutes =
			-((struct date_yy *)parm)->yyRelMinutes;
	    ((struct date_yy *)parm)->yyRelHour =
			-((struct date_yy *)parm)->yyRelHour;
	    ((struct date_yy *)parm)->yyRelDay =
			-((struct date_yy *)parm)->yyRelDay;
	    ((struct date_yy *)parm)->yyRelMonth =
			-((struct date_yy *)parm)->yyRelMonth;
	    ((struct date_yy *)parm)->yyRelYear =
			-((struct date_yy *)parm)->yyRelYear;
	}
    break;
case 34:
{
	    ((struct date_yy *)parm)->yyRelYear += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 35:
{
	    ((struct date_yy *)parm)->yyRelYear += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 36:
{
	    ((struct date_yy *)parm)->yyRelYear += yyvsp[0].Number;
	}
    break;
case 37:
{
	    ((struct date_yy *)parm)->yyRelMonth += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 38:
{
	    ((struct date_yy *)parm)->yyRelMonth += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 39:
{
	    ((struct date_yy *)parm)->yyRelMonth += yyvsp[0].Number;
	}
    break;
case 40:
{
	    ((struct date_yy *)parm)->yyRelDay += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 41:
{
	    ((struct date_yy *)parm)->yyRelDay += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 42:
{
	    ((struct date_yy *)parm)->yyRelDay += yyvsp[0].Number;
	}
    break;
case 43:
{
	    ((struct date_yy *)parm)->yyRelHour += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 44:
{
	    ((struct date_yy *)parm)->yyRelHour += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 45:
{
	    ((struct date_yy *)parm)->yyRelHour += yyvsp[0].Number;
	}
    break;
case 46:
{
	    ((struct date_yy *)parm)->yyRelMinutes += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 47:
{
	    ((struct date_yy *)parm)->yyRelMinutes += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 48:
{
	    ((struct date_yy *)parm)->yyRelMinutes += yyvsp[0].Number;
	}
    break;
case 49:
{
	    ((struct date_yy *)parm)->yyRelSeconds += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 50:
{
	    ((struct date_yy *)parm)->yyRelSeconds += yyvsp[-1].Number * yyvsp[0].Number;
	}
    break;
case 51:
{
	    ((struct date_yy *)parm)->yyRelSeconds += yyvsp[0].Number;
	}
    break;
case 52:
{
	    if (((struct date_yy *)parm)->yyHaveTime && 
			((struct date_yy *)parm)->yyHaveDate && 
			!((struct date_yy *)parm)->yyHaveRel)
	      ((struct date_yy *)parm)->yyYear = yyvsp[0].Number;
	    else
	      {
		if (yyvsp[0].Number>10000)
		  {
		    ((struct date_yy *)parm)->yyHaveDate++;
		    ((struct date_yy *)parm)->yyDay= (yyvsp[0].Number)%100;
		    ((struct date_yy *)parm)->yyMonth= (yyvsp[0].Number/100)%100;
		    ((struct date_yy *)parm)->yyYear = yyvsp[0].Number/10000;
		  }
		else
		  {
		    ((struct date_yy *)parm)->yyHaveTime++;
		    if (yyvsp[0].Number < 100)
		      {
			((struct date_yy *)parm)->yyHour = yyvsp[0].Number;
			((struct date_yy *)parm)->yyMinutes = 0;
		      }
		    else
		      {
		    	((struct date_yy *)parm)->yyHour = yyvsp[0].Number / 100;
		    	((struct date_yy *)parm)->yyMinutes = yyvsp[0].Number % 100;
		      }
		    ((struct date_yy *)parm)->yySeconds = 0;
		    ((struct date_yy *)parm)->yyMeridian = MER24;
		  }
	      }
	  }
    break;
case 53:
{
	    yyval.Meridian = MER24;
	  }
    break;
case 54:
{
	    yyval.Meridian = yyvsp[0].Meridian;
	  }
    break;
}



  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


time_t get_date (char *p, time_t *now);

#ifndef PHP_WIN32
extern struct tm	*gmtime();
extern struct tm	*localtime();
extern time_t		mktime();
#endif

/* Month and day table. */
static TABLE const MonthDayTable[] = {
    { "january",	tMONTH,  1 },
    { "february",	tMONTH,  2 },
    { "march",		tMONTH,  3 },
    { "april",		tMONTH,  4 },
    { "may",		tMONTH,  5 },
    { "june",		tMONTH,  6 },
    { "july",		tMONTH,  7 },
    { "august",		tMONTH,  8 },
    { "september",	tMONTH,  9 },
    { "sept",		tMONTH,  9 },
    { "october",	tMONTH, 10 },
    { "november",	tMONTH, 11 },
    { "december",	tMONTH, 12 },
    { "sunday",		tDAY, 0 },
    { "monday",		tDAY, 1 },
    { "tuesday",	tDAY, 2 },
    { "tues",		tDAY, 2 },
    { "wednesday",	tDAY, 3 },
    { "wednes",		tDAY, 3 },
    { "thursday",	tDAY, 4 },
    { "thur",		tDAY, 4 },
    { "thurs",		tDAY, 4 },
    { "friday",		tDAY, 5 },
    { "saturday",	tDAY, 6 },
    { NULL, 0, 0 }
};

/* Time units table. */
static TABLE const UnitsTable[] = {
    { "year",		tYEAR_UNIT,	1 },
    { "month",		tMONTH_UNIT,	1 },
    { "fortnight",	tDAY_UNIT,	14 },
    { "week",		tDAY_UNIT,	7 },
    { "day",		tDAY_UNIT,	1 },
    { "hour",		tHOUR_UNIT,	1 },
    { "minute",		tMINUTE_UNIT,	1 },
    { "min",		tMINUTE_UNIT,	1 },
    { "second",		tSEC_UNIT,	1 },
    { "sec",		tSEC_UNIT,	1 },
    { NULL, 0, 0 }
};

/* Assorted relative-time words. */
static TABLE const OtherTable[] = {
    { "tomorrow",	tDAY_UNIT,	1 },
    { "yesterday",	tDAY_UNIT,	-1 },
    { "today",		tDAY_UNIT,	0 },
    { "now",		tDAY_UNIT,	0 },
    { "last",		tUNUMBER,	-1 },
    { "this",		tUNUMBER,	0 },
    { "next",		tUNUMBER,	2 },
    { "first",		tUNUMBER,	1 },
/*  { "second",		tUNUMBER,	2 }, */
    { "third",		tUNUMBER,	3 },
    { "fourth",		tUNUMBER,	4 },
    { "fifth",		tUNUMBER,	5 },
    { "sixth",		tUNUMBER,	6 },
    { "seventh",	tUNUMBER,	7 },
    { "eighth",		tUNUMBER,	8 },
    { "ninth",		tUNUMBER,	9 },
    { "tenth",		tUNUMBER,	10 },
    { "eleventh",	tUNUMBER,	11 },
    { "twelfth",	tUNUMBER,	12 },
    { "ago",		tAGO,	1 },
    { NULL, 0, 0 }
};

/* The timezone table. */
static TABLE const TimezoneTable[] = {
    { "gmt",	tZONE,     HOUR ( 0) },	/* Greenwich Mean */
    { "ut",	tZONE,     HOUR ( 0) },	/* Universal (Coordinated) */
    { "utc",	tZONE,     HOUR ( 0) },
    { "wet",	tZONE,     HOUR ( 0) },	/* Western European */
    { "bst",	tDAYZONE,  HOUR ( 0) },	/* British Summer */
    { "wat",	tZONE,     HOUR ( 1) },	/* West Africa */
    { "at",	tZONE,     HOUR ( 2) },	/* Azores */
#if	0
    /* For completeness.  BST is also British Summer, and GST is
     * also Guam Standard. */
    { "bst",	tZONE,     HOUR ( 3) },	/* Brazil Standard */
    { "gst",	tZONE,     HOUR ( 3) },	/* Greenland Standard */
#endif
#if 0
    { "nft",	tZONE,     HOUR (3.5) },	/* Newfoundland */
    { "nst",	tZONE,     HOUR (3.5) },	/* Newfoundland Standard */
    { "ndt",	tDAYZONE,  HOUR (3.5) },	/* Newfoundland Daylight */
#endif
    { "ast",	tZONE,     HOUR ( 4) },	/* Atlantic Standard */
    { "adt",	tDAYZONE,  HOUR ( 4) },	/* Atlantic Daylight */
    { "est",	tZONE,     HOUR ( 5) },	/* Eastern Standard */
    { "edt",	tDAYZONE,  HOUR ( 5) },	/* Eastern Daylight */
    { "cst",	tZONE,     HOUR ( 6) },	/* Central Standard */
    { "cdt",	tDAYZONE,  HOUR ( 6) },	/* Central Daylight */
    { "mst",	tZONE,     HOUR ( 7) },	/* Mountain Standard */
    { "mdt",	tDAYZONE,  HOUR ( 7) },	/* Mountain Daylight */
    { "pst",	tZONE,     HOUR ( 8) },	/* Pacific Standard */
    { "pdt",	tDAYZONE,  HOUR ( 8) },	/* Pacific Daylight */
    { "yst",	tZONE,     HOUR ( 9) },	/* Yukon Standard */
    { "ydt",	tDAYZONE,  HOUR ( 9) },	/* Yukon Daylight */
    { "hst",	tZONE,     HOUR (10) },	/* Hawaii Standard */
    { "hdt",	tDAYZONE,  HOUR (10) },	/* Hawaii Daylight */
    { "cat",	tZONE,     HOUR (10) },	/* Central Alaska */
    { "akst",	tZONE,     HOUR (10) }, /* Alaska Standard */
    { "akdt",	tZONE,     HOUR (10) }, /* Alaska Daylight */
    { "ahst",	tZONE,     HOUR (10) },	/* Alaska-Hawaii Standard */
    { "nt",	tZONE,     HOUR (11) },	/* Nome */
    { "idlw",	tZONE,     HOUR (12) },	/* International Date Line West */
    { "cet",	tZONE,     -HOUR (1) },	/* Central European */
    { "cest",	tDAYZONE,  -HOUR (1) },	/* Central European Summer */
    { "met",	tZONE,     -HOUR (1) },	/* Middle European */
    { "mewt",	tZONE,     -HOUR (1) },	/* Middle European Winter */
    { "mest",	tDAYZONE,  -HOUR (1) },	/* Middle European Summer */
    { "mesz",	tDAYZONE,  -HOUR (1) },	/* Middle European Summer */
    { "swt",	tZONE,     -HOUR (1) },	/* Swedish Winter */
    { "sst",	tDAYZONE,  -HOUR (1) },	/* Swedish Summer */
    { "fwt",	tZONE,     -HOUR (1) },	/* French Winter */
    { "fst",	tDAYZONE,  -HOUR (1) },	/* French Summer */
    { "eet",	tZONE,     -HOUR (2) },	/* Eastern Europe, USSR Zone 1 */
    { "bt",	tZONE,     -HOUR (3) },	/* Baghdad, USSR Zone 2 */
#if 0
    { "it",	tZONE,     -HOUR (3.5) },/* Iran */
#endif
    { "zp4",	tZONE,     -HOUR (4) },	/* USSR Zone 3 */
    { "zp5",	tZONE,     -HOUR (5) },	/* USSR Zone 4 */
#if 0
    { "ist",	tZONE,     -HOUR (5.5) },/* Indian Standard */
#endif
    { "zp6",	tZONE,     -HOUR (6) },	/* USSR Zone 5 */
#if	0
    /* For completeness.  NST is also Newfoundland Standard, and SST is
     * also Swedish Summer. */
    { "nst",	tZONE,     -HOUR (6.5) },/* North Sumatra */
    { "sst",	tZONE,     -HOUR (7) },	/* South Sumatra, USSR Zone 6 */
#endif	/* 0 */
    { "wast",	tZONE,     -HOUR (7) },	/* West Australian Standard */
    { "wadt",	tDAYZONE,  -HOUR (7) },	/* West Australian Daylight */
#if 0
    { "jt",	tZONE,     -HOUR (7.5) },/* Java (3pm in Cronusland!) */
#endif
    { "cct",	tZONE,     -HOUR (8) },	/* China Coast, USSR Zone 7 */
    { "jst",	tZONE,     -HOUR (9) },	/* Japan Standard, USSR Zone 8 */
#if 0
    { "cast",	tZONE,     -HOUR (9.5) },/* Central Australian Standard */
    { "cadt",	tDAYZONE,  -HOUR (9.5) },/* Central Australian Daylight */
#endif
    { "east",	tZONE,     -HOUR (10) },	/* Eastern Australian Standard */
    { "eadt",	tDAYZONE,  -HOUR (10) },	/* Eastern Australian Daylight */
    { "gst",	tZONE,     -HOUR (10) },	/* Guam Standard, USSR Zone 9 */
    { "nzt",	tZONE,     -HOUR (12) },	/* New Zealand */
    { "nzst",	tZONE,     -HOUR (12) },	/* New Zealand Standard */
    { "nzdt",	tDAYZONE,  -HOUR (12) },	/* New Zealand Daylight */
    { "idle",	tZONE,     -HOUR (12) },	/* International Date Line East */
    {  NULL, 0, 0  }
};

/* Military timezone table. */
static TABLE const MilitaryTable[] = {
    { "a",	tZONE,	HOUR (- 1) },
    { "b",	tZONE,	HOUR (- 2) },
    { "c",	tZONE,	HOUR (- 3) },
    { "d",	tZONE,	HOUR (- 4) },
    { "e",	tZONE,	HOUR (- 5) },
    { "f",	tZONE,	HOUR (- 6) },
    { "g",	tZONE,	HOUR (- 7) },
    { "h",	tZONE,	HOUR (- 8) },
    { "i",	tZONE,	HOUR (- 9) },
    { "k",	tZONE,	HOUR (-10) },
    { "l",	tZONE,	HOUR (-11) },
    { "m",	tZONE,	HOUR (-12) },
    { "n",	tZONE,	HOUR (  1) },
    { "o",	tZONE,	HOUR (  2) },
    { "p",	tZONE,	HOUR (  3) },
    { "q",	tZONE,	HOUR (  4) },
    { "r",	tZONE,	HOUR (  5) },
    { "s",	tZONE,	HOUR (  6) },
    { "t",	tZONE,	HOUR (  7) },
    { "u",	tZONE,	HOUR (  8) },
    { "v",	tZONE,	HOUR (  9) },
    { "w",	tZONE,	HOUR ( 10) },
    { "x",	tZONE,	HOUR ( 11) },
    { "y",	tZONE,	HOUR ( 12) },
    { "z",	tZONE,	HOUR (  0) },
    { NULL, 0, 0 }
};




/* ARGSUSED */
static int
yyerror (s)
     char *s ATTRIBUTE_UNUSED;
{
  return 0;
}

static int
ToHour (Hours, Meridian)
     int Hours;
     MERIDIAN Meridian;
{
  switch (Meridian)
    {
    case MER24:
      if (Hours < 0 || Hours > 23)
	return -1;
      return Hours;
    case MERam:
      if (Hours < 1 || Hours > 12)
	return -1;
      if (Hours == 12)
	Hours = 0;
      return Hours;
    case MERpm:
      if (Hours < 1 || Hours > 12)
	return -1;
      if (Hours == 12)
	Hours = 0;
      return Hours + 12;
    default:
      abort ();
    }
  /* NOTREACHED */
}

static int
ToYear (Year)
     int Year;
{
  if (Year < 0)
    Year = -Year;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  if (Year < 69)
    Year += 2000;
  else if (Year < 100)
    Year += 1900;

  return Year;
}

static int
LookupWord (lvalp,buff)
	YYSTYPE *lvalp;
     char *buff;
{
  register char *p;
  register char *q;
  register const TABLE *tp;
  int i;
  int abbrev;

  /* Make it lowercase. */
  for (p = buff; *p; p++)
    if (ISUPPER ((unsigned char) *p))
      *p = tolower (*p);

  if (strcmp (buff, "am") == 0 || strcmp (buff, "a.m.") == 0)
    {
      lvalp->Meridian = MERam;
      return tMERIDIAN;
    }
  if (strcmp (buff, "pm") == 0 || strcmp (buff, "p.m.") == 0)
    {
      lvalp->Meridian = MERpm;
      return tMERIDIAN;
    }

  /* See if we have an abbreviation for a month. */
  if (strlen (buff) == 3)
    abbrev = 1;
  else if (strlen (buff) == 4 && buff[3] == '.')
    {
      abbrev = 1;
      buff[3] = '\0';
    }
  else
    abbrev = 0;

  for (tp = MonthDayTable; tp->name; tp++)
    {
      if (abbrev)
	{
	  if (strncmp (buff, tp->name, 3) == 0)
	    {
	      lvalp->Number = tp->value;
	      return tp->type;
	    }
	}
      else if (strcmp (buff, tp->name) == 0)
	{
	  lvalp->Number = tp->value;
	  return tp->type;
	}
    }

  for (tp = TimezoneTable; tp->name; tp++)
    if (strcmp (buff, tp->name) == 0)
      {
	lvalp->Number = tp->value;
	return tp->type;
      }

  if (strcmp (buff, "dst") == 0)
    return tDST;

  for (tp = UnitsTable; tp->name; tp++)
    if (strcmp (buff, tp->name) == 0)
      {
	lvalp->Number = tp->value;
	return tp->type;
      }

  /* Strip off any plural and try the units table again. */
  i = strlen (buff) - 1;
  if (buff[i] == 's')
    {
      buff[i] = '\0';
      for (tp = UnitsTable; tp->name; tp++)
	if (strcmp (buff, tp->name) == 0)
	  {
	    lvalp->Number = tp->value;
	    return tp->type;
	  }
      buff[i] = 's';		/* Put back for "this" in OtherTable. */
    }

  for (tp = OtherTable; tp->name; tp++)
    if (strcmp (buff, tp->name) == 0)
      {
	lvalp->Number = tp->value;
	return tp->type;
      }

  /* Military timezones. */
  if (buff[1] == '\0' && ISALPHA ((unsigned char) *buff))
    {
      for (tp = MilitaryTable; tp->name; tp++)
	if (strcmp (buff, tp->name) == 0)
	  {
	    lvalp->Number = tp->value;
	    return tp->type;
	  }
    }

  /* Drop out any periods and try the timezone table again. */
  for (i = 0, p = q = buff; *q; q++)
    if (*q != '.')
      *p++ = *q;
    else
      i++;
  *p = '\0';
  if (i)
    for (tp = TimezoneTable; tp->name; tp++)
      if (strcmp (buff, tp->name) == 0)
	{
	  lvalp->Number = tp->value;
	  return tp->type;
	}

  return tID;
}

int yylex(YYSTYPE *lvalp, void *parm)
{
  register unsigned char c;
  register char *p;
  char buff[20];
  int Count;
  int sign;
  struct date_yy * date = (struct date_yy *)parm;

  for (;;)
    {
      while (ISSPACE ((unsigned char) *date->yyInput))
	date->yyInput++;

      if (ISDIGIT (c = *date->yyInput) || c == '-' || c == '+')
	{
	  if (c == '-' || c == '+')
	    {
	      sign = c == '-' ? -1 : 1;
	      if (!ISDIGIT (*++date->yyInput))
		/* skip the '-' sign */
		continue;
	    }
	  else
	    sign = 0;
	  for (lvalp->Number = 0; ISDIGIT (c = *date->yyInput++);)
	    lvalp->Number = 10 * lvalp->Number + c - '0';
	  date->yyInput--;
	  if (sign < 0)
	    lvalp->Number = -lvalp->Number;
	  /* Ignore ordinal suffixes on numbers */
	  c = *date->yyInput;
	  if (c == 's' || c == 'n' || c == 'r' || c == 't') {
	    c = *++date->yyInput;
	    if (c == 't' || c == 'd' || c == 'h') {
	      date->yyInput++;
	    } else {
	      date->yyInput--;
	    }
	  }
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
	  printf ("T: %s\n", sign ? "tS" : "tU");
#endif
	  return sign ? tSNUMBER : tUNUMBER;
	}
      if (ISALPHA (c))
	{
	  for (p = buff; (c = *date->yyInput++, ISALPHA (c)) || c == '.';)
	    if (p < &buff[sizeof buff - 1])
	      *p++ = c;
	  *p = '\0';
	  date->yyInput--;
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
	  printf ("T: LW\n");
#endif
	  return LookupWord (lvalp, buff);
	}
      if (c != '(') {
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
	printf ("T: %c\n", *date->yyInput);
#endif
	return *date->yyInput++;
	  }
      Count = 0;
      do
	{
	  c = *date->yyInput++;
	  if (c == '\0') {
#ifdef PHP_DEBUG_PARSE_DATE_PARSER
	printf ("T: %c\n", c);
#endif
	    return c;
	  }
	  if (c == '(')
	    Count++;
	  else if (c == ')')
	    Count--;
	}
      while (Count > 0);
    }
}

#define TM_YEAR_ORIGIN 1900

/* Yield A - B, measured in seconds.  */
static long
difftm (struct tm *a, struct tm *b)
{
  int ay = a->tm_year + (TM_YEAR_ORIGIN - 1);
  int by = b->tm_year + (TM_YEAR_ORIGIN - 1);
  long days = (
  /* difference in day of year */
		a->tm_yday - b->tm_yday
  /* + intervening leap days */
		+ ((ay >> 2) - (by >> 2))
		- (ay / 100 - by / 100)
		+ ((ay / 100 >> 2) - (by / 100 >> 2))
  /* + difference in years * 365 */
		+ (long) (ay - by) * 365
  );
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
		+ (a->tm_min - b->tm_min))
	  + (a->tm_sec - b->tm_sec));
}

time_t parse_date(char *p, time_t *now)
{
  struct tm tm, tm0, *tmp;
  time_t Start;
  struct date_yy date;

  date.yyInput = p;
  Start = now ? *now : time ((time_t *) NULL);
  tmp = localtime (&Start);
  if (!tmp)
    return -1;
  date.yyYear = tmp->tm_year + TM_YEAR_ORIGIN;
  date.yyMonth = tmp->tm_mon + 1;
  date.yyDay = tmp->tm_mday;
  date.yyHour = tmp->tm_hour;
  date.yyMinutes = tmp->tm_min;
  date.yySeconds = tmp->tm_sec;
  tm.tm_isdst = tmp->tm_isdst;
  date.yyMeridian = MER24;
  date.yyRelSeconds = 0;
  date.yyRelMinutes = 0;
  date.yyRelHour = 0;
  date.yyRelDay = 0;
  date.yyRelMonth = 0;
  date.yyRelYear = 0;
  date.yyHaveDate = 0;
  date.yyHaveDay = 0;
  date.yyHaveRel = 0;
  date.yyHaveTime = 0;
  date.yyHaveZone = 0;

  if (yyparse ((void *)&date)
      || date.yyHaveTime > 1 || date.yyHaveZone > 1 
	  || date.yyHaveDate > 1 || date.yyHaveDay > 1)
    return -1;

  tm.tm_year = ToYear (date.yyYear) - TM_YEAR_ORIGIN + date.yyRelYear;
  tm.tm_mon = date.yyMonth - 1 + date.yyRelMonth;
  tm.tm_mday = date.yyDay + date.yyRelDay;
  if (date.yyHaveTime || (date.yyHaveRel && !date.yyHaveDate && !date.yyHaveDay))
    {
      tm.tm_hour = ToHour (date.yyHour, date.yyMeridian);
      if (tm.tm_hour < 0)
	return -1;
      tm.tm_min = date.yyMinutes;
      tm.tm_sec = date.yySeconds;
    }
  else
    {
      tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    }
  tm.tm_hour += date.yyRelHour;
  tm.tm_min += date.yyRelMinutes;
  tm.tm_sec += date.yyRelSeconds;

  /* Let mktime deduce tm_isdst if we have an absolute timestamp,
     or if the relative timestamp mentions days, months, or years.  */
  if (date.yyHaveDate | date.yyHaveDay | date.yyHaveTime | date.yyRelDay | date.yyRelMonth | date.yyRelYear)
    tm.tm_isdst = -1;

  tm0 = tm;

  Start = mktime (&tm);

  if (Start == (time_t) -1)
    {

      /* Guard against falsely reporting errors near the time_t boundaries
         when parsing times in other time zones.  For example, if the min
         time_t value is 1970-01-01 00:00:00 UTC and we are 8 hours ahead
         of UTC, then the min localtime value is 1970-01-01 08:00:00; if
         we apply mktime to 1970-01-01 00:00:00 we will get an error, so
         we apply mktime to 1970-01-02 08:00:00 instead and adjust the time
         zone by 24 hours to compensate.  This algorithm assumes that
         there is no DST transition within a day of the time_t boundaries.  */
      if (date.yyHaveZone)
	{
	  tm = tm0;
	  if (tm.tm_year <= EPOCH - TM_YEAR_ORIGIN)
	    {
	      tm.tm_mday++;
	      date.yyTimezone -= 24 * 60;
	    }
	  else
	    {
	      tm.tm_mday--;
	      date.yyTimezone += 24 * 60;
	    }
	  Start = mktime (&tm);
	}

      if (Start == (time_t) -1)
	return Start;
    }

  if (date.yyHaveDay && !date.yyHaveDate)
    {
      tm.tm_mday += ((date.yyDayNumber - tm.tm_wday + 7) % 7
		     + 7 * (date.yyDayOrdinal - (0 < date.yyDayOrdinal)));
      Start = mktime (&tm);
      if (Start == (time_t) -1)
	return Start;
    }

  if (date.yyHaveZone)
    {
      long delta;
      struct tm *gmt = gmtime (&Start);
      if (!gmt)
	return -1;
      delta = date.yyTimezone * 60L + difftm (&tm, gmt);
      if ((Start + delta < Start) != (delta < 0))
	return -1;		/* time_t overflow */
      Start += delta;
    }

  return Start;
}
