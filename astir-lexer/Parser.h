#pragma once

#include "Token.h"
#include <list>
#include <memory>

#include "Exception.h"
#include "Specification.h"

class Parser {
public:
	Parser() = default;

	std::unique_ptr<Specification> parse(const std::list<Token>& tokens) const;
	std::unique_ptr<MachineDefinition> parseMachineDefinition(std::list<Token>::const_iterator & it) const;
	std::unique_ptr<Statement> parseStatement(std::list<Token>::const_iterator& it) const;
};

class ParserException : public Exception {
public:
	ParserException(const std::string& message)
		: Exception(message) {
	}
	ParserException(const std::string& messagePrefix, const Token& currentToken)
		: ParserException(messagePrefix + "\nCurrent Token: " + currentToken.toString()) {
	}
	ParserException(const std::string& messagePrefix, const Token& currentToken, const Token& initialToken)
		: ParserException(messagePrefix + "\nCurrent Token: " + currentToken.toString() + "\Initial Token: " + initialToken.toString()) {
	}
};

class UnexpectedTokenException : public ParserException {
public:
	UnexpectedTokenException(const std::string& message)
		: ParserException(message) { }

	UnexpectedTokenException(const Token& tokenGiven, const std::string& expected)
		: ParserException("Unexpected token " + tokenGiven.toHumanString() + " encountered at " + tokenGiven.locationString() + ", expected " + expected) { }

	UnexpectedTokenException(const Token& tokenGiven, const std::string& expected, const std::string& productionDescription)
		: ParserException("Unexpected token " + tokenGiven.toHumanString() + " encountered at " + tokenGiven.locationString() + " in production " + productionDescription + ", expected " + expected) {
	}

	UnexpectedTokenException(const Token& tokenGiven, const std::string& expected, const std::string& productionDescription, const Token& initialToken)
		: ParserException("Unexpected token " + tokenGiven.toHumanString() + " encountered at " + tokenGiven.locationString() + " in production " + productionDescription  + ", expected " + expected + ". The current production started at " + initialToken.locationString() + " with token " + initialToken.toHumanString()) { }
};