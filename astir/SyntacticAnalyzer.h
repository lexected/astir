#pragma once

#include "Token.h"
#include <list>
#include <memory>

#include "SyntacticTree.h"
#include "SyntacticAnalysisException.h"

class SyntacticAnalyzer {
public:
	SyntacticAnalyzer() = default;

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
