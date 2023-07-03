#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	TOK_NEWLINE,

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

	TOK_STAR,
	TOK_STAR_EQ,

	TOK_SLASH,
	TOK_SLASH_EQ,

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

	token_kind_symbols[TOK_STAR] = symbolInternNul("*");
	token_kind_symbols[TOK_SLASH] = symbolInternNul("/");

	token_kind_symbols[TOK_DASH_GT] = symbolInternNul("->");
}

static const char *tokenKindStr(TokenKind kind) {
	switch (kind) {
	case TOK_EOF: return "EOF";
	case TOK_ERROR: return "ERROR";

	case TOK_WHITESPACE: return "WHITESPACE";
	case TOK_NEWLINE: return "NEWLINE";

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

	case TOK_STAR: return "STAR";
	case TOK_STAR_EQ: return "STAR_EQ";

	case TOK_SLASH: return "SLASH";
	case TOK_SLASH_EQ: return "SLASH_EQ";

	case TOK_DASH_GT: return "DASH_GT";

	default: return "(sentinel)";
	}

	return "(unknown token)";
}

typedef struct SourceLocation SourceLocation;
struct SourceLocation {
	const char *path;

	size_t byte_index;

	uint32_t row;
	uint32_t column;
};

typedef struct {
	// Filled in by caller
	SourceLocation start;

	// Caller must fill this in after `lexOne` returns
	SourceLocation end;

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

static bool isNumber(char c) {
	return (c >= '0' && c <= '9');
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

	} else if (source_len >= 2 && source[0] == '/' && source[1] == '/') {
		token->kind = TOK_WHITESPACE;
		size_t len = 2;

		while (len < source_len && source[len] != '\n') {
			len++;
		}

		token->len = len;

		return;

	} else if (source[0] == '\n') {
		token->kind = TOK_NEWLINE;
		token->len = 1;
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

	} else if (isNumber(source[0])) {
		token->kind = TOK_NUMBER;

		size_t len = 1;
		while (len < source_len && isNumber(source[len])) {
			len++;
		}

		token->len = len;
		token->symbol = symbolIntern(source, len);
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

typedef struct Parser Parser;
struct Parser {
	const char *source;
	size_t source_len;

	Token curtok;

	int temp; // for creating bytecode values
};

void parserAdvance(Parser *self);

void parserInit(Parser *self, const char *source, size_t source_len, const char *path) {
	self->source = source;
	self->source_len = source_len;

	self->curtok = (Token){ 0 };
	self->curtok.start.path = path;
	self->curtok.end.path = path;

	parserAdvance(self);
}

void parserAdvance(Parser *self) {
	self->curtok.start = self->curtok.end;

	size_t idx = self->curtok.start.byte_index;
	lexOne(self->source + idx, self->source_len - idx, &self->curtok);

	self->curtok.end.byte_index += self->curtok.len;

	if (self->curtok.kind == TOK_NEWLINE) {
		self->curtok.end.row++;
		self->curtok.end.column = 0;
	} else {
		self->curtok.end.column += self->curtok.len;
	}

	if (self->curtok.kind == TOK_WHITESPACE) return parserAdvance(self);

	printf("advance to %s:%d:%d: ", self->curtok.start.path, self->curtok.start.row + 1, self->curtok.start.column + 1);

	printf(
		"[%s]",
		tokenKindStr(self->curtok.kind)
	);

	// raw
	if (self->curtok.kind != TOK_NEWLINE)
		printf(
			" “%.*s”",
			(int) self->curtok.len,
			self->source + self->curtok.start.byte_index
		);

	if (self->curtok.symbol)
		printf(" “%.*s”", (int) self->curtok.symbol->len, self->curtok.symbol->data);

	printf("\n");
}

typedef struct Node Node;

struct Node {
	int num;
};

static const Node NODE_NULL = (Node) { .num = 0 };
bool nodeIsNull(Node self) {
	return self.num == 0;
}

Node parserTemp(Parser *self) {
	return (Node) {
		.num = ++self->temp,
	};
}

Node parseLiteral(Parser *p) {
	if (p->curtok.kind == TOK_NUMBER) {
		Node res = parserTemp(p);

		printf("%%%d = %.*s\n", res.num, (int) p->curtok.symbol->len, p->curtok.symbol->data);

		parserAdvance(p);

		return res;
	}

	return NODE_NULL;
}

typedef struct PrefixParselet PrefixParselet;
struct PrefixParselet {
	int rbp;
	Node (*callback)(Parser *p, PrefixParselet *self);

	const char *op;
};

typedef struct InfixParselet InfixParselet;
struct InfixParselet {
	int lbp, rbp;
	Node (*callback)(Parser *p, InfixParselet *self, Node lhs);

	const char *op;
};

PrefixParselet prefix_parselets[TOK_MAX_ENUM] = { 0 };
InfixParselet infix_parselets[TOK_MAX_ENUM] = { 0 };

Node parsePrefix(Parser *p) {
	TokenKind op = p->curtok.kind;
	PrefixParselet *parselet = &prefix_parselets[op];
	if (parselet->callback == NULL) return NODE_NULL;

	return parselet->callback(p, parselet);
}

Node parseExpr(Parser *p, int precedence) {
	Node lhs = parsePrefix(p);
	if (nodeIsNull(lhs)) return NODE_NULL;

	while (true) {
		TokenKind op = p->curtok.kind;

		InfixParselet *parselet = &infix_parselets[op];
		if (parselet->callback == NULL) break;

		if (parselet->lbp <= precedence) return lhs;

		lhs = parselet->callback(p, parselet, lhs);
	}

	return lhs;
}

Node parseGenericLiteral(Parser *p, PrefixParselet *self) {
	Node res = parserTemp(p);

	printf("%%%d = %s “%.*s”\n", res.num, self->op, (int) p->curtok.symbol->len, p->curtok.symbol->data);
	parserAdvance(p);

	return res;
}

Node parseGenericBinary(Parser *p, InfixParselet *self, Node lhs) {
	parserAdvance(p);

	Node rhs = parseExpr(p, self->rbp);
	if (nodeIsNull(rhs)) {
		printf("%s:%d:%d: expected expression\n", p->curtok.start.path, p->curtok.start.row + 1, p->curtok.start.column + 1);
		return lhs;
	}

	Node res = parserTemp(p);
	printf("%%%d = %%%d %s %%%d\n", res.num, lhs.num, self->op, rhs.num);
	return res;
}

Node parseCall(Parser *p, InfixParselet *self, Node lhs) {
	parserAdvance(p);

	Node args[32] = { 0 };
	int args_count = 0;

	while (true) {
		Node rhs = parseExpr(p, 0);
		if (nodeIsNull(rhs)) break;

		assert(args_count < 32 && "error: more than 32 arguments!\n");
		args[args_count++] = rhs;

		if (p->curtok.kind == TOK_COMMA) {
			parserAdvance(p);
			continue;
		}

		if (p->curtok.kind == TOK_PAREN_CLOSE) break;
	}

	if (p->curtok.kind == TOK_PAREN_CLOSE) {
		parserAdvance(p);
	} else {
		assert(false && "Expected `)` to close function call");
	}

	Node res = parserTemp(p);
	printf("%%%d = %%%d(", res.num, lhs.num);

	for (int i = 0; i < args_count; i++) {
		printf("%%%d, ", args[i].num);
	}

	printf(")\n");

	return res;
}

static void initParselets(void) {
	prefix_parselets[TOK_NAME] = (PrefixParselet) {
		.callback = parseGenericLiteral,
		.op = "name",
	};
	prefix_parselets[TOK_NUMBER] = (PrefixParselet) {
		.callback = parseGenericLiteral,
		.op = "num",
	};

	infix_parselets[TOK_PAREN_OPEN] = (InfixParselet) {
		.lbp = 15,
		.callback = parseCall,
	};
	infix_parselets[TOK_PLUS] = (InfixParselet) {
		.lbp = 5,
		.rbp = 5,

		.callback = parseGenericBinary,

		.op = "+",
	};

	infix_parselets[TOK_STAR] = (InfixParselet) {
		.lbp = 10,
		.rbp = 10,

		.callback = parseGenericBinary,

		.op = "*",
	};
}

 int main(int argc, char **argv) {
	initTokenKindSymbols();
	initParselets();

	char data[16 * 1024 + 1];
	FILE *fp = fopen("example.ds", "rb");

	ssize_t amt = fread(data, 1, 16 * 1024, fp);
	if (amt <= 0) {
		perror("fread");
		abort();
	}

	data[amt] = '\0';

	Parser p;
	parserInit(&p, data, amt, "./example.ds");

	parseExpr(&p, 0);
}
