#pragma once

#include <istream>
#include <list>
#include <map>

#include "Token.h"

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
	LexicalAnalyzer();

	std::list<Token> process(std::istream& input);
	void resetInternalState();
	void resetPositionState();
	void resetState();

	static std::string tokenTypeToString(TokenType type);
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
		std::pair<std::string, TokenType>("prediction", TokenType::KW_PREDICTION)
		});
};
