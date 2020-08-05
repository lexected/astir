#include "Token.h"

#include <exception>

std::string Token::toString() const {
	return std::string("type ") + typeString() + ", string \'" + string + "\', location " + locationString();
}

std::string Token::typeString() const {
	return Token::convertTypeToString(this->type);
}

std::string Token::toHumanString() const {
	if (type == TokenType::STRING) {
		return std::string("\"") + string + "\"";
	} else {
		return string;
	}
}

std::string Token::locationString() const {
	return std::to_string(line) + ":" + std::to_string(column);
}

std::string Token::convertTypeToString(TokenType type) {
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
			throw Exception("Unrecognized token type: " + std::to_string((unsigned int)type));
	}
}