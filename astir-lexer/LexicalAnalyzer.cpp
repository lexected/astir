#include "LexicalAnalyzer.h"

#include <string>
#include <cctype>

LexicalAnalyzer::LexicalAnalyzer()
	: m_currentLine(1), m_currentColumn(0), m_state(LexicalAnalyzerState::Default), m_currentToken(),
	m_currentCharacter('\0'), m_endOfStreamReached(true), m_consumeNew(false) {
}

std::list<Token> LexicalAnalyzer::process(std::istream& input) {
	std::list<Token> ret;

	m_consumeNew = true;
	m_endOfStreamReached = false;
	while (!m_endOfStreamReached) {
		if (m_consumeNew) {
			if (!input.get(m_currentCharacter)) {
				m_endOfStreamReached = true;
				if (m_state == LexicalAnalyzerState::Default) {
					break;
				}
			} else {
				if (m_currentCharacter == '\n') {
					++m_currentLine;
					m_currentColumn = 0;
				} else {
					++m_currentColumn;
				}
			}
		} else {
			m_consumeNew = true;
		}

		switch (m_state) {
			case LexicalAnalyzerState::Default:
				if (std::isalpha(m_currentCharacter)) {
					m_state = LexicalAnalyzerState::Identifier;
					m_currentToken.string.append(std::string({ m_currentCharacter }));
					m_currentToken.type = TokenType::IDENTIFIER;
					m_currentToken.column = m_currentColumn;
					m_currentToken.line = m_currentLine;
				} else if(std::isalnum(m_currentCharacter)) {
					m_state = LexicalAnalyzerState::Number;
					m_currentToken.string.append(std::string({ m_currentCharacter }));
					m_currentToken.type = TokenType::NUMBER;
					m_currentToken.column = m_currentColumn;
					m_currentToken.line = m_currentLine;
				} else if (isspace(m_currentCharacter)) {
					/* do nothin' */
				} else if (m_currentCharacter == '"') {
					m_state = LexicalAnalyzerState::String;
					m_currentToken.type = TokenType::STRING;
					m_currentToken.column = m_currentColumn;
					m_currentToken.line = m_currentLine;
				} else if (m_currentCharacter == '<') {
					m_state = LexicalAnalyzerState::LeftArrow;
					m_currentToken.string = "<-";
					m_currentToken.type = TokenType::OP_LEFTARR;
					m_currentToken.column = m_currentColumn;
					m_currentToken.line = m_currentLine;
				} else {
					m_currentToken.string = std::string({ m_currentCharacter });
					m_currentToken.column = m_currentColumn;
					m_currentToken.line = m_currentLine;
					switch (m_currentCharacter) {
						case '(':
							m_currentToken.type = TokenType::PAR_LEFT;
							break;
						case ')':
							m_currentToken.type = TokenType::PAR_RIGHT;
							break;
						case '[':
							m_currentToken.type = TokenType::SQUARE_LEFT;
							break;
						case ']':
							m_currentToken.type = TokenType::SQUARE_RIGHT;
							break;
						case '{':
							m_currentToken.type = TokenType::CURLY_LEFT;
							break;
						case '}':
							m_currentToken.type = TokenType::CURLY_RIGHT;
							break;
						case '=':
							m_currentToken.type = TokenType::OP_EQUALS;
							break;
						case ':':
							m_currentToken.type = TokenType::OP_COLON;
							break;
						case ';':
							m_currentToken.type = TokenType::OP_SEMICOLON;
							break;
						case '.':
							m_currentToken.type = TokenType::OP_DOT;
							break;
						case '^':
							m_currentToken.type = TokenType::OP_CARET;
							break;
						case '$':
							m_currentToken.type = TokenType::OP_DOLLAR;
							break;
						case '*':
							m_currentToken.type = TokenType::OP_STAR;
							break;
						case '+':
							m_currentToken.type = TokenType::OP_PLUS;
							break;
						case '?':
							m_currentToken.type = TokenType::OP_QM;
							break;
						case '|':
							m_currentToken.type = TokenType::OP_OR;
							break;
						case '/':
							m_currentToken.type = TokenType::OP_FWDSLASH;
							break;
						case ',':
							m_currentToken.type = TokenType::OP_COMMA;
							break;
						case '&':
							m_currentToken.type = TokenType::OP_AMPERSAND;
							break;
						case '-':
							m_currentToken.type = TokenType::OP_DASH;
							break;
						default:
							throw LexicalAnalyzerException("Unrecognized character '" + m_currentToken.string + "' found on line " + std::to_string(m_currentLine) + ":" + std::to_string(m_currentColumn));
							break;
					}
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
				}
				break;
			case LexicalAnalyzerState::Identifier:
				if (std::isalnum(m_currentCharacter) && !m_endOfStreamReached) {
					m_currentToken.string.append(std::string({ m_currentCharacter }));
				} else {
					decltype(m_keywordMap)::const_iterator it;
					if ((it = m_keywordMap.find(m_currentToken.string)) != m_keywordMap.cend()) {
						m_currentToken.type = it->second;
					}
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
					m_consumeNew = false;
				}
				break;
			case LexicalAnalyzerState::Number:
				if (std::isdigit(m_currentCharacter) && !m_endOfStreamReached) {
					m_currentToken.string.append(std::string({ m_currentCharacter }));
				} else {
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
					m_consumeNew = false;
				}
				break;
			case LexicalAnalyzerState::String:
				if (m_currentCharacter != '"' && !m_endOfStreamReached) {
					m_currentToken.string.append(std::string({ m_currentCharacter }));
				} else {
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				}
				break;
			case LexicalAnalyzerState::LeftArrow:
				if (m_currentCharacter != '-' || m_endOfStreamReached) {
					throw LexicalAnalyzerException("Unrecognized character '" + m_currentToken.string + "' found on line " + std::to_string(m_currentLine) + ":" + std::to_string(m_currentColumn) + ", expected '-' to complete the left-arrow operator '<-'");
				} else {
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				}
				break;
		}
	}

	m_currentToken.string = "";
	m_currentToken.type = TokenType::EOS;
	ret.push_back(m_currentToken);

	return ret;
}

void LexicalAnalyzer::resetInternalState() {
	m_consumeNew = true;
	m_state = LexicalAnalyzerState::Default;
}

void LexicalAnalyzer::resetPositionState() {
	m_currentColumn = 0;
	m_currentLine = 1;
}

void LexicalAnalyzer::resetState() {
	resetInternalState();
	resetPositionState();
}