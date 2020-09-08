#pragma once

#include "Exception.h"
#include "Token.h"

class SyntacticAnalysisException : public Exception {
public:
	SyntacticAnalysisException(const std::string& message)
		: Exception(message) { }
	SyntacticAnalysisException(const std::string& messagePrefix, const Token& currentToken)
		: SyntacticAnalysisException(messagePrefix + "\nCurrent Token: " + currentToken.toString()) { }
	SyntacticAnalysisException(const std::string& messagePrefix, const Token& currentToken, const Token& initialToken)
		: SyntacticAnalysisException(messagePrefix + "\nCurrent Token: " + currentToken.toString() + "\nInitial Token: " + initialToken.toString()) { }
};

class UnexpectedTokenException : public SyntacticAnalysisException {
public:
	UnexpectedTokenException(const std::string& message)
		: SyntacticAnalysisException(message) { }

	UnexpectedTokenException(const Token& tokenGiven, const std::string& expected)
		: SyntacticAnalysisException("Unexpected token " + tokenGiven.toHumanString() + " encountered at " + tokenGiven.locationString() + ", expected " + expected) { }

	UnexpectedTokenException(const Token& tokenGiven, const std::string& expected, const std::string& productionDescription)
		: SyntacticAnalysisException("Unexpected token " + tokenGiven.toHumanString() + " encountered at " + tokenGiven.locationString() + " in production " + productionDescription + ", expected " + expected) { }

	UnexpectedTokenException(const Token& tokenGiven, const std::string& expected, const std::string& productionDescription, const Token& initialToken)
		: SyntacticAnalysisException("Unexpected token " + tokenGiven.toHumanString() + " encountered at " + tokenGiven.locationString() + " in production " + productionDescription + ", expected " + expected + ". The current production started at " + initialToken.locationString() + " with token " + initialToken.toHumanString()) { }
};