#pragma once

#include "LLkParserDefinition.h"
#include "MachineStatement.h"

class CppLLkParserGenerator {
public:
	CppLLkParserGenerator(const LLkBuilder& builder);

	NFA visit(const CategoryStatement* category) const;
	NFA visit(const PatternStatement* rule) const;
	NFA visit(const ProductionStatement* rule) const;
	NFA visit(const RegexStatement* rule) const;

	NFA visit(const DisjunctiveRegex* regex) const;
	NFA visit(const ConjunctiveRegex* regex) const;

	NFA visit(const RepetitiveRegex* regex) const;

	NFA visit(const EmptyRegex* regex) const;
	NFA visit(const AnyRegex* regex) const;
	NFA visit(const ExceptAnyRegex* regex) const;
	NFA visit(const LiteralRegex* regex) const;
	NFA visit(const ArbitrarySymbolRegex* regex) const;
	NFA visit(const ReferenceRegex* regex) const;

private:
	const LLkBuilder& m_builder;
};