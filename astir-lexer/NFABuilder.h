#pragma once

#include "Regex.h"
#include "NFA.h"

class Machine;

class NFABuilder {
public:
	NFABuilder(const Machine& context)
		: m_context(context) { }

	NFA visit( const DisjunctiveRegex* regex) const;
	NFA visit(const ConjunctiveRegex* regex) const;

	NFA visit(const RepetitiveRegex* regex) const;
	NFA visit(const LookaheadRegex* regex) const;
	NFA visit(const ActionAtomicRegex* regex) const;
	
	NFA visit(const AnyRegex* regex) const;
	NFA visit(const ExceptAnyRegex* regex) const;
	NFA visit(const LiteralRegex* regex) const;
	NFA visit(const ArbitraryLiteralRegex* regex) const;
	NFA visit(const ReferenceRegex* regex) const; 
	NFA visit(const LineEndRegex* regex) const;
private:
	const Machine& m_context;

	std::list<LiteralSymbolGroup> computeLiteralGroups(const AnyRegex* regex) const;
};

