#pragma once

#include "LRA.h"

#include "SyntacticTree.h"
#include "MachineStatement.h"
#include "Regex.h"

class LRABuilder {
public:
	LRABuilder(const MachineDefinition& contextMachine)
		: m_contextMachine(contextMachine) { }

	void visit(const CategoryStatement* category, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const PatternStatement* pattern, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const ProductionStatement* production, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const RegexStatement* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	
	void visit(const DisjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const ConjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	
	void visit(const RepetitiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	
	void visit(const EmptyRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const AnyRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const ExceptAnyRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const LiteralRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const ArbitrarySymbolRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;
	void visit(const ReferenceRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const;

private:
	const MachineDefinition& m_contextMachine;

	std::list<SymbolGroupPtrVector> computeItemLookahead(std::list<std::unique_ptr<RootRegex>>::const_iterator symbolPrecededByDotIt, std::list<std::unique_ptr<RootRegex>>::const_iterator endOfProductionIt, SymbolGroupPtrVector parentLookahead) const;
	std::list<SymbolGroupPtrVector> cross(const SymbolGroupPtrVector& initialString, const SymbolGroupList& listOfPossibleUnitContinuations) const;
	SymbolGroupPtrVector truncatedConcat(const SymbolGroupPtrVector& initialString, const SymbolGroupPtrVector& continuation) const;
};