#pragma once

#include "Regex.h"
#include "NFA.h"

class NFABuilder {
public:
	NFA visit(const DisjunctiveRegex* regex) const;
	NFA visit(const ConjunctiveRegex* regex) const;

	NFA visit(const RepetitiveRegex* regex) const;
	NFA visit(const LookaheadRegex* regex) const;
	NFA visit(const ActionAtomicRegex* regex) const;
	
	NFA visit(const PrimitiveRegex* regex) const;
};

