#ifndef config_HEADER_H
#define config_HEADER_H 1
#define config_IN_HEADER 1

#line 5 "libmemcached/csl/scanner.h"

#include <libmemcached/csl/common.h>
#include <libmemcached/csl/context.h>
#include <libmemcached/csl/parser.h>
#include <libmemcached/csl/symbol.h>

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define YY_NO_INPUT

#define YY_EXTRA_TYPE Context*

#line 22 "libmemcached/csl/scanner.h"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

/* %not-for-header */

#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 6
#define YY_FLEX_SUBMINOR_VERSION 4
#if YY_FLEX_SUBMINOR_VERSION > 0
#define FLEX_BETA
#endif

/* %if-c++-only */
/* %endif */

/* %if-c-only */
#ifdef yy_create_buffer
#define config__create_buffer_ALREADY_DEFINED
#else
#define yy_create_buffer config__create_buffer
#endif

#ifdef yy_delete_buffer
#define config__delete_buffer_ALREADY_DEFINED
#else
#define yy_delete_buffer config__delete_buffer
#endif

#ifdef yy_scan_buffer
#define config__scan_buffer_ALREADY_DEFINED
#else
#define yy_scan_buffer config__scan_buffer
#endif

#ifdef yy_scan_string
#define config__scan_string_ALREADY_DEFINED
#else
#define yy_scan_string config__scan_string
#endif

#ifdef yy_scan_bytes
#define config__scan_bytes_ALREADY_DEFINED
#else
#define yy_scan_bytes config__scan_bytes
#endif

#ifdef yy_init_buffer
#define config__init_buffer_ALREADY_DEFINED
#else
#define yy_init_buffer config__init_buffer
#endif

#ifdef yy_flush_buffer
#define config__flush_buffer_ALREADY_DEFINED
#else
#define yy_flush_buffer config__flush_buffer
#endif

#ifdef yy_load_buffer_state
#define config__load_buffer_state_ALREADY_DEFINED
#else
#define yy_load_buffer_state config__load_buffer_state
#endif

#ifdef yy_switch_to_buffer
#define config__switch_to_buffer_ALREADY_DEFINED
#else
#define yy_switch_to_buffer config__switch_to_buffer
#endif

#ifdef yypush_buffer_state
#define config_push_buffer_state_ALREADY_DEFINED
#else
#define yypush_buffer_state config_push_buffer_state
#endif

#ifdef yypop_buffer_state
#define config_pop_buffer_state_ALREADY_DEFINED
#else
#define yypop_buffer_state config_pop_buffer_state
#endif

#ifdef yyensure_buffer_stack
#define config_ensure_buffer_stack_ALREADY_DEFINED
#else
#define yyensure_buffer_stack config_ensure_buffer_stack
#endif

#ifdef yylex
#define config_lex_ALREADY_DEFINED
#else
#define yylex config_lex
#endif

#ifdef yyrestart
#define config_restart_ALREADY_DEFINED
#else
#define yyrestart config_restart
#endif

#ifdef yylex_init
#define config_lex_init_ALREADY_DEFINED
#else
#define yylex_init config_lex_init
#endif

#ifdef yylex_init_extra
#define config_lex_init_extra_ALREADY_DEFINED
#else
#define yylex_init_extra config_lex_init_extra
#endif

#ifdef yylex_destroy
#define config_lex_destroy_ALREADY_DEFINED
#else
#define yylex_destroy config_lex_destroy
#endif

#ifdef yyget_debug
#define config_get_debug_ALREADY_DEFINED
#else
#define yyget_debug config_get_debug
#endif

#ifdef yyset_debug
#define config_set_debug_ALREADY_DEFINED
#else
#define yyset_debug config_set_debug
#endif

#ifdef yyget_extra
#define config_get_extra_ALREADY_DEFINED
#else
#define yyget_extra config_get_extra
#endif

#ifdef yyset_extra
#define config_set_extra_ALREADY_DEFINED
#else
#define yyset_extra config_set_extra
#endif

#ifdef yyget_in
#define config_get_in_ALREADY_DEFINED
#else
#define yyget_in config_get_in
#endif

#ifdef yyset_in
#define config_set_in_ALREADY_DEFINED
#else
#define yyset_in config_set_in
#endif

#ifdef yyget_out
#define config_get_out_ALREADY_DEFINED
#else
#define yyget_out config_get_out
#endif

#ifdef yyset_out
#define config_set_out_ALREADY_DEFINED
#else
#define yyset_out config_set_out
#endif

#ifdef yyget_leng
#define config_get_leng_ALREADY_DEFINED
#else
#define yyget_leng config_get_leng
#endif

#ifdef yyget_text
#define config_get_text_ALREADY_DEFINED
#else
#define yyget_text config_get_text
#endif

#ifdef yyget_lineno
#define config_get_lineno_ALREADY_DEFINED
#else
#define yyget_lineno config_get_lineno
#endif

#ifdef yyset_lineno
#define config_set_lineno_ALREADY_DEFINED
#else
#define yyset_lineno config_set_lineno
#endif

#ifdef yyget_column
#define config_get_column_ALREADY_DEFINED
#else
#define yyget_column config_get_column
#endif

#ifdef yyset_column
#define config_set_column_ALREADY_DEFINED
#else
#define yyset_column config_set_column
#endif

#ifdef yywrap
#define config_wrap_ALREADY_DEFINED
#else
#define yywrap config_wrap
#endif

/* %endif */

#ifdef yyget_lval
#define config_get_lval_ALREADY_DEFINED
#else
#define yyget_lval config_get_lval
#endif

#ifdef yyset_lval
#define config_set_lval_ALREADY_DEFINED
#else
#define yyset_lval config_set_lval
#endif

#ifdef yyalloc
#define config_alloc_ALREADY_DEFINED
#else
#define yyalloc config_alloc
#endif

#ifdef yyrealloc
#define config_realloc_ALREADY_DEFINED
#else
#define yyrealloc config_realloc
#endif

#ifdef yyfree
#define config_free_ALREADY_DEFINED
#else
#define yyfree config_free
#endif

/* %if-c-only */

/* %endif */

/* First, we deal with  platform-specific or compiler-specific issues. */

/* begin standard C headers. */
/* %if-c-only */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
/* %endif */

/* %if-tables-serialization */
/* %endif */
/* end standard C headers. */

/* %if-c-or-c++ */
/* flex integer type definitions */

#ifndef FLEXINT_H
#define FLEXINT_H

/* C99 systems have <inttypes.h>. Non-C99 systems may or may not. */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* C99 says to define __STDC_LIMIT_MACROS before including stdint.h,
 * if you want the limit (max/min) macros for int types. 
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include <inttypes.h>
typedef int8_t flex_int8_t;
typedef uint8_t flex_uint8_t;
typedef int16_t flex_int16_t;
typedef uint16_t flex_uint16_t;
typedef int32_t flex_int32_t;
typedef uint32_t flex_uint32_t;
#else
typedef signed char flex_int8_t;
typedef short int flex_int16_t;
typedef int flex_int32_t;
typedef unsigned char flex_uint8_t; 
typedef unsigned short int flex_uint16_t;
typedef unsigned int flex_uint32_t;

/* Limits of integral types. */
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#ifndef SIZE_MAX
#define SIZE_MAX               (~(size_t)0)
#endif

#endif /* ! C99 */

#endif /* ! FLEXINT_H */

/* %endif */

/* begin standard C++ headers. */
/* %if-c++-only */
/* %endif */

/* TODO: this is always defined, so inline it */
#define yyconst const

#if defined(__GNUC__) && __GNUC__ >= 3
#define yynoreturn __attribute__((__noreturn__))
#else
#define yynoreturn
#endif

/* %not-for-header */

/* %not-for-header */

/* %if-reentrant */

/* An opaque pointer. */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

/* For convenience, these vars (plus the bison vars far below)
   are macros in the reentrant scanner. */
#define yyin yyg->yyin_r
#define yyout yyg->yyout_r
#define yyextra yyg->yyextra_r
#define yyleng yyg->yyleng_r
#define yytext yyg->yytext_r
#define yylineno (YY_CURRENT_BUFFER_LVALUE->yy_bs_lineno)
#define yycolumn (YY_CURRENT_BUFFER_LVALUE->yy_bs_column)
#define yy_flex_debug yyg->yy_flex_debug_r

/* %endif */

/* %if-not-reentrant */
/* %endif */

/* Size of default input buffer. */
#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

/* %if-not-reentrant */
/* %endif */

/* %if-c-only */
/* %if-not-reentrant */
/* %endif */
/* %endif */

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
/* %if-c-only */
	FILE *yy_input_file;
/* %endif */

/* %if-c++-only */
/* %endif */

	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB
	 * characters.
	 */
	int yy_buf_size;

	/* Number of characters read into yy_ch_buf, not including EOB
	 * characters.
	 */
	int yy_n_chars;

	/* Whether we "own" the buffer - i.e., we know we created it,
	 * and can realloc() it to grow it, and should free() it to
	 * delete it.
	 */
	int yy_is_our_buffer;

	/* Whether this is an "interactive" input source; if so, and
	 * if we're using stdio for input, then we want to use getc()
	 * instead of fread(), to make sure we stop fetching input after
	 * each newline.
	 */
	int yy_is_interactive;

	/* Whether we're considered to be at the beginning of a line.
	 * If so, '^' rules will be active on the next match, otherwise
	 * not.
	 */
	int yy_at_bol;

    int yy_bs_lineno; /**< The line count. */
    int yy_bs_column; /**< The column count. */

	/* Whether to try to fill the input buffer when we reach the
	 * end of it.
	 */
	int yy_fill_buffer;

	int yy_buffer_status;

	};
#endif /* !YY_STRUCT_YY_BUFFER_STATE */

/* %if-c-only Standard (non-C++) definition */
/* %not-for-header */

/* %endif */

/* %if-c-only Standard (non-C++) definition */

/* %if-not-reentrant */
/* %not-for-header */

/* %endif */

void yyrestart ( FILE *input_file , yyscan_t yyscanner );
void yy_switch_to_buffer ( YY_BUFFER_STATE new_buffer , yyscan_t yyscanner );
YY_BUFFER_STATE yy_create_buffer ( FILE *file, int size , yyscan_t yyscanner );
void yy_delete_buffer ( YY_BUFFER_STATE b , yyscan_t yyscanner );
void yy_flush_buffer ( YY_BUFFER_STATE b , yyscan_t yyscanner );
void yypush_buffer_state ( YY_BUFFER_STATE new_buffer , yyscan_t yyscanner );
void yypop_buffer_state ( yyscan_t yyscanner );

YY_BUFFER_STATE yy_scan_buffer ( char *base, yy_size_t size , yyscan_t yyscanner );
YY_BUFFER_STATE yy_scan_string ( const char *yy_str , yyscan_t yyscanner );
YY_BUFFER_STATE yy_scan_bytes ( const char *bytes, int len , yyscan_t yyscanner );

/* %endif */

void *yyalloc ( yy_size_t , yyscan_t yyscanner );
void *yyrealloc ( void *, yy_size_t , yyscan_t yyscanner );
void yyfree ( void * , yyscan_t yyscanner );

/* %% [1.0] yytext/yyin/yyout/yy_state_type/yylineno etc. def's & init go here */
/* Begin user sect3 */

#define config_wrap(yyscanner) (/*CONSTCOND*/1)
#define YY_SKIP_YYWRAP

#define FLEX_DEBUG

#define yytext_ptr yytext_r

/* %if-c-only Standard (non-C++) definition */

/* %endif */

#ifdef YY_HEADER_EXPORT_START_CONDITIONS
#define INITIAL 0

#endif

#ifndef YY_NO_UNISTD_H
/* Special case for "unistd.h", since it is non-ANSI. We include it way
 * down here because we want the user's section 1 to have been scanned first.
 * The user has a chance to override it with an option.
 */
/* %if-c-only */
#include <unistd.h>
/* %endif */
/* %if-c++-only */
/* %endif */
#endif

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

/* %if-c-only Reentrant structure and macros (non-C++). */
/* %if-reentrant */

/* %if-c-only */

/* %endif */

/* %if-reentrant */

int yylex_init (yyscan_t* scanner);

int yylex_init_extra ( YY_EXTRA_TYPE user_defined, yyscan_t* scanner);

/* %endif */

/* %endif End reentrant structures and macros. */

/* Accessor methods to globals.
   These are made visible to non-reentrant scanners for convenience. */

int yylex_destroy ( yyscan_t yyscanner );

int yyget_debug ( yyscan_t yyscanner );

void yyset_debug ( int debug_flag , yyscan_t yyscanner );

YY_EXTRA_TYPE yyget_extra ( yyscan_t yyscanner );

void yyset_extra ( YY_EXTRA_TYPE user_defined , yyscan_t yyscanner );

FILE *yyget_in ( yyscan_t yyscanner );

void yyset_in  ( FILE * _in_str , yyscan_t yyscanner );

FILE *yyget_out ( yyscan_t yyscanner );

void yyset_out  ( FILE * _out_str , yyscan_t yyscanner );

			int yyget_leng ( yyscan_t yyscanner );

char *yyget_text ( yyscan_t yyscanner );

int yyget_lineno ( yyscan_t yyscanner );

void yyset_lineno ( int _line_number , yyscan_t yyscanner );

int yyget_column  ( yyscan_t yyscanner );

void yyset_column ( int _column_no , yyscan_t yyscanner );

/* %if-bison-bridge */

YYSTYPE * yyget_lval ( yyscan_t yyscanner );

void yyset_lval ( YYSTYPE * yylval_param , yyscan_t yyscanner );

/* %endif */

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int yywrap ( yyscan_t yyscanner );
#else
extern int yywrap ( yyscan_t yyscanner );
#endif
#endif

/* %not-for-header */

/* %endif */

#ifndef yytext_ptr
static void yy_flex_strncpy ( char *, const char *, int , yyscan_t yyscanner);
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen ( const char * , yyscan_t yyscanner);
#endif

#ifndef YY_NO_INPUT
/* %if-c-only Standard (non-C++) definition */
/* %not-for-header */

/* %endif */
#endif

/* %if-c-only */

/* %endif */

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k */
#define YY_READ_BUF_SIZE 16384
#else
#define YY_READ_BUF_SIZE 8192
#endif /* __ia64__ */
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* %if-tables-serialization structures and prototypes */
/* %not-for-header */

/* %not-for-header */

/* Default declaration of generated scanner - a define so the user can
 * easily add parameters.
 */
#ifndef YY_DECL
#define YY_DECL_IS_OURS 1
/* %if-c-only Standard (non-C++) definition */

extern int yylex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner);

#define YY_DECL int yylex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner)
/* %endif */
/* %if-c++-only C++ definition */
/* %endif */
#endif /* !YY_DECL */

/* %not-for-header */

/* %if-c++-only */
/* %not-for-header */

/* %endif */

/* yy_get_previous_state - get the state just before the EOB char was reached */

/* %if-c-only */
/* %not-for-header */

#undef YY_NEW_FILE
#undef YY_FLUSH_BUFFER
#undef yy_set_bol
#undef yy_new_buffer
#undef yy_set_interactive
#undef YY_DO_BEFORE_ACTION

#ifdef YY_DECL_IS_OURS
#undef YY_DECL_IS_OURS
#undef YY_DECL
#endif

#ifndef config__create_buffer_ALREADY_DEFINED
#undef yy_create_buffer
#endif
#ifndef config__delete_buffer_ALREADY_DEFINED
#undef yy_delete_buffer
#endif
#ifndef config__scan_buffer_ALREADY_DEFINED
#undef yy_scan_buffer
#endif
#ifndef config__scan_string_ALREADY_DEFINED
#undef yy_scan_string
#endif
#ifndef config__scan_bytes_ALREADY_DEFINED
#undef yy_scan_bytes
#endif
#ifndef config__init_buffer_ALREADY_DEFINED
#undef yy_init_buffer
#endif
#ifndef config__flush_buffer_ALREADY_DEFINED
#undef yy_flush_buffer
#endif
#ifndef config__load_buffer_state_ALREADY_DEFINED
#undef yy_load_buffer_state
#endif
#ifndef config__switch_to_buffer_ALREADY_DEFINED
#undef yy_switch_to_buffer
#endif
#ifndef config_push_buffer_state_ALREADY_DEFINED
#undef yypush_buffer_state
#endif
#ifndef config_pop_buffer_state_ALREADY_DEFINED
#undef yypop_buffer_state
#endif
#ifndef config_ensure_buffer_stack_ALREADY_DEFINED
#undef yyensure_buffer_stack
#endif
#ifndef config_lex_ALREADY_DEFINED
#undef yylex
#endif
#ifndef config_restart_ALREADY_DEFINED
#undef yyrestart
#endif
#ifndef config_lex_init_ALREADY_DEFINED
#undef yylex_init
#endif
#ifndef config_lex_init_extra_ALREADY_DEFINED
#undef yylex_init_extra
#endif
#ifndef config_lex_destroy_ALREADY_DEFINED
#undef yylex_destroy
#endif
#ifndef config_get_debug_ALREADY_DEFINED
#undef yyget_debug
#endif
#ifndef config_set_debug_ALREADY_DEFINED
#undef yyset_debug
#endif
#ifndef config_get_extra_ALREADY_DEFINED
#undef yyget_extra
#endif
#ifndef config_set_extra_ALREADY_DEFINED
#undef yyset_extra
#endif
#ifndef config_get_in_ALREADY_DEFINED
#undef yyget_in
#endif
#ifndef config_set_in_ALREADY_DEFINED
#undef yyset_in
#endif
#ifndef config_get_out_ALREADY_DEFINED
#undef yyget_out
#endif
#ifndef config_set_out_ALREADY_DEFINED
#undef yyset_out
#endif
#ifndef config_get_leng_ALREADY_DEFINED
#undef yyget_leng
#endif
#ifndef config_get_text_ALREADY_DEFINED
#undef yyget_text
#endif
#ifndef config_get_lineno_ALREADY_DEFINED
#undef yyget_lineno
#endif
#ifndef config_set_lineno_ALREADY_DEFINED
#undef yyset_lineno
#endif
#ifndef config_get_column_ALREADY_DEFINED
#undef yyget_column
#endif
#ifndef config_set_column_ALREADY_DEFINED
#undef yyset_column
#endif
#ifndef config_wrap_ALREADY_DEFINED
#undef yywrap
#endif
#ifndef config_get_lval_ALREADY_DEFINED
#undef yyget_lval
#endif
#ifndef config_set_lval_ALREADY_DEFINED
#undef yyset_lval
#endif
#ifndef config_get_lloc_ALREADY_DEFINED
#undef yyget_lloc
#endif
#ifndef config_set_lloc_ALREADY_DEFINED
#undef yyset_lloc
#endif
#ifndef config_alloc_ALREADY_DEFINED
#undef yyalloc
#endif
#ifndef config_realloc_ALREADY_DEFINED
#undef yyrealloc
#endif
#ifndef config_free_ALREADY_DEFINED
#undef yyfree
#endif
#ifndef config_text_ALREADY_DEFINED
#undef yytext
#endif
#ifndef config_leng_ALREADY_DEFINED
#undef yyleng
#endif
#ifndef config_in_ALREADY_DEFINED
#undef yyin
#endif
#ifndef config_out_ALREADY_DEFINED
#undef yyout
#endif
#ifndef config__flex_debug_ALREADY_DEFINED
#undef yy_flex_debug
#endif
#ifndef config_lineno_ALREADY_DEFINED
#undef yylineno
#endif
#ifndef config_tables_fload_ALREADY_DEFINED
#undef yytables_fload
#endif
#ifndef config_tables_destroy_ALREADY_DEFINED
#undef yytables_destroy
#endif
#ifndef config_TABLES_NAME_ALREADY_DEFINED
#undef yyTABLES_NAME
#endif

#line 203 "libmemcached/csl/scanner.l"


#line 850 "libmemcached/csl/scanner.h"
#undef config_IN_HEADER
#endif /* config_HEADER_H */
