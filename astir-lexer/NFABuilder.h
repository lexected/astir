#pragma once

#include "Regex.h"
#include "NFA.h"

class Machine;

class NFABuilder {
public:
	NFABuilder(const Machine& context, const MachineComponent* component)
		: m_context(context), m_component(component) { }

	NFA visit(const DisjunctiveRegex* regex) const;
	NFA visit(const ConjunctiveRegex* regex) const;

	NFA visit(const RepetitiveRegex* regex) const;
	NFA visit(const LookaheadRegex* regex) const;
	
	NFA visit(const AnyRegex* regex) const;
	NFA visit(const ExceptAnyRegex* regex) const;
	NFA visit(const LiteralRegex* regex) const;
	NFA visit(const ArbitraryLiteralRegex* regex) const;
	NFA visit(const ReferenceRegex* regex) const; 
	NFA visit(const LineEndRegex* regex) const;
private:
	const Machine& m_context;
	const MachineComponent* m_component;

	std::list<LiteralSymbolGroup> computeLiteralGroups(const AnyRegex* regex) const;
	ActionRegister computeActionRegisterEntries(const std::list<ActionTargetPair>& actionTargetPairs) const;
};

