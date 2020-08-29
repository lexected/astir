#include "LexicalAnalyzer.h"

#include <string>
#include <cctype>

LexicalAnalyzer::LexicalAnalyzer()
	: m_currentLocation(1, 0), m_state(LexicalAnalyzerState::Default), m_currentToken(),
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
					++m_currentLocation.line;
					m_currentLocation.column = 0;
				} else {
					++m_currentLocation.column;
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
					m_currentToken.setLocation(m_currentLocation);
				} else if (std::isalnum(m_currentCharacter) || m_currentCharacter == '_') {
					m_state = LexicalAnalyzerState::Number;
					m_currentToken.string.append(std::string({ m_currentCharacter }));
					m_currentToken.type = TokenType::NUMBER;
					m_currentToken.setLocation(m_currentLocation);
				} else if (isspace(m_currentCharacter)) {
					/* do nothin' */
				} else if (m_currentCharacter == '"') {
					m_state = LexicalAnalyzerState::String;
					m_currentToken.type = TokenType::STRING;
					m_currentToken.setLocation(m_currentLocation);
				} else if (m_currentCharacter == '<') {
					m_state = LexicalAnalyzerState::LeftArrow;
					m_currentToken.string = "<-";
					m_currentToken.type = TokenType::OP_LEFTARR;
					m_currentToken.setLocation(m_currentLocation);
				} else if (m_currentCharacter == '/') {
					m_state = LexicalAnalyzerState::ForwardSlash;
					m_currentToken.string = "/";
					m_currentToken.setLocation(m_currentLocation);
				} else {
					m_currentToken.string = std::string({ m_currentCharacter });
					m_currentToken.setLocation(m_currentLocation);
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
						case ',':
							m_currentToken.type = TokenType::OP_COMMA;
							break;
						case '&':
							m_currentToken.type = TokenType::OP_AMPERSAND;
							break;
						case '-':
							m_currentToken.type = TokenType::OP_DASH;
							break;
						case '@':
							m_currentToken.type = TokenType::OP_AT;
							break;
						default:
							throw LexicalAnalyzerException("Unrecognized character '" + m_currentToken.string + "' found on line " + m_currentLocation.toString());
							break;
					}
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
				}
				break;
			case LexicalAnalyzerState::Identifier:
				if ((std::isalnum(m_currentCharacter) || m_currentCharacter == '_') && !m_endOfStreamReached) {
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
					throw LexicalAnalyzerException("Unrecognized character '" + m_currentToken.string + "' found on line " + m_currentLocation.toString() + ", expected '-' to complete the left-arrow operator '<-'");
				} else {
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				}
				break;
			case LexicalAnalyzerState::ForwardSlash:
				if (m_currentCharacter == '/') {
					m_state = LexicalAnalyzerState::LineComment;
				} else if (m_currentCharacter == '*') {
					m_state = LexicalAnalyzerState::MultilineComment;
				} else {
					m_currentToken.type = TokenType::OP_FWDSLASH;
					ret.push_back(m_currentToken);
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				}
				break;
			case LexicalAnalyzerState::LineComment:
				if (m_currentCharacter == '\n' || m_endOfStreamReached) {
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				}
				break;
			case LexicalAnalyzerState::MultilineComment:
				if (m_endOfStreamReached) {
					throw LexicalAnalyzerException("Multiline comment without closure started on " + m_currentToken.locationString() + ", expected '*/' to close the multiline comment before the input stream end");
				}

				if (m_currentCharacter == '*') {
					m_state = LexicalAnalyzerState::MultilineCommentStarEncountered;
				}
				break;
			case LexicalAnalyzerState::MultilineCommentStarEncountered:
				if (m_endOfStreamReached) {
					throw LexicalAnalyzerException("Multiline comment without closure started on " + m_currentToken.locationString() + ", expected '*/' (now only '/' is missing as star has recently been encountered) to close the multiline comment before the input stream end");
				}

				if (m_currentCharacter == '/') {
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				} else {
					m_state = LexicalAnalyzerState::MultilineComment;
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
	m_currentLocation.column = 0;
	m_currentLocation.line = 1;
}

void LexicalAnalyzer::resetState() {
	resetInternalState();
	resetPositionState();
}