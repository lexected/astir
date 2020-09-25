#include "LexicalAnalyzer.h"

#include <string>
#include <cctype>

LexicalAnalyzer::LexicalAnalyzer()
	: m_currentLocation(1, 0), m_state(LexicalAnalyzerState::Default), m_currentToken(),
	m_currentCharacter('\0'), m_endOfStreamReached(true), m_consumeNew(false), m_stringIsDoubleQuote(false) {
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
				if (std::isalpha(m_currentCharacter) || m_currentCharacter == '_') {
					m_state = LexicalAnalyzerState::Identifier;
					m_currentToken.string.append(std::string({ m_currentCharacter }));
					m_currentToken.type = TokenType::IDENTIFIER;
					m_currentToken.setLocation(m_currentLocation);
				} else if (std::isdigit(m_currentCharacter)) {
					m_state = LexicalAnalyzerState::Number;
					m_currentToken.string.append(std::string({ m_currentCharacter }));
					m_currentToken.type = TokenType::NUMBER;
					m_currentToken.setLocation(m_currentLocation);
				} else if (isspace(m_currentCharacter)) {
					/* do nothin' */
				} else if (m_currentCharacter == '"' || m_currentCharacter == '\'') {
					m_state = LexicalAnalyzerState::String;
					m_stringIsDoubleQuote = (m_currentCharacter == '"');
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
							throw LexicalAnalysisException("Unrecognized character '" + m_currentToken.string + "' found on line " + m_currentLocation.toString());
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
				if (m_currentCharacter == '\n') {
					throw LexicalAnalysisException("Unexpected end of line encountered at " + m_currentLocation.toString() + ", string started at " + m_currentToken.locationString() + " was left unterminated");
				} else if (m_currentCharacter == '\\') {
					m_state = LexicalAnalyzerState::StringEscapeSequence;
				} else if (m_currentCharacter == '"' && m_stringIsDoubleQuote || m_currentCharacter == '\'' && !m_stringIsDoubleQuote) {
					if (!m_currentToken.string.empty() || m_currentToken.string.empty() && m_currentLocation.column - 1 == m_currentToken.location().column) {
						ret.push_back(m_currentToken);
					}
					
					m_currentToken.string.clear();
					m_state = LexicalAnalyzerState::Default;
				} else if (!m_endOfStreamReached) {
					m_currentToken.string.append(1, m_currentCharacter);
					if(m_stringIsDoubleQuote) {
						ret.push_back(m_currentToken);
						m_currentToken.string.clear();
					}
				} else {
					throw LexicalAnalysisException("Unexpected end of stream encountered at " + m_currentLocation.toString() + ", string started at " + m_currentToken.locationString() + " was left unterminated");
				}
				break;
			case LexicalAnalyzerState::StringEscapeSequence: {
				char toAppend = '\0';
				if (m_currentCharacter == '\'') {
					toAppend = '\'';
				} else if (m_currentCharacter == '\"') {
					toAppend = '\"';
				} else if (m_currentCharacter == '?') {
					toAppend = '?';
				} else if (m_currentCharacter == '\\') {
					toAppend = '\\';
				} else if (m_currentCharacter == 'a') {
					toAppend = '\a';
				} else if (m_currentCharacter == 'b') {
					toAppend = '\b';
				} else if (m_currentCharacter == 'f') {
					toAppend = '\f';
				} else if (m_currentCharacter == 'n') {
					toAppend = '\n';
				} else if (m_currentCharacter == 'r') {
					toAppend = '\n';
				} else if (m_currentCharacter == 't') {
					toAppend = '\t';
				} else if (m_currentCharacter == 'v') {
					toAppend = '\v';
				} 
				if (toAppend != '\0') {
					m_currentToken.string.append(1, toAppend);
					if (m_stringIsDoubleQuote) {
						ret.push_back(m_currentToken);
						m_currentToken.string.clear();
					}
					m_state = LexicalAnalyzerState::String;
				} else if (m_currentCharacter == 'x') {
					m_state = LexicalAnalyzerState::StringHexEscapeSequence;
				} else if (m_currentCharacter >= '0' && m_currentCharacter <= '7') {
					m_state = LexicalAnalyzerState::StringOctalEscapeSequence;
					m_currentEscapeSequence.append(1, m_currentCharacter);
				} else {
					throw LexicalAnalysisException("Unknown escape sequence encountered at " + m_currentLocation.toString() + " in string started at " + m_currentToken.locationString());
				}
				break;
			}
			case LexicalAnalyzerState::StringOctalEscapeSequence:
				if (m_currentCharacter >= '0' && m_currentCharacter <= '9') {
					m_currentEscapeSequence.append(1, m_currentCharacter);
				} else {
					unsigned long long value = std::stoull(m_currentEscapeSequence, 0, 8);
					size_t bits = (m_currentEscapeSequence.length()-1) * 3;
					if (m_currentEscapeSequence[0] > '4') {
						bits += 3;
					}
					if (bits % 8 > 0) {
						bits = bits + 8 - (bits % 8);
					}
					size_t bytes = bits / 8;
					const char* valueBytesPtr = (char*)&value;
					while (bytes > 0) {
						m_currentToken.string.append(1, *valueBytesPtr);
						if (m_stringIsDoubleQuote) {
							ret.push_back(m_currentToken);
							m_currentToken.string.clear();
						}
						++valueBytesPtr;
						--bytes;
					}
					m_currentEscapeSequence.clear();
					m_consumeNew = false;
					m_state = LexicalAnalyzerState::String;
				}
				break;
			case LexicalAnalyzerState::StringHexEscapeSequence:
				if (m_currentCharacter >= '0' && m_currentCharacter <= '9'
					|| m_currentCharacter >= 'a' && m_currentCharacter <= 'f'
					|| m_currentCharacter >= 'A' && m_currentCharacter <= 'F') {
					m_currentEscapeSequence.append(1, m_currentCharacter);
				} else {
					unsigned long long value = std::stoull(m_currentEscapeSequence, 0, 16);
					size_t bits = m_currentEscapeSequence.length() * 4;
					if (bits % 8 > 0) {
						bits = bits + 8 - (bits % 8);
					}
					size_t bytes = bits / 8;
					const char* valueBytesPtr = (const char*)&value;
					while (bytes > 0) {
						m_currentToken.string.append(1, *valueBytesPtr);
						if (m_stringIsDoubleQuote) {
							ret.push_back(m_currentToken);
							m_currentToken.string.clear();
						}
						++valueBytesPtr;
						--bytes;
					}
					m_currentEscapeSequence.clear();
					m_consumeNew = false;
					m_state = LexicalAnalyzerState::String;
				}
				break;
			case LexicalAnalyzerState::LeftArrow:
				if (m_currentCharacter != '-' || m_endOfStreamReached) {
					throw LexicalAnalysisException("Unrecognized character '" + m_currentToken.string + "' found on line " + m_currentLocation.toString() + ", expected '-' to complete the left-arrow operator '<-'");
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
					throw LexicalAnalysisException("Multiline comment without closure started on " + m_currentToken.locationString() + ", expected '*/' to close the multiline comment before the input stream end");
				}

				if (m_currentCharacter == '*') {
					m_state = LexicalAnalyzerState::MultilineCommentStarEncountered;
				}
				break;
			case LexicalAnalyzerState::MultilineCommentStarEncountered:
				if (m_endOfStreamReached) {
					throw LexicalAnalysisException("Multiline comment without closure started on " + m_currentToken.locationString() + ", expected '*/' (now only '/' is missing as star has recently been encountered) to close the multiline comment before the input stream end");
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