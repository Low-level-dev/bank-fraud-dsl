%option noyywrap
%option yylineno
%option noinput
%option nounput

%{
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

digit       [0-9]
letter      [A-Za-z_]
id          {letter}({letter}|{digit}|_)*
real        {digit}+(\.{digit}+)?
string      \"[^\"]*\"

%%

"rule"          { return RULE;     }
"if"            { return IF;       }
"then"          { return THEN;     }
"flag"          { return FLAG;     }
"block"         { return BLOCK;    }
"allow"         { return ALLOW;    }
"as"            { return AS;       }
"and"           { return AND;      }
"or"            { return OR;       }
"not"           { return NOT;      }
"within"        { return WITHIN;   }
"min"           { return MIN;      }
"exceeds"       { return VELOCITY; }

"SUSPICIOUS"    { yylval.str = strdup(yytext); return ACTION_TYPE; }
"FRAUD"         { yylval.str = strdup(yytext); return ACTION_TYPE; }
"REVIEW"        { yylval.str = strdup(yytext); return ACTION_TYPE; }
"SAFE"          { yylval.str = strdup(yytext); return ACTION_TYPE; }

">="            { return GTE; }
"<="            { return LTE; }
"=="            { return EQ;  }
"!="            { return NEQ; }
">"             { return GT;  }
"<"             { return LT;  }

"{"             { return LBRACE;  }
"}"             { return RBRACE;  }
"("             { return LPAREN;  }
")"             { return RPAREN;  }
";"             { return SEMI;    }

{real}          { yylval.num = atof(yytext); return NUMBER; }

{string}        {
                    yytext[yyleng - 1] = '\0';
                    yylval.str = strdup(yytext + 1);
                    return STRING;
                }

{id}            { yylval.str = strdup(yytext); return ID; }

[ \t\r]+        { /* skip whitespace */ }
\n              { /* yylineno auto-increments */ }
"//".*          { /* skip single-line comments */ }

.               {
                    fprintf(stderr,
                        "Lexer Error (line %d): unknown character '%s'\n",
                        yylineno, yytext);
                }

%%
