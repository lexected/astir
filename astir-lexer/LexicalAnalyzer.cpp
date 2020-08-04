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

std::string LexicalAnalyzer::tokenTypeToString(TokenType type) {
	switch (type) {
		case TokenType::CURLY_LEFT:
			return "CURLY_LEFT";
		case TokenType::CURLY_RIGHT:
			return "CURLY_RIGHT";
		case TokenType::IDENTIFIER:
			return "IDENTIFIER";
		case TokenType::KW_REGEX:
			return "KW_REGEX";
		case TokenType::KW_TOKEN:
			return "KW_TOKEN";
		case TokenType::NUMBER:
			return "NUMBER";
		case TokenType::OP_AMPERSAND:
			return "OP_AMPERSAND";
		case TokenType::OP_CARET:
			return "OP_CARET";
		case TokenType::OP_COLON:
			return "OP_COLON";
		case TokenType::OP_COMMA:
			return "OP_COMMA";
		case TokenType::OP_DASH:
			return "OP_DASH";
		case TokenType::OP_DOLLAR:
			return "OP_DOLLAR";
		case TokenType::OP_DOT:
			return "OP_DOT";
		case TokenType::OP_EQUALS:
			return "OP_EQUALS";
		case TokenType::OP_FWDSLASH:
			return "OP_FWDSLASH";
		case TokenType::OP_LEFTARR:
			return "OP_LEFTARR";
		case TokenType::OP_OR:
			return "OP_OR";
		case TokenType::OP_PLUS:
			return "OP_PLUS";
		case TokenType::OP_QM:
			return "OP_QM";
		case TokenType::OP_SEMICOLON:
			return "OP_SEMICOLON";
		case TokenType::OP_STAR:
			return "OP_STAR";
		case TokenType::PAR_LEFT:
			return "PAR_LEFT";
		case TokenType::PAR_RIGHT:
			return "PAR_RIGHT";
		case TokenType::SQUARE_LEFT:
			return "SQUARE_LEFT";
		case TokenType::SQUARE_RIGHT:
			return "SQUARE_RIGHT";
		case TokenType::STRING:
			return "STRING";
		default:
			throw LexicalAnalyzerException("Unrecognized token type: " + std::to_string((unsigned int)type));
	}
}
