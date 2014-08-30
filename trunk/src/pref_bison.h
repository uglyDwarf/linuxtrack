/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_LTR_INT_PARSER_PREF_BISON_HPP_INCLUDED
# define YY_LTR_INT_PARSER_PREF_BISON_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int ltr_int_parser_debug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_COMMENT = 258,
    TOKEN_LEFT_BRACKET = 259,
    TOKEN_RIGHT_BRACKET = 260,
    TOKEN_KEY = 261,
    TOKEN_EQ = 262,
    TOKEN_VALUE = 263,
    TOKEN_SECNAME = 264,
    LOW_PRIO = 265
  };
#endif
/* Tokens.  */
#define TOKEN_COMMENT 258
#define TOKEN_LEFT_BRACKET 259
#define TOKEN_RIGHT_BRACKET 260
#define TOKEN_KEY 261
#define TOKEN_EQ 262
#define TOKEN_VALUE 263
#define TOKEN_SECNAME 264
#define LOW_PRIO 265

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 16 "pref_bison.ypp" /* yacc.c:1909  */

  std::string *str;
  keyVal *kv;
  section *sec;
  prefs *prf;

#line 81 "pref_bison.hpp" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



int ltr_int_parser_parse (prefs *prf);

#endif /* !YY_LTR_INT_PARSER_PREF_BISON_HPP_INCLUDED  */
