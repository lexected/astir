#pragma once

#include <istream>
#include <list>

enum class TokenType {
	KW_TOKEN,
	KW_REGEX,
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

enum class LexicalAnalyzerState {
	Default,
	Identifier,
	Number,
	String,
	LeftArrow
};

class LexicalAnalyzerException : std::exception {
public:
	LexicalAnalyzerException(const std::string& errmsg)
		: std::exception(errmsg.c_str()) {}
};

class LexicalAnalyzer {
public:
	LexicalAnalyzer() = default;

	std::list<Token> process(std::istream& input);

	static std::string tokenTypeToString(TokenType type);
};

