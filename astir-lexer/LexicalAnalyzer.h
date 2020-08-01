#pragma once

#include <istream>
#include <list>

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
};

