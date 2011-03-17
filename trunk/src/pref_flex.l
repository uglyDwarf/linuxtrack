%{

  #include <stdlib.h>
  #include <string.h>
  #include "pref_bison.h"
  #include "utils.h"
  extern int line_num;
%}
%option noyywrap
%option nounput
%option noinput

%x VALUE SEC_NAME

KEY			[_a-zA-Z0-9-]+
%%
=[[:blank:]]*		{BEGIN(VALUE); return(TOKEN_EQ);}
\[                      {BEGIN(SEC_NAME); return(TOKEN_LEFT_BRACKET);}  
\]                      {return(TOKEN_RIGHT_BRACKET);}  

{KEY}			{
			  yylval.str=ltr_int_my_strdup(yytext);
			  return(TOKEN_KEY);
			}
#.*			{
			  yylval.str=ltr_int_my_strdup(yytext);
			  return(TOKEN_COMMENT);
			}
  
[[:blank:]]		/* eat up whitespace */

<VALUE>[^\n]*           {
                          yylval.str=ltr_int_my_strdup(yytext);
                          return(TOKEN_VALUE);
                        }


<INITIAL,VALUE>\n       {
                          line_num++;
                          BEGIN(INITIAL);
                        }
<SEC_NAME>[^\]]*        {
                          yylval.str=ltr_int_my_strdup(yytext);
                          BEGIN(INITIAL);
                          return(TOKEN_SECNAME);
                        }
  
.			ltr_int_log_message( "Unrecognized character: %s\n", yytext );
%%

