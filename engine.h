#ifndef ENGINE_H
#define ENGINE_H

/* ═══════════════════════════════════════════════════
   engine.h  –  Data structures & API for the
                Bank Fraud Detection Rule Engine
   ═══════════════════════════════════════════════════ */

/* ── Transaction fields (from CSV) ── */
typedef struct {
    char   txn_id[32];
    char   account[32];
    double amount;
    char   country[8];
    int    hour;          /* 0-23 */
    int    frequency;     /* transactions in last 10 min */
    char   txn_type[16];  /* TRANSFER / WITHDRAWAL / DEPOSIT */
} Transaction;

/* ── Condition types ── */
typedef enum {
    COND_NUM,    /* field > 1000        */
    COND_STR,    /* country != "BD"     */
    COND_AND,    /* cond AND cond       */
    COND_OR,     /* cond OR  cond       */
    COND_NOT,    /* NOT cond            */
    COND_WITHIN  /* frequency > 5 within 10 min */
} CondType;

typedef struct Condition {
    CondType type;
    /* for COND_NUM / COND_STR / COND_WITHIN */
    char  field[32];
    char  op[4];
    double num_val;
    char  str_val[64];
    double within_minutes;
    /* for COND_AND / COND_OR / COND_NOT */
    struct Condition *left;
    struct Condition *right;
} Condition;

/* ── Action types ── */
typedef enum { ACT_FLAG, ACT_BLOCK, ACT_ALLOW } ActType;

typedef struct {
    ActType type;
    char    label[32];   /* SUSPICIOUS / FRAUD / REVIEW / SAFE */
} Action;

/* ── Rule ── */
typedef struct Rule {
    char       name[128];
    Condition *condition;
    Action    *action;
    int        hit_count;   /* how many transactions triggered this rule */
    struct Rule *next;
} Rule;

/* ── Rule list ── */
typedef struct {
    Rule *head;
    Rule *tail;
    int   count;
} RuleList;

/* ════════════════════════════════
   Constructor helpers (used by parser)
   ════════════════════════════════ */
Condition *make_cond_num   (const char *field, const char *op, double val);
Condition *make_cond_str   (const char *field, const char *op, const char *val);
Condition *make_cond_logic (CondType type, Condition *left, Condition *right);
Condition *make_cond_not   (Condition *child);
Condition *make_cond_within(const char *field, double threshold, double minutes);

Action    *make_action (ActType type, const char *label);
Rule      *make_rule   (const char *name, Condition *cond, Action *act);

void       add_rule    (RuleList *list, Rule *rule);

/* ════════════════════════════════
   Engine API
   ════════════════════════════════ */
void run_engine(RuleList *rules, const char *csv_path);

/* ════════════════════════════════
   Cleanup
   ════════════════════════════════ */
void free_condition(Condition *c);
void free_rule_list(RuleList *list);

#endif /* ENGINE_H */
