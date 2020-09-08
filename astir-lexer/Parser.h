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

	std::unique_ptr<MachineStatement> parseMachineStatement(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<CategoryStatement> parseCategoryStatement(std::list<Token>::const_iterator& it, Rootness rootness) const;
	void parseInAttributedStatement(std::list<Token>::const_iterator& productionStartIt, std::list<Token>::const_iterator& it, AttributedStatement& statement, const std::string& attributedStatementType) const;
	void parseInRuleStatement(std::list<Token>::const_iterator& productionStartIt, std::list<Token>::const_iterator& it, RuleStatement& statement, const std::string& attributedStatementType) const;
	std::unique_ptr<ProductionStatement> parseProductionStatement(std::list<Token>::const_iterator& it, Rootness rootness, Terminality terminality) const;
	std::unique_ptr<PatternStatement> parsePatternStatement(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RegexStatement> parseRegexStatement(std::list<Token>::const_iterator& it) const;

	std::unique_ptr<Field> parseMemberDeclaration(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RootRegex> parseRootRegex(std::list<Token>::const_iterator& it) const;
	std::unique_ptr<RepetitiveRegex> parseRepetitiveRegex(std::list<Token>::const_iterator& it) const;
	RegexActionType parseRegexAction(std::list<Token>::const_iterator& it) const;

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