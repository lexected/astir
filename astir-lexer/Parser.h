#pragma once

#include "Token.h"
#include <list>
#include <memory>

#include "Exception.h"
#include "SyntacticTree.h"

class Parser {
public:
	Parser() = default;

	std::unique_ptr<SyntacticTree> parse(const std::list<Token>& tokens) const;
private:
	std::unique_ptr<UsesStatement> parseUsesStatement(std::list<Token>::const_iterator& it) const;
	
	std::unique_ptr<MachineDefinition> parseMachineDefinition(std::list<Token>::const_iterator& it) const;
	bool tryParseMachineFlag(std::list<Token>::const_iterator& it, std::map<MachineFlag, MachineDefinitionAttribute>& attributes) const;
	std::unique_ptr<MachineDefinition> parseMachineType(std::list<Token>::const_iterator & it) const;

	std::unique_ptr<CategoryStatement> parseCategoryStatement(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RuleStatement> parseRuleStatement(std::list<Token>::const_iterator& it) const;

	std::unique_ptr<Field> parseMemberDeclaration(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RootRegex> parseRootRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RepetitiveRegex> parseRepetitiveRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<LookaheadRegex> parseLookaheadRegex(std::list<Token>::const_iterator& it) const;
	RegexAction parseRegexAction(std::list<Token>::const_iterator& it) const;

	std::unique_ptr<AtomicRegex> parseAtomicRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<PrimitiveRegex> parsePrimitiveRegex(std::list<Token>::const_iterator& it) const;
	bool tryParseAnyRegex(std::list<Token>::const_iterator& it, std::unique_ptr<AnyRegex>& anyRegexPtr) const;
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