#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

typedef struct Symbol *Symbol;
struct Symbol {
	Symbol next;

	size_t len;
	char data[];
};

Symbol symbols = NULL;

Symbol symbolIntern(const char *data, size_t data_len) {
	Symbol *it = &symbols;

	while (*it != NULL) {
		if ((*it)->len == data_len &&
			!memcmp((*it)->data, data, data_len)) {

			return *it;
		}

		it = &(*it)->next;
	}

	*it = malloc(sizeof(**it) + data_len);

	(*it)->next = NULL;
	(*it)->len = data_len;
	memcpy((*it)->data, data, data_len);

	return *it;
}

Symbol symbolInternNul(const char *str) {
	return symbolIntern(str, strlen(str));
}

typedef enum {
	TOK_EOF,
	TOK_ERROR,

	TOK_WHITESPACE,

	TOK_NAME,
	TOK_NUMBER,

	TOK_PAREN_OPEN,
	TOK_PAREN_CLOSE,

	TOK_BRACE_OPEN,
	TOK_BRACE_CLOSE,

	TOK_BRACKET_OPEN,
	TOK_BRACKET_CLOSE,

	TOK_PERIOD,
	TOK_COMMA,

	TOK_COLON,
	TOK_SEMICOLON,

	TOK_EQ,
	TOK_EQ_EQ,

	TOK_PLUS,
	TOK_PLUS_EQ,

	TOK_DASH,
	TOK_DASH_EQ,

	TOK_DASH_GT,

	TOK_MAX_ENUM,
} TokenKind;

Symbol token_kind_symbols[TOK_MAX_ENUM] = { 0 };
static void initTokenKindSymbols(void) {
	token_kind_symbols[TOK_PAREN_OPEN] = symbolInternNul("(");
	token_kind_symbols[TOK_PAREN_CLOSE] = symbolInternNul(")");

	token_kind_symbols[TOK_BRACE_OPEN] = symbolInternNul("{");
	token_kind_symbols[TOK_BRACE_CLOSE] = symbolInternNul("}");

	token_kind_symbols[TOK_PERIOD] = symbolInternNul(".");
	token_kind_symbols[TOK_COMMA] = symbolInternNul(",");

	token_kind_symbols[TOK_COLON] = symbolInternNul(":");
	token_kind_symbols[TOK_SEMICOLON] = symbolInternNul(";");

	token_kind_symbols[TOK_EQ] = symbolInternNul("=");

	token_kind_symbols[TOK_PLUS] = symbolInternNul("+");
	token_kind_symbols[TOK_DASH] = symbolInternNul("-");

	token_kind_symbols[TOK_DASH_GT] = symbolInternNul("->");
}

static const char *tokenKindStr(TokenKind kind) {
	switch (kind) {
	case TOK_EOF: return "EOF";
	case TOK_ERROR: return "ERROR";

	case TOK_WHITESPACE: return "WHITESPACE";

	case TOK_NAME: return "NAME";
	case TOK_NUMBER: return "NUMBER";

	case TOK_PAREN_OPEN: return "PAREN_OPEN";
	case TOK_PAREN_CLOSE: return "PAREN_CLOSE";

	case TOK_BRACE_OPEN: return "BRACE_OPEN";
	case TOK_BRACE_CLOSE: return "BRACE_CLOSE";

	case TOK_BRACKET_OPEN: return "BRACKET_OPEN";
	case TOK_BRACKET_CLOSE: return "BRACKET_CLOSE";

	case TOK_PERIOD: return "PERIOD";
	case TOK_COMMA: return "COMMA";

	case TOK_COLON: return "COLON";
	case TOK_SEMICOLON: return "SEMICOLON";

	case TOK_EQ: return "EQ";
	case TOK_EQ_EQ: return "EQ_EQ";

	case TOK_PLUS: return "PLUS";
	case TOK_PLUS_EQ: return "PLUS_EQ";

	case TOK_DASH: return "DASH";
	case TOK_DASH_EQ: return "DASH_EQ";

	case TOK_DASH_GT: return "DASH_GT";
	}

	return "(unknown token)";
}

typedef struct {
	// Filled in by caller
	size_t start;

	// Filled in by `lexOne`
	TokenKind kind;
	size_t len;

	Symbol symbol;
} Token;

static bool isNameStart(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static bool isNameContinue(char c) {
	return isNameStart(c) || (c >= '0' && c <= '9');
}

void lexOne(const char *source, size_t source_len, Token *token) {
	token->kind = TOK_ERROR;
	token->len = 0;
	token->symbol = NULL;

	if (source[0] == '\0') {
		token->kind = TOK_EOF;
		token->len = 0;

		return;
	}

	if (source[0] == '\r' || source[0] == '\t' || source[0] == ' ') {
		token->kind = TOK_WHITESPACE;
		token->len = 1;
		return;

	} else if (source[0] == '\n') {
		token->kind = TOK_WHITESPACE;
		token->len = 1;
		return;

	} else if (source_len >= 2 && source[0] == '/' && source[1] == '/') {
		token->kind = TOK_WHITESPACE;
		size_t len = 2;

		while (len < source_len && source[len] != '\n') {
			len++;
		}

		token->len = len;

		return;

	} else if (isNameStart(source[0])) {
		token->kind = TOK_NAME;

		size_t len = 1;
		while (len < source_len && isNameContinue(source[len])) {
			len++;
		}

		token->len = len;

		Symbol that_symbol = symbolIntern(source, len);
		for (int i = 0; i < TOK_MAX_ENUM; i++) {
			if (token_kind_symbols[i] != that_symbol) continue;

			token->kind = i;
			break;
		}

		token->symbol = that_symbol;
		return;
	}

	assert(token->kind == TOK_ERROR);
	assert(token->len == 0);

	while ((token->len + 1) < source_len) {
		bool found = false;

		Symbol that_symbol = symbolIntern(source, token->len + 1);
		for (int i = 0; i < TOK_MAX_ENUM; i++) {
			if (token_kind_symbols[i] != that_symbol) continue;

			token->kind = i;
			token->len++;

			found = true;
			break;
		}

		if (!found) break;
	}

	if (token->len == 0) token->len++;
}

int main(int argc, char **argv) {
	initTokenKindSymbols();

	char data[16 * 1024 + 1];
	FILE *fp = fopen("example.ds", "rb");

	ssize_t amt = fread(data, 1, 16 * 1024, fp);
	if (amt <= 0) {
		perror("fread");
		abort();
	}

	data[amt] = '\0';

	Token token = {
		.start = 0,
	};
	while (true) {
		token.start += token.len;
		lexOne(data + token.start, amt - token.start, &token);

		if (token.kind == TOK_WHITESPACE) continue;

		printf("[%s] “%.*s”", tokenKindStr(token.kind), (int) token.len, data + token.start);
		if (token.symbol)
			printf(" “%.*s”", (int) token.symbol->len, token.symbol->data);

		printf("\n");

		if (token.len == 0) break;
	}
}
