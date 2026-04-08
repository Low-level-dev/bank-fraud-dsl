# 🏦 Bank Fraud Detection DSL

A **Domain Specific Language (DSL)** compiler built with **Lex + Yacc (Flex + Bison)**
that lets non-programmers write fraud detection rules in plain English-like syntax,
then automatically evaluates those rules against real bank transaction data.

> **Compiler Design Project** — demonstrates Lexical Analysis, Syntax Analysis,
> Semantic Evaluation, and Symbol Management using Flex + Bison.

---

## 📋 Table of Contents
- [The Problem](#-the-problem)
- [What This Project Does](#-what-this-project-does)
- [Project Structure](#-project-structure)
- [Language Syntax](#-language-syntax)
- [Installation](#-installation)
- [How to Run](#-how-to-run)
- [Sample Output](#-sample-output)
- [How It Works](#-how-it-works)
- [Adding Your Own Rules](#-adding-your-own-rules)

---

## 🔥 The Problem

Banks lose **billions** to fraud every year. The core issue:

- Fraud analysts **know** the patterns — but can't write code
- Developers can code — but don't understand fraud deeply
- Every new rule requires a developer, takes days, costs money
- Fraud happens in **real time** — slow rules mean real losses

**This project solves that gap.**

---

## 💡 What This Project Does

A fraud analyst writes rules like this in a plain text file:

```
rule "Large Foreign Transfer" {
    if amount > 100000 and country != "BD"
    then flag as FRAUD;
}

rule "Rapid Fire Transactions" {
    if frequency exceeds 5 within 10 min
    then block;
}
```

The compiler:
1. **Lex** tokenises the rule file into meaningful tokens
2. **Yacc** parses the tokens and validates the grammar
3. **Engine** loads transactions from a CSV file
4. Each transaction is evaluated against every rule
5. Results are printed with colour-coded verdicts + a summary report

---

## 📁 Project Structure

```
bank_fraud_dsl/
│
├── lexer.l                  ← Flex lexer  (tokeniser)
├── parser.y                 ← Bison parser (grammar + rule builder)
├── engine.h                 ← Data structures & API declarations
├── engine.c                 ← Rule evaluation + CSV processing + report
├── main.c                   ← Entry point
├── Makefile                 ← Build system
│
└── samples/
    ├── rules.txt            ← Sample fraud detection rules (edit freely)
    ├── transactions.csv     ← Sample bank transactions
    └── bad_rules.txt        ← Intentionally broken rules (tests error handling)
```

---

## 📝 Language Syntax

### Rule Structure
```
rule "Rule Name" {
    if <condition> then <action>;
}
```

### Conditions

| Syntax | Example |
|--------|---------|
| `field op number` | `amount > 50000` |
| `field op "string"` | `country != "BD"` |
| `cond and cond` | `amount > 1000 and hour < 5` |
| `cond or cond` | `hour < 4 or hour > 22` |
| `not cond` | `not country == "BD"` |
| `(cond)` | `(amount > 500 and hour < 4)` |
| `field exceeds N within M min` | `frequency exceeds 5 within 10 min` |

### Fields (CSV columns)

| Field | Type | Description |
|-------|------|-------------|
| `amount` | number | Transaction amount |
| `country` | string | Destination country code |
| `hour` | number | Hour of day (0–23) |
| `frequency` | number | Transactions in last 10 min |
| `txn_type` | string | `TRANSFER` / `WITHDRAWAL` / `DEPOSIT` |
| `account` | string | Account ID |

### Operators
`>` `<` `>=` `<=` `==` `!=`

### Actions

| Action | Meaning |
|--------|---------|
| `flag as FRAUD` | Mark as confirmed fraud |
| `flag as SUSPICIOUS` | Mark for investigation |
| `flag as REVIEW` | Mark for human review |
| `block` | Block the transaction |
| `allow` | Explicitly allow (whitelist) |

### Comments
```
// This is a single-line comment
```

---

## ⚙️ Installation

### Prerequisites
- `gcc`
- `flex`
- `bison`

### Linux / macOS
```bash
# Ubuntu / Debian
sudo apt-get install gcc flex bison

# macOS (Homebrew)
brew install flex bison
```

### Clone the repo
```bash
git clone https://github.com/YOUR_USERNAME/bank_fraud_dsl.git
cd bank_fraud_dsl
```

---

## ▶️ How to Run

### Quick start (sample data)
```bash
make        # build
make run    # run with sample rules + transactions
```

### Custom run
```bash
make
./fraud_engine <rules_file> <transactions_csv>

# Example:
./fraud_engine samples/rules.txt samples/transactions.csv
```

### Test error detection
```bash
./fraud_engine samples/bad_rules.txt samples/transactions.csv
```

### Clean build files
```bash
make clean
```

---

## 🖥️ Sample Output

```
╔══════════════════════════════════════════════════════╗
║      Bank Fraud Detection DSL  — v1.0               ║
║      Compiler Design Project  (Lex + Yacc)          ║
╚══════════════════════════════════════════════════════╝

Loading rules from: samples/rules.txt
─────────────────────────────────────────────────
  [LOADED] Rule: "Large Foreign Transfer"
  [LOADED] Rule: "Rapid Fire Transactions"
  [LOADED] Rule: "Nighttime Large Withdrawal"
  ... (7 rules total)

✅ Parsed 7 rule(s) successfully.

═══════════════════════════════════════════════════════
         BANK FRAUD DETECTION ENGINE — RESULTS
═══════════════════════════════════════════════════════

  🚨 FRAUD         | TXN: T001 | ACC: ACC1001 | $120000.00 | US | hr:02 | Rule: "Large Foreign Transfer"
  🔒 BLOCKED       | TXN: T002 | ACC: ACC1002 | $  3000.00 | BD | hr:14 | Rule: "Rapid Fire Transactions"
  ⚠  SUSPICIOUS    | TXN: T003 | ACC: ACC1003 | $ 75000.00 | BD | hr:03 | Rule: "Nighttime Large Withdrawal"
  ✅ CLEAN         | TXN: T005 | ACC: ACC1005 | $   500.00 | BD | hr:11 |
  🔍 REVIEW        | TXN: T009 | ACC: ACC1009 | $ 30000.00 | CN | hr:16 | Rule: "Foreign Transaction Review"

───────────────────────────────────────────────────────
  SUMMARY REPORT
───────────────────────────────────────────────────────
  Total Transactions : 20
  Flagged            : 12
  Blocked            : 2
  Clean (no rule)    : 6

  RULE HIT STATISTICS
───────────────────────────────────────────────────────
  Large Foreign Transfer          : 3 hit(s)
  Rapid Fire Transactions         : 2 hit(s)
  Late Night Small Transfers      : 4 hit(s)
═══════════════════════════════════════════════════════
```

---

## ⚙️ How It Works (Compiler Pipeline)

```
rules.txt
    │
    ▼
┌─────────────────────────────────┐
│  LEXER  (lexer.l / Flex)        │  Tokenises keywords, operators,
│                                 │  strings, numbers, identifiers
└──────────────┬──────────────────┘
               │  token stream
               ▼
┌─────────────────────────────────┐
│  PARSER  (parser.y / Bison)     │  Validates grammar, builds
│                                 │  Condition trees + Rule objects
└──────────────┬──────────────────┘
               │  Rule list in memory
               ▼
┌─────────────────────────────────┐
│  ENGINE  (engine.c)             │  Reads transactions.csv row by row,
│                                 │  evaluates each against rule tree,
│                                 │  prints verdict + summary report
└─────────────────────────────────┘
```

### Compiler Concepts Demonstrated

| Concept | Where |
|---------|-------|
| Lexical Analysis | `lexer.l` — regex patterns, token types |
| Syntax Analysis | `parser.y` — LALR(1) grammar, precedence rules |
| Semantic Evaluation | `engine.c` — recursive condition tree evaluator |
| Symbol / Data Management | `engine.h` — Condition, Action, Rule, RuleList structs |
| Error Reporting | Line numbers in both lexer and parser errors |
| Memory Management | `free_rule_list()`, `free_condition()` |

---

## ✏️ Adding Your Own Rules

Open `samples/rules.txt` and add a new rule block:

```
rule "My Custom Rule" {
    if amount > 200000 and txn_type == "TRANSFER"
    then flag as SUSPICIOUS;
}
```

Save and run — no recompilation needed. Rules are loaded at runtime.

---

## 👨‍💻 Author

Built as a Compiler Design course project.  
Demonstrates practical application of Lex + Yacc beyond the classic calculator example.

---

## 📄 License

MIT License — free to use, modify, and distribute.
