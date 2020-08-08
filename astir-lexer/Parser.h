#pragma once

#include "Token.h"
#include <list>
#include <memory>

#include "Exception.h"
#include "SpecificationFile.h"

class Parser {
public:
	Parser() = default;

	std::unique_ptr<SpecificationFile> parse(const std::list<Token>& tokens) const;
	std::unique_ptr<UsingStatement> parseUsingStatement(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<MachineDefinition> parseMachineDefinition(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<FADefinition> parseFADefinition(std::list<Token>::const_iterator & it) const;
	std::unique_ptr<MachineStatement> parseMachineStatement(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<CategoryStatement> parseCategoryStatement(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<GrammarStatement> parseGrammarStatement(std::list<Token>::const_iterator& it) const;

	std::unique_ptr<MemberDeclaration> parseMemberDeclaration(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RootRegex> parseRootRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RepetitiveRegex> parseRepetitiveRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<LookaheadRegex> parseLookaheadRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<ActionAtomicRegex> parseActionAtomicRegex(std::list<Token>::const_iterator& it) const;
	RegexAction parseRegexAction(std::list<Token>::const_iterator& it) const;

	std::unique_ptr<AtomicRegex> parseAtomicRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<AnyRegex> parseAnyRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<ConjunctiveRegex> parseConjunctiveRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<DisjunctiveRegex> parseDisjunctiveRegex(std::list<Token>::const_iterator& it) const;
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
		: ParserException(messagePrefix + "\nCurrent Token: " + currentToken.toString() + "\nInitial Token: " + initialToken.toString()) {
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