#pragma once

#include "LLkParserGenerator.h"
#include "IndentedStringStream.h"

class CppLLkParserGenerator : public LLkParserGenerator {
public:
	CppLLkParserGenerator(LLkBuilder& builder);

	void visitTypeFormingStatements(const std::list<std::shared_ptr<TypeFormingStatement>>& rootDisjunction) override;
	void visitRootDisjunction(const std::list<std::shared_ptr<TypeFormingStatement>>& typeFormingStatements) override;

	void visit(const CategoryStatement* category) override;
	void visit(const PatternStatement* rule) override;
	void visit(const ProductionStatement* rule) override;
	void visit(const RegexStatement* rule) override;

	void visit(const DisjunctiveRegex* regex) override;
	void visit(const ConjunctiveRegex* regex) override;

	void visit(const RepetitiveRegex* regex) override;

	void visit(const EmptyRegex* regex) override;
	void visit(const AnyRegex* regex) override;
	void visit(const ExceptAnyRegex* regex) override;
	void visit(const LiteralRegex* regex) override;
	void visit(const ArbitrarySymbolRegex* regex) override;
	void visit(const ReferenceRegex* regex) override;

	std::string parsingDeclarations() const;
	std::string parsingDefinitions() const { return m_output.str(); }
private:
	void handleTypeFormingPreamble(const std::string& typeName);
	void handleTypeFormingPostamble();
	void handleRuleBody(const RuleStatement* rule);
	std::string makeConditionTesting(const LLkDecisionPoint& dp, unsigned long depth = 0, bool needsUnpeeking = false) const;
	std::string makeCondition(const std::shared_ptr<SymbolGroup>& sgPtr, std::string& postamble, unsigned long depth) const;
	std::string makeExpectationMessage(const LLkDecisionPoint& dp);
	std::string makeExpectationMessage(const std::vector<LLkDecisionPoint>& dps);
	std::string makeExpectationGrammar(const LLkDecisionPoint& dp);

	IndentedStringStream m_output;
	std::list<std::string> m_declarations;
};