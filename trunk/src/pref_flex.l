%{

  #include <stdlib.h>
  #include <string.h>
  #include "pref_bison.h"
  extern int line_num;
%}
%option noyywrap

%x VALUE

KEY			[a-zA-Z0-9-]+
%%
=			{BEGIN(VALUE); return(TOKEN_EQ);}
\[                      { return(TOKEN_LEFT_BRACKET);}  
\]                      { return(TOKEN_RIGHT_BRACKET);}  

{KEY}			{
			  yylval.str=strdup(yytext);
			  return(TOKEN_KEY);
			}
#.*			{
			  yylval.str=strdup(yytext);
			  return(TOKEN_COMMENT);
			}
  
[[:blank:]]		/* eat up whitespace */

<VALUE>[^\n]*		{
			  yylval.str=strdup(yytext);
			  return(TOKEN_VALUE);
			}


<INITIAL,VALUE>\n	{
			  line_num++;
                          BEGIN(INITIAL);
			}
  
.			printf( "Unrecognized character: %s\n", yytext );
%%
