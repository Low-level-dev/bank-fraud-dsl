%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"

int  yylex(void);
void yyerror(const char *s);
extern int yylineno;

RuleList g_rulelist;
%}

%code requires {
#include "engine.h"
}

%union {
    double     num;
    char      *str;
    Condition *cond;
    Action    *act;
    Rule      *rule;
}

%token RULE IF THEN FLAG BLOCK ALLOW AS AND OR NOT WITHIN MIN
%token LBRACE RBRACE LPAREN RPAREN SEMI VELOCITY
%token GT LT GTE LTE EQ NEQ
%token <num> NUMBER
%token <str> STRING ID ACTION_TYPE

%type <rule>  rule
%type <cond>  condition condition_expr condition_term condition_atom
%type <act>   action
%type <str>   compare_op

%left  OR
%left  AND
%right UNOT

%%

program
    : rule_list
    ;

rule_list
    : rule_list rule   { add_rule(&g_rulelist, $2); }
    | rule             { add_rule(&g_rulelist, $1); }
    ;

rule
    : RULE STRING LBRACE IF condition THEN action SEMI RBRACE
      {
          $$ = make_rule($2, $5, $7);
          free($2);
          printf("  [LOADED] Rule: \"%s\"\n", $$->name);
      }
    ;

condition
    : condition_expr   { $$ = $1; }
    ;

condition_expr
    : condition_expr OR condition_term
        { $$ = make_cond_logic(COND_OR,  $1, $3); }
    | condition_term
        { $$ = $1; }
    ;

condition_term
    : condition_term AND condition_atom
        { $$ = make_cond_logic(COND_AND, $1, $3); }
    | condition_atom
        { $$ = $1; }
    ;

condition_atom
    : NOT condition_atom  %prec UNOT
        { $$ = make_cond_not($2); }

    /* velocity check: "frequency exceeds N within M min"
       Uses dedicated VELOCITY keyword to avoid shift/reduce conflict */
    | ID VELOCITY NUMBER WITHIN NUMBER MIN
        { $$ = make_cond_within($1, $3, $5); free($1); }

    | ID compare_op NUMBER
        { $$ = make_cond_num($1, $2, $3); free($1); free($2); }

    | ID compare_op STRING
        { $$ = make_cond_str($1, $2, $3); free($1); free($2); free($3); }

    | ID compare_op ID
        { $$ = make_cond_str($1, $2, $3); free($1); free($2); free($3); }

    | LPAREN condition_expr RPAREN
        { $$ = $2; }
    ;

compare_op
    : GT   { $$ = strdup(">");  }
    | LT   { $$ = strdup("<");  }
    | GTE  { $$ = strdup(">="); }
    | LTE  { $$ = strdup("<="); }
    | EQ   { $$ = strdup("=="); }
    | NEQ  { $$ = strdup("!="); }
    ;

action
    : FLAG AS ACTION_TYPE   { $$ = make_action(ACT_FLAG,  $3); free($3); }
    | BLOCK                 { $$ = make_action(ACT_BLOCK, NULL); }
    | ALLOW                 { $$ = make_action(ACT_ALLOW, NULL); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error (line %d): %s\n", yylineno, s);
}
