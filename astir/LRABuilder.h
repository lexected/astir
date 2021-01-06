#pragma once

#include "WeakContext.h"
#include "LRA.h"

#include "SyntacticTree.h"
#include "MachineStatement.h"
#include "Regex.h"

class LRABuilder {
public:
	LRABuilder(const MachineDefinition& contextMachine)
		: m_contextMachine(contextMachine) { }

	void visit(const CategoryStatement* category, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const PatternStatement* rule, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const ProductionStatement* rule, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const RegexStatement* rule, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	
	void visit(const DisjunctiveRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const ConjunctiveRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	
	void visit(const RepetitiveRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	
	void visit(const EmptyRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const AnyRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const ExceptAnyRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const LiteralRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const ArbitrarySymbolRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;
	void visit(const ReferenceRegex* regex, LRA* lra, AFAState startingState, std::shared_ptr<WeakContext> context) const;

private:
	const MachineDefinition& m_contextMachine;
};