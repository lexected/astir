#include "Token.h"

#include <exception>

std::string Token::toString() const {
	return std::string("type ") + typeString() + ", string \'" + string + "\', location " + locationString();
}

void Token::setLocation(const FileLocation& loc) {
	m_fileLocation = loc;
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

const FileLocation& Token::location() const {
	return m_fileLocation;
}

std::string Token::convertTypeToString(TokenType type) {
	switch (type) {
		case TokenType::KW_USES:
			return "KW_USES";

		case TokenType::KW_WITH:
			return "KW_WITH";
		case TokenType::KW_ON:
			return "KW_ON";
		
		case TokenType::KW_FINITE:
			return "KW_FINITE";
		case TokenType::KW_AUTOMATON:
			return "KW_AUTOMATON";
		case TokenType::KW_PRODUCTIONS_TERMINAL_BY_DEFAULT:
			return "KW_PRODUCTIONS_TERMINAL_BY_DEFAULT";
		case TokenType::KW_PRODUCTIONS_NONTERMINAL_BY_DEFAULT:
			return "KW_PRODUCTIONS_NONTERMINAL_BY_DEFAULT";
		case TokenType::KW_RULES_PRODUCTIONS_BY_DEFAULT:
			return "KW_RULES_PRODUCTIONS_BY_DEFAULT";
		case TokenType::KW_RULES_PATTERNS_BY_DEFAULT:
			return "KW_RULES_PATTERNS_BY_DEFAULT";
		case TokenType::KW_PRODUCTIONS_ROOT_BY_DEFAULT:
			return "KW_PRODUCTIONS_ROOT_BY_DEFAULT";
		case TokenType::KW_PRODUCTIONS_NONROOT_BY_DEFAULT:
			return "KW_PRODUCTIONS_NONROOT_BY_DEFAULT";
		case TokenType::KW_CATEGORIES_ROOT_BY_DEFAULT:
			return "KW_CATEGORIES_ROOT_BY_DEFAULT";
		case TokenType::KW_CATEGORIES_NONROOT_BY_DEFAULT:
			return "KW_CATEGORIES_NONROOT_BY_DEFAULT";

		
		case TokenType::KW_ROOT:
			return "KW_ROOT";
		case TokenType::KW_IGNORED:
			return "KW_IGNORED";
		case TokenType::KW_TERMINAL:
			return "KW_TERMINAL";
		case TokenType::KW_NONTERMINAL:
			return "KW_NONTERMINAL";
		case TokenType::KW_CATEGORY:
			return "KW_CATEGORY";
		case TokenType::KW_PRODUCTION:
			return "KW_PRODUCTION";
		case TokenType::KW_PATTERN:
			return "KW_PATTERN";
		case TokenType::KW_REGEX:
			return "KW_REGEX";

		case TokenType::KW_ITEM:
			return "KW_ITEM";
		case TokenType::KW_LIST:
			return "KW_LIST";
		case TokenType::KW_RAW:
			return "KW_RAW";

		case TokenType::KW_FLAG:
			return "KW_FLAG";
		case TokenType::KW_UNFLAG:
			return "KW_UNFLAG";

		case TokenType::KW_CAPTURE:
			return "KW_CAPTURE";
		case TokenType::KW_EMPTY:
			return "KW_EMPTY";
		case TokenType::KW_APPEND:
			return "KW_APPEND";
		case TokenType::KW_PREPEND:
			return "KW_PREPEND";
		
		case TokenType::KW_SET:
			return "KW_SET";
		case TokenType::KW_UNSET:
			return "KW_UNSET";
		
		case TokenType::KW_PUSH:
			return "KW_PUSH";
		case TokenType::KW_POP:
			return "KW_POP";
		case TokenType::KW_CLEAR:
			return "KW_CLEAR";
		
		case TokenType::IDENTIFIER:
			return "IDENTIFIER";
		case TokenType::STRING:
			return "STRING";
		case TokenType::NUMBER:
			return "NUMBER";

		case TokenType::PAR_LEFT:
			return "PAR_LEFT";
		case TokenType::PAR_RIGHT:
			return "PAR_RIGHT";
		case TokenType::SQUARE_LEFT:
			return "SQUARE_LEFT";
		case TokenType::SQUARE_RIGHT:
			return "SQUARE_RIGHT";
		case TokenType::CURLY_LEFT:
			return "CURLY_LEFT";
		case TokenType::CURLY_RIGHT:
			return "CURLY_RIGHT";

		case TokenType::OP_COLON:
			return "OP_COLON";
		case TokenType::OP_EQUALS:
			return "OP_EQUALS";
		case TokenType::OP_LEFTARR:
			return "OP_LEFTARR";
		case TokenType::OP_SEMICOLON:
			return "OP_SEMICOLON";
		case TokenType::OP_COMMA:
			return "OP_COMMA";
		case TokenType::OP_DOT:
			return "OP_DOT";
		case TokenType::OP_CARET:
			return "OP_CARET";
		case TokenType::OP_DOLLAR:
			return "OP_DOLLAR";
		
		case TokenType::OP_STAR:
			return "OP_STAR";
		case TokenType::OP_PLUS:
			return "OP_PLUS";
		case TokenType::OP_QM:
			return "OP_QM";
		case TokenType::OP_OR:
			return "OP_OR";
		case TokenType::OP_FWDSLASH:
			return "OP_FWDSLASH";

		case TokenType::OP_AMPERSAND:
			return "OP_AMPERSAND";
		case TokenType::OP_DASH:
			return "OP_DASH";
		case TokenType::OP_AT:
			return "OP_AT";
		
		case TokenType::EOS:
			return "EOS";

		default:
			throw Exception("Unrecognized token type: " + std::to_string((unsigned long)type));
	}
}