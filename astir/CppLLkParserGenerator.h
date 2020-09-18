#pragma once

#include "LLkParserGenerator.h"
#include "IndentedStringStream.h"

class CppLLkParserGenerator : public LLkParserGenerator {
public:
	CppLLkParserGenerator(LLkBuilder& builder);

	void visitRootDisjunction(const std::list<std::shared_ptr<TypeFormingStatement>>& rootDisjunction) override;

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
	void handleRuleBody(const RuleStatement* rule);
	void outputConditionTesting(const LLkDecisionPoint& dp, unsigned long depth = 0);
	void outputCondition(const std::shared_ptr<SymbolGroup>& sgPtr, unsigned long depth);

	IndentedStringStream m_output;
	std::list<std::string> m_declarations;
};