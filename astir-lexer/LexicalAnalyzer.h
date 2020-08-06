#pragma once

#include <istream>
#include <list>
#include <map>

#include "Token.h"
#include "Exception.h"

enum class LexicalAnalyzerState {
	Default,
	Identifier,
	Number,
	String,
	LeftArrow
};

class LexicalAnalyzerException : public Exception {
public:
	LexicalAnalyzerException(const std::string& errmsg)
		: Exception(errmsg) {}
};

class LexicalAnalyzer {
public:
	LexicalAnalyzer();

	std::list<Token> process(std::istream& input);
	void resetInternalState();
	void resetPositionState();
	void resetState();
private:
	unsigned int m_currentColumn;
	unsigned int m_currentLine;
	LexicalAnalyzerState m_state;
	Token m_currentToken;

	char m_currentCharacter;
	bool m_consumeNew;
	bool m_endOfStreamReached;

	const std::map<std::string, TokenType> m_keywordMap = std::map<std::string, TokenType>({
		std::pair<std::string, TokenType>("token", TokenType::KW_TOKEN),
		std::pair<std::string, TokenType>("regex", TokenType::KW_REGEX),
		std::pair<std::string, TokenType>("production", TokenType::KW_PRODUCTION),
		std::pair<std::string, TokenType>("rule", TokenType::KW_RULE),
		std::pair<std::string, TokenType>("category", TokenType::KW_CATEGORY),
		std::pair<std::string, TokenType>("deterministic", TokenType::KW_DETERMINISTIC),
		std::pair<std::string, TokenType>("non_deterministic", TokenType::KW_NONDETERMINISTIC),
		std::pair<std::string, TokenType>("finite", TokenType::KW_FINITE),
		std::pair<std::string, TokenType>("automaton", TokenType::KW_AUTOMATON),
		std::pair<std::string, TokenType>("parser", TokenType::KW_PARSER),
		std::pair<std::string, TokenType>("recursive_descent", TokenType::KW_RECURSIVE_DESCENT),
		std::pair<std::string, TokenType>("with", TokenType::KW_WITH),
		std::pair<std::string, TokenType>("follows", TokenType::KW_FOLLOWS),
		std::pair<std::string, TokenType>("extends", TokenType::KW_EXTENDS),
		std::pair<std::string, TokenType>("individual_string_literals", TokenType::KW_INDIVIDUAL_STRING_LITERALS),
		std::pair<std::string, TokenType>("grouped_string_literals", TokenType::KW_GROUPED_STRING_LITERALS),
		std::pair<std::string, TokenType>("table_lookup", TokenType::KW_TABLE_LOOKUP),
		std::pair<std::string, TokenType>("machine_lookup", TokenType::KW_MACHINE_LOOKUP),
		std::pair<std::string, TokenType>("backtracking", TokenType::KW_BACKTRACKING),
		std::pair<std::string, TokenType>("prediction", TokenType::KW_PREDICTION),
		std::pair<std::string, TokenType>("set", TokenType::KW_SET),
		std::pair<std::string, TokenType>("unset", TokenType::KW_UNSET),
		std::pair<std::string, TokenType>("flag", TokenType::KW_FLAG),
		std::pair<std::string, TokenType>("unflag", TokenType::KW_UNFLAG),
		std::pair<std::string, TokenType>("append", TokenType::KW_APPEND),
		std::pair<std::string, TokenType>("prepend", TokenType::KW_PREPEND),
		std::pair<std::string, TokenType>("clear", TokenType::KW_CLEAR),
		std::pair<std::string, TokenType>("left_trim", TokenType::KW_LEFT_TRIM),
		std::pair<std::string, TokenType>("right_trim", TokenType::KW_RIGHT_TRIM)
		});
};
