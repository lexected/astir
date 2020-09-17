#pragma once

#include "LLkParserDefinition.h"
#include "MachineStatement.h"

class LLkParserGenerator {
public:
	virtual void visit(const CategoryStatement* category) = 0;
	virtual void visit(const PatternStatement* rule) = 0;
	virtual void visit(const ProductionStatement* rule) = 0;
	virtual void visit(const RegexStatement* rule) = 0;

	virtual void visit(const DisjunctiveRegex* regex) = 0;
	virtual void visit(const ConjunctiveRegex* regex) = 0;

	virtual void visit(const RepetitiveRegex* regex) = 0;

	virtual void visit(const EmptyRegex* regex) = 0;
	virtual void visit(const AnyRegex* regex) = 0;
	virtual void visit(const ExceptAnyRegex* regex) = 0;
	virtual void visit(const LiteralRegex* regex) = 0;
	virtual void visit(const ArbitrarySymbolRegex* regex) = 0;
	virtual void visit(const ReferenceRegex* regex) = 0;

protected:
	LLkParserGenerator(LLkBuilder& builder)
		: m_builder(builder) { }

	LLkBuilder& m_builder;
};