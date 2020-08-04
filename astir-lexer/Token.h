#pragma once

#include <string>

enum class TokenType {
	KW_TOKEN,
	KW_REGEX,
	KW_PRODUCTION,
	KW_RULE,
	KW_DETERMINISTIC,
	KW_NONDETERMINISTIC,
	KW_FINITE,
	KW_AUTOMATON,
	KW_PARSER,
	KW_RECURSIVE_DESCENT,
	KW_WITH,
	KW_FOLLOWS,
	KW_EXTENDS,
	KW_INDIVIDUAL_STRING_LITERALS,
	KW_GROUPED_STRING_LITERALS,
	KW_TABLE_LOOKUP,
	KW_MACHINE_LOOKUP,
	KW_BACKTRACKING,
	KW_PREDICTION,

	IDENTIFIER,
	STRING,
	NUMBER,

	PAR_LEFT,
	PAR_RIGHT,
	SQUARE_LEFT,
	SQUARE_RIGHT,
	CURLY_LEFT,
	CURLY_RIGHT,

	OP_EQUALS,
	OP_COLON,
	OP_LEFTARR,
	OP_SEMICOLON,
	OP_DOT,
	OP_CARET,
	OP_DOLLAR,

	OP_STAR,
	OP_PLUS,
	OP_QM,
	OP_OR,
	OP_FWDSLASH,
	OP_COMMA,
	OP_AMPERSAND,
	OP_DASH
};

struct Token {
	unsigned int line;
	unsigned int column;
	TokenType type;
	std::string string;

	Token() : line(1), column(1), type(TokenType::IDENTIFIER), string() {}
};