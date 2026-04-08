# ════════════════════════════════════════════════════════
#  Makefile — Bank Fraud Detection DSL
#  Requires: gcc, flex, bison
# ════════════════════════════════════════════════════════

CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = fraud_engine

# Generated source files
PARSER_C = parser.tab.c
PARSER_H = parser.tab.h
LEXER_C  = lex.yy.c

SRCS = $(PARSER_C) $(LEXER_C) engine.c main.c
OBJS = $(SRCS:.c=.o)

# ── Default target ──
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lfl

# ── Generate parser from Bison ──
$(PARSER_C) $(PARSER_H): parser.y
	bison -d -Wcounterexamples -o $(PARSER_C) parser.y

# ── Generate lexer from Flex ──
$(LEXER_C): lexer.l $(PARSER_H)
	flex -o $(LEXER_C) lexer.l

# ── Run with sample files ──
run: $(TARGET)
	./$(TARGET) samples/rules.txt samples/transactions.csv

# ── Clean build artifacts ──
clean:
	rm -f $(TARGET) $(PARSER_C) $(PARSER_H) $(LEXER_C) *.o

# ── Help ──
help:
	@echo ""
	@echo "  make          — build the project"
	@echo "  make run      — build and run with sample data"
	@echo "  make clean    — remove all build artifacts"
	@echo "  make help     — show this message"
	@echo ""
	@echo "  Manual run:"
	@echo "  ./fraud_engine <rules_file> <transactions_csv>"
	@echo ""

.PHONY: all run clean help
