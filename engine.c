/* ═══════════════════════════════════════════════════════════════
   engine.c  –  Rule construction, condition evaluation,
                CSV transaction processing, and report output.
   ═══════════════════════════════════════════════════════════════ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "engine.h"

/* ─────────────────────────────────────────────
   ANSI colour codes (gracefully ignored if the
   terminal does not support them)
   ───────────────────────────────────────────── */
#define COL_RED     "\033[1;31m"
#define COL_YEL     "\033[1;33m"
#define COL_GRN     "\033[1;32m"
#define COL_CYN     "\033[1;36m"
#define COL_MAG     "\033[1;35m"
#define COL_WHT     "\033[1;37m"
#define COL_RST     "\033[0m"

/* ════════════════════════════════════════════════
   Constructor helpers
   ════════════════════════════════════════════════ */

Condition *make_cond_num(const char *field, const char *op, double val) {
    Condition *c = calloc(1, sizeof(Condition));
    c->type = COND_NUM;
    strncpy(c->field, field, sizeof(c->field)-1);
    strncpy(c->op,    op,    sizeof(c->op)-1);
    c->num_val = val;
    return c;
}

Condition *make_cond_str(const char *field, const char *op, const char *val) {
    Condition *c = calloc(1, sizeof(Condition));
    c->type = COND_STR;
    strncpy(c->field,   field, sizeof(c->field)-1);
    strncpy(c->op,      op,    sizeof(c->op)-1);
    strncpy(c->str_val, val,   sizeof(c->str_val)-1);
    return c;
}

Condition *make_cond_logic(CondType type, Condition *left, Condition *right) {
    Condition *c = calloc(1, sizeof(Condition));
    c->type  = type;
    c->left  = left;
    c->right = right;
    return c;
}

Condition *make_cond_not(Condition *child) {
    Condition *c = calloc(1, sizeof(Condition));
    c->type  = COND_NOT;
    c->left  = child;
    return c;
}

Condition *make_cond_within(const char *field, double threshold, double minutes) {
    Condition *c = calloc(1, sizeof(Condition));
    c->type           = COND_WITHIN;
    strncpy(c->field, field, sizeof(c->field)-1);
    strncpy(c->op,    ">",   sizeof(c->op)-1);
    c->num_val        = threshold;
    c->within_minutes = minutes;
    return c;
}

Action *make_action(ActType type, const char *label) {
    Action *a = calloc(1, sizeof(Action));
    a->type = type;
    if (label) strncpy(a->label, label, sizeof(a->label)-1);
    return a;
}

Rule *make_rule(const char *name, Condition *cond, Action *act) {
    Rule *r = calloc(1, sizeof(Rule));
    strncpy(r->name, name, sizeof(r->name)-1);
    r->condition = cond;
    r->action    = act;
    r->hit_count = 0;
    r->next      = NULL;
    return r;
}

void add_rule(RuleList *list, Rule *rule) {
    if (!list->head) { list->head = list->tail = rule; }
    else             { list->tail->next = rule; list->tail = rule; }
    list->count++;
}

/* ════════════════════════════════════════════════
   Get numeric field value from a transaction
   ════════════════════════════════════════════════ */
static double get_num_field(const Transaction *t, const char *field) {
    if (strcmp(field, "amount")    == 0) return t->amount;
    if (strcmp(field, "hour")      == 0) return (double)t->hour;
    if (strcmp(field, "frequency") == 0) return (double)t->frequency;
    fprintf(stderr, "  [WARN] Unknown numeric field: '%s'\n", field);
    return 0.0;
}

/* Get string field value from a transaction */
static const char *get_str_field(const Transaction *t, const char *field) {
    if (strcmp(field, "country")  == 0) return t->country;
    if (strcmp(field, "txn_type") == 0) return t->txn_type;
    if (strcmp(field, "account")  == 0) return t->account;
    if (strcmp(field, "txn_id")   == 0) return t->txn_id;
    fprintf(stderr, "  [WARN] Unknown string field: '%s'\n", field);
    return "";
}

/* ════════════════════════════════════════════════
   Apply a comparison operator on numbers
   ════════════════════════════════════════════════ */
static int cmp_num(double a, const char *op, double b) {
    if (strcmp(op, ">")  == 0) return a >  b;
    if (strcmp(op, "<")  == 0) return a <  b;
    if (strcmp(op, ">=") == 0) return a >= b;
    if (strcmp(op, "<=") == 0) return a <= b;
    if (strcmp(op, "==") == 0) return a == b;
    if (strcmp(op, "!=") == 0) return a != b;
    return 0;
}

/* Apply a comparison operator on strings */
static int cmp_str(const char *a, const char *op, const char *b) {
    int eq = (strcmp(a, b) == 0);
    if (strcmp(op, "==") == 0) return  eq;
    if (strcmp(op, "!=") == 0) return !eq;
    return 0;
}

/* ════════════════════════════════════════════════
   Recursive condition evaluator
   ════════════════════════════════════════════════ */
static int eval_condition(const Condition *c, const Transaction *t) {
    if (!c) return 0;
    switch (c->type) {
        case COND_NUM:
            return cmp_num(get_num_field(t, c->field), c->op, c->num_val);

        case COND_STR:
            return cmp_str(get_str_field(t, c->field), c->op, c->str_val);

        case COND_WITHIN:
            /* "frequency > N within M min"
               We model this as: the frequency field exceeds the threshold.
               In a production system you would query a time-window DB here. */
            return get_num_field(t, c->field) > c->num_val;

        case COND_AND:
            return eval_condition(c->left, t) && eval_condition(c->right, t);

        case COND_OR:
            return eval_condition(c->left, t) || eval_condition(c->right, t);

        case COND_NOT:
            return !eval_condition(c->left, t);
    }
    return 0;
}

/* ════════════════════════════════════════════════
   Trim leading/trailing whitespace in-place
   ════════════════════════════════════════════════ */
static void trim(char *s) {
    if (!s) return;
    /* leading */
    int i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i) memmove(s, s+i, strlen(s)-i+1);
    /* trailing */
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

/* ════════════════════════════════════════════════
   Parse one CSV line into a Transaction struct.
   Expected columns (with header):
   txn_id, account, amount, country, hour, frequency, txn_type
   ════════════════════════════════════════════════ */
static int parse_csv_line(const char *line, Transaction *t) {
    char buf[512];
    strncpy(buf, line, sizeof(buf)-1);

    char *tok;
    int   col = 0;
    tok = strtok(buf, ",");
    while (tok && col < 7) {
        trim(tok);
        switch (col) {
            case 0: strncpy(t->txn_id,   tok, sizeof(t->txn_id)-1);   break;
            case 1: strncpy(t->account,  tok, sizeof(t->account)-1);  break;
            case 2: t->amount    = atof(tok);                          break;
            case 3: strncpy(t->country,  tok, sizeof(t->country)-1);  break;
            case 4: t->hour      = atoi(tok);                          break;
            case 5: t->frequency = atoi(tok);                          break;
            case 6: strncpy(t->txn_type, tok, sizeof(t->txn_type)-1); break;
        }
        col++;
        tok = strtok(NULL, ",");
    }
    return (col == 7);   /* 1 = success, 0 = bad line */
}

/* ════════════════════════════════════════════════
   Pretty-print action label
   ════════════════════════════════════════════════ */
static void print_verdict(const Transaction *t, const Rule *r) {
    const char *col = COL_WHT;
    const char *tag = "";

    switch (r->action->type) {
        case ACT_FLAG:
            if      (strcmp(r->action->label, "FRAUD")      == 0) { col = COL_RED; tag = "🚨 FRAUD";      }
            else if (strcmp(r->action->label, "SUSPICIOUS")  == 0) { col = COL_YEL; tag = "⚠  SUSPICIOUS"; }
            else if (strcmp(r->action->label, "REVIEW")      == 0) { col = COL_MAG; tag = "🔍 REVIEW";     }
            else                                                    { col = COL_CYN; tag = r->action->label; }
            break;
        case ACT_BLOCK: col = COL_RED; tag = "🔒 BLOCKED";  break;
        case ACT_ALLOW: col = COL_GRN; tag = "✅ ALLOWED";  break;
    }

    printf("  %s%-14s%s | TXN: %-8s | ACC: %-10s | $%10.2f | %-5s | hr:%02d | freq:%d | Rule: \"%s\"\n",
           col, tag, COL_RST,
           t->txn_id, t->account, t->amount,
           t->country, t->hour, t->frequency,
           r->name);
}

/* ════════════════════════════════════════════════
   Main engine entry point
   ════════════════════════════════════════════════ */
void run_engine(RuleList *rules, const char *csv_path) {

    FILE *fp = fopen(csv_path, "r");
    if (!fp) {
        fprintf(stderr, "\nEngine Error: Cannot open CSV file '%s'\n", csv_path);
        return;
    }

    printf("\n%s═══════════════════════════════════════════════════════%s\n", COL_CYN, COL_RST);
    printf("%s         BANK FRAUD DETECTION ENGINE — RESULTS          %s\n", COL_WHT, COL_RST);
    printf("%s═══════════════════════════════════════════════════════%s\n\n", COL_CYN, COL_RST);

    /* Statistics */
    int total = 0, flagged = 0, blocked = 0, allowed = 0, clean = 0;

    char line[512];
    int  lineno = 0;

    while (fgets(line, sizeof(line), fp)) {
        lineno++;
        /* strip newline */
        line[strcspn(line, "\r\n")] = '\0';

        /* skip header row and blank lines */
        if (lineno == 1 || strlen(line) == 0) continue;

        Transaction t;
        memset(&t, 0, sizeof(t));
        if (!parse_csv_line(line, &t)) {
            fprintf(stderr, "  [WARN] Skipping malformed CSV line %d\n", lineno);
            continue;
        }
        total++;

        /* evaluate every rule against this transaction */
        int hit = 0;
        for (Rule *r = rules->head; r; r = r->next) {
            if (eval_condition(r->condition, &t)) {
                print_verdict(&t, r);
                r->hit_count++;
                hit = 1;
                if (r->action->type == ACT_BLOCK) blocked++;
                else if (r->action->type == ACT_ALLOW) allowed++;
                else flagged++;
                break;   /* first matching rule wins */
            }
        }
        if (!hit) {
            printf("  %s✅ CLEAN        %s | TXN: %-8s | ACC: %-10s | $%10.2f | %-5s | hr:%02d | freq:%d\n",
                   COL_GRN, COL_RST,
                   t.txn_id, t.account, t.amount,
                   t.country, t.hour, t.frequency);
            clean++;
        }
    }
    fclose(fp);

    /* ── Summary report ── */
    printf("\n%s───────────────────────────────────────────────────────%s\n", COL_CYN, COL_RST);
    printf("%s  SUMMARY REPORT%s\n", COL_WHT, COL_RST);
    printf("%s───────────────────────────────────────────────────────%s\n", COL_CYN, COL_RST);
    printf("  Total Transactions : %d\n", total);
    printf("  %sFlagged            : %d%s\n", COL_YEL, flagged, COL_RST);
    printf("  %sBlocked            : %d%s\n", COL_RED, blocked, COL_RST);
    printf("  %sAllowed (explicit) : %d%s\n", COL_GRN, allowed, COL_RST);
    printf("  %sClean (no rule)    : %d%s\n", COL_GRN, clean,   COL_RST);

    printf("\n%s  RULE HIT STATISTICS%s\n", COL_WHT, COL_RST);
    printf("%s───────────────────────────────────────────────────────%s\n", COL_CYN, COL_RST);
    for (Rule *r = rules->head; r; r = r->next) {
        printf("  %-40s : %d hit(s)\n", r->name, r->hit_count);
    }
    printf("%s═══════════════════════════════════════════════════════%s\n\n", COL_CYN, COL_RST);
}

/* ════════════════════════════════════════════════
   Memory cleanup
   ════════════════════════════════════════════════ */
void free_condition(Condition *c) {
    if (!c) return;
    free_condition(c->left);
    free_condition(c->right);
    free(c);
}

void free_rule_list(RuleList *list) {
    Rule *r = list->head;
    while (r) {
        Rule *next = r->next;
        free_condition(r->condition);
        free(r->action);
        free(r);
        r = next;
    }
    list->head = list->tail = NULL;
    list->count = 0;
}
