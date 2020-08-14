#pragma once

#include <string>

#include "Exception.h"
#include "IFileLocalizable.h"

enum class TokenType {
	KW_USES,

	KW_FOLLOWS,
	KW_WITH,
	
	KW_DETERMINISTIC,
	KW_NONDETERMINISTIC,
	KW_FINITE,
	KW_AUTOMATON,
	KW_INDIVIDUAL_STRING_LITERALS,
	KW_GROUPED_STRING_LITERALS,

	KW_CATEGORY,
	KW_TERMINAL,
	KW_NONTERMINAL,
	KW_PATTERN,
	KW_PRODUCTION,

	KW_ITEM,
	KW_LIST,
	KW_RAW,

	KW_SET,
	KW_UNSET,
	KW_FLAG,
	KW_UNFLAG,
	KW_APPEND,
	KW_PREPEND,
	KW_CLEAR,
	KW_LEFT_TRIM,
	KW_RIGHT_TRIM,

	IDENTIFIER,
	STRING,
	NUMBER,

	PAR_LEFT,
	PAR_RIGHT,
	SQUARE_LEFT,
	SQUARE_RIGHT,
	CURLY_LEFT,
	CURLY_RIGHT,

	OP_COLON,
	OP_EQUALS,
	OP_LEFTARR,
	OP_SEMICOLON,
	OP_COMMA,
	OP_DOT,
	OP_CARET,
	OP_DOLLAR,

	OP_STAR,
	OP_PLUS,
	OP_QM,
	OP_OR,
	OP_FWDSLASH,
	
	OP_AMPERSAND,
	OP_DASH,
	OP_AT,

	EOS /* end of stream wiseapples */
};

struct Token : public IFileLocalizable {
	TokenType type;
	std::string string;

	Token()
		: m_fileLocation(1, 1), type(TokenType::IDENTIFIER), string() {}

	void setLocation(const FileLocation& loc);
	std::string typeString() const;
	std::string toString() const;
	std::string toHumanString() const;

	const FileLocation& location() const override;

	static std::string convertTypeToString(TokenType type);
private:
	FileLocation m_fileLocation;
};