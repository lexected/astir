#pragma once

#include "ILLkFirstable.h"
#include "MachineDefinition.h"

#include "MachineStatement.h"
#include "Regex.h"

class LLkFirster {
public:
	LLkFirster(const MachineDefinition* machine);

	SymbolGroupList visit(const CategoryStatement* cs, const SymbolGroupList& prefix);
	SymbolGroupList visit(const RuleStatement* rs, const SymbolGroupList& prefix);

	SymbolGroupList visit(const RepetitiveRegex* rr, const SymbolGroupList& prefix);
	SymbolGroupList visit(const DisjunctiveRegex* dr, const SymbolGroupList& prefix);
	SymbolGroupList visit(const ConjunctiveRegex* cr, const SymbolGroupList& prefix);

	SymbolGroupList visit(const EmptyRegex* rr, const SymbolGroupList& prefix);
	SymbolGroupList visit(const AnyRegex* ar, const SymbolGroupList& prefix);
	SymbolGroupList visit(const ExceptAnyRegex* ar, const SymbolGroupList& prefix);
	SymbolGroupList visit(const LiteralRegex* lr, const SymbolGroupList& prefix);
	SymbolGroupList visit(const ReferenceRegex* rr, const SymbolGroupList& prefix);
	SymbolGroupList visit(const ArbitrarySymbolRegex* asr, const SymbolGroupList& prefix);

private:
	const MachineDefinition* m_machine;
};

