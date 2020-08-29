#pragma once

#include <istream>
#include <list>
#include <map>

#include "Token.h"
#include "Exception.h"

enum class LexicalAnalyzerState {
	Default,
	ForwardSlash,
	LineComment,
	MultilineComment,
	MultilineCommentStarEncountered,

	Identifier,
	Number,
	String,
	StringEscapeSequence,
	StringOctalEscapeSequence,
	StringHexEscapeSequence,
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
	FileLocation m_currentLocation;
	LexicalAnalyzerState m_state;
	bool m_stringIsDoubleQuote;
	std::string m_currentEscapeSequence;
	Token m_currentToken;

	char m_currentCharacter;
	bool m_consumeNew;
	bool m_endOfStreamReached;

	const std::map<std::string, TokenType> m_keywordMap = std::map<std::string, TokenType>({
		std::pair<std::string, TokenType>("uses", TokenType::KW_USES),

		std::pair<std::string, TokenType>("on", TokenType::KW_ON),
		std::pair<std::string, TokenType>("with", TokenType::KW_WITH),

		std::pair<std::string, TokenType>("deterministic", TokenType::KW_DETERMINISTIC),
		std::pair<std::string, TokenType>("nondeterministic", TokenType::KW_NONDETERMINISTIC),
		std::pair<std::string, TokenType>("finite", TokenType::KW_FINITE),
		std::pair<std::string, TokenType>("automaton", TokenType::KW_AUTOMATON),
		std::pair<std::string, TokenType>("productions_terminal_by_default", TokenType::KW_PRODUCTIONS_TERMINAL_BY_DEFAULT),
		std::pair<std::string, TokenType>("productions_nonterminal_by_default", TokenType::KW_PRODUCTIONS_NONTERMINAL_BY_DEFAULT),
		std::pair<std::string, TokenType>("rules_productions_by_default", TokenType::KW_RULES_PRODUCTIONS_BY_DEFAULT),
		std::pair<std::string, TokenType>("rules_patterns_by_default", TokenType::KW_RULES_PRODUCTIONS_BY_DEFAULT),
		std::pair<std::string, TokenType>("category", TokenType::KW_CATEGORY),
		std::pair<std::string, TokenType>("terminal", TokenType::KW_TERMINAL),
		std::pair<std::string, TokenType>("nonterminal", TokenType::KW_NONTERMINAL),
		std::pair<std::string, TokenType>("pattern", TokenType::KW_PATTERN),
		std::pair<std::string, TokenType>("production", TokenType::KW_PRODUCTION),
		std::pair<std::string, TokenType>("item", TokenType::KW_ITEM),
		std::pair<std::string, TokenType>("list", TokenType::KW_LIST),
		std::pair<std::string, TokenType>("raw", TokenType::KW_RAW),

		std::pair<std::string, TokenType>("flag", TokenType::KW_FLAG),
		std::pair<std::string, TokenType>("unflag", TokenType::KW_UNFLAG),

		std::pair<std::string, TokenType>("capture", TokenType::KW_CAPTURE),
		std::pair<std::string, TokenType>("empty", TokenType::KW_EMPTY),
		std::pair<std::string, TokenType>("append", TokenType::KW_APPEND),
		std::pair<std::string, TokenType>("prepend", TokenType::KW_PREPEND),

		std::pair<std::string, TokenType>("set", TokenType::KW_SET),
		std::pair<std::string, TokenType>("unset", TokenType::KW_UNSET),
		
		std::pair<std::string, TokenType>("push", TokenType::KW_PUSH),
		std::pair<std::string, TokenType>("pop", TokenType::KW_POP),
		std::pair<std::string, TokenType>("clear", TokenType::KW_CLEAR),
		});
};
