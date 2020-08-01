#include "LexicalAnalyzer.h"

#include <string>
#include <cctype>

std::list<Token> LexicalAnalyzer::process(std::istream& input) {
	std::list<Token> ret;
	LexicalAnalyzerState state = LexicalAnalyzerState::Default;
	unsigned int currentColumn = 0;
	Token currentToken;

	char c;
	bool consumeNew = true;
	bool endOfFileReached = false;
	while (!endOfFileReached) {
		if (consumeNew) {
			if (!input.get(c)) {
				endOfFileReached = true;
				if (state == LexicalAnalyzerState::Default) {
					break;
				}
			} else {
				if (c == '\n') {
					++currentToken.line;
					currentColumn = 0;
				} else {
					++currentColumn;
				}
			}
		} else {
			consumeNew = true;
		}

		switch (state) {
			case LexicalAnalyzerState::Default:
				if (std::isalpha(c)) {
					state = LexicalAnalyzerState::Identifier;
					currentToken.string.append(std::string({ c }));
					currentToken.type = TokenType::IDENTIFIER;
					currentToken.column = currentColumn;
				} else if(std::isalnum(c)) {
					state = LexicalAnalyzerState::Number;
					currentToken.string.append(std::string({ c }));
					currentToken.type = TokenType::NUMBER;
					currentToken.column = currentColumn;
				} else if (isspace(c)) {
					/* do nothin' */
				} else if (c == '"') {
					state = LexicalAnalyzerState::String;
					currentToken.type = TokenType::STRING;
					currentToken.column = currentColumn;
				} else if (c == '<') {
					state = LexicalAnalyzerState::LeftArrow;
					currentToken.string = "<-";
					currentToken.type = TokenType::OP_LEFTARR;
					currentToken.column = currentColumn;
				} else {
					currentToken.string = std::string({ c });
					currentToken.column = currentColumn;
					switch (c) {
						case '(':
							currentToken.type = TokenType::PAR_LEFT;
							break;
						case ')':
							currentToken.type = TokenType::PAR_RIGHT;
							break;
						case '[':
							currentToken.type = TokenType::SQUARE_LEFT;
							break;
						case ']':
							currentToken.type = TokenType::SQUARE_RIGHT;
							break;
						case '{':
							currentToken.type = TokenType::CURLY_LEFT;
							break;
						case '}':
							currentToken.type = TokenType::CURLY_RIGHT;
							break;
						case '=':
							currentToken.type = TokenType::OP_EQUALS;
							break;
						case ':':
							currentToken.type = TokenType::OP_COLON;
							break;
						case ';':
							currentToken.type = TokenType::OP_SEMICOLON;
							break;
						case '.':
							currentToken.type = TokenType::OP_DOT;
							break;
						case '^':
							currentToken.type = TokenType::OP_CARET;
							break;
						case '$':
							currentToken.type = TokenType::OP_DOLLAR;
							break;
						case '*':
							currentToken.type = TokenType::OP_STAR;
							break;
						case '+':
							currentToken.type = TokenType::OP_PLUS;
							break;
						case '?':
							currentToken.type = TokenType::OP_QM;
							break;
						case '|':
							currentToken.type = TokenType::OP_OR;
							break;
						case '/':
							currentToken.type = TokenType::OP_FWDSLASH;
							break;
						case ',':
							currentToken.type = TokenType::OP_COMMA;
							break;
						case '&':
							currentToken.type = TokenType::OP_AMPERSAND;
							break;
						case '-':
							currentToken.type = TokenType::OP_DASH;
							break;
						default:
							throw LexicalAnalyzerException("Unrecognized character '" + currentToken.string + "' found on line " + std::to_string(currentToken.line) + ":" + std::to_string(currentToken.column));
							break;
					}
					ret.push_back(currentToken);
					currentToken.string.clear();
				}
				break;
			case LexicalAnalyzerState::Identifier:
				if (std::isalnum(c) && !endOfFileReached) {
					currentToken.string.append(std::string({ c }));
				} else {
					ret.push_back(currentToken);
					currentToken.string.clear();
					state = LexicalAnalyzerState::Default;
					consumeNew = false;
				}
				break;
			case LexicalAnalyzerState::Number:
				if (std::isdigit(c) && !endOfFileReached) {
					currentToken.string.append(std::string({ c }));
				} else {
					ret.push_back(currentToken);
					currentToken.string.clear();
					state = LexicalAnalyzerState::Default;
					consumeNew = false;
				}
				break;
			case LexicalAnalyzerState::String:
				if (c != '"' && !endOfFileReached) {
					currentToken.string.append(std::string({ c }));
				} else {
					ret.push_back(currentToken);
					currentToken.string.clear();
					state = LexicalAnalyzerState::Default;
				}
				break;
			case LexicalAnalyzerState::LeftArrow:
				if (c != '-' || endOfFileReached) {
					throw LexicalAnalyzerException("Unrecognized character '" + currentToken.string + "' found on line " + std::to_string(currentToken.line) + ":" + std::to_string(currentToken.column) + ", expected '-' to complete the left-arrow operator '<-'");
				} else {
					ret.push_back(currentToken);
					currentToken.string.clear();
					state = LexicalAnalyzerState::Default;
				}
				break;
		}
	}

	return ret;
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
			return "]";
		case TokenType::SQUARE_RIGHT:
			return "[";
		case TokenType::STRING:
			return "STRING";
		default:
			throw LexicalAnalyzerException("Unrecognized token type: " + std::to_string((unsigned int)type));
	}
}
