#pragma once

#include "SyntacticTree.h"
#include "Regex.h"

#include <map>

#include "ILLkNonterminal.h"
#include "SymbolGroup.h"

struct LLkNonterminalContext {
	ILLkNonterminalCPtr parent;
	std::list<ILLkNonterminalCPtr> followedBy;

	LLkNonterminalContext()
		: parent(nullptr) { }
	LLkNonterminalContext(ILLkNonterminalCPtr parent)
		: parent(parent) { }
	LLkNonterminalContext(ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& followedBy)
		: parent(parent), followedBy(followedBy) { }
};

struct LLkTransition;
struct LLkDecisionPoint {
	std::list<std::unique_ptr<LLkTransition>> transitions;

	SymbolGroupList computeConditionSymbols() const;

	LLkDecisionPoint& operator+=(const LLkDecisionPoint& rhs);
};

struct LLkTransition {
	std::shared_ptr<SymbolGroup> condition;
	LLkDecisionPoint point;

	LLkTransition() = default;
	LLkTransition(const std::shared_ptr<SymbolGroup>& condition)
		: condition(condition) { }
	LLkTransition(const std::shared_ptr<SymbolGroup>& condition, const LLkDecisionPoint& point)
		: condition(condition), point(point) { }
};

struct LLkFlyweight {
	std::list<LLkNonterminalContext> contexts;
	LLkDecisionPoint decisions;
};

class LLkBuilder {
public:
	LLkBuilder(unsigned long k, const MachineDefinition& context);

	void visit(const CategoryStatement* categoryStatement);
	void visit(const RuleStatement* ruleStatement);

	void visit(const DisjunctiveRegex* regex);
	void visit(const ConjunctiveRegex* regex);

	void visit(const RepetitiveRegex* regex);

	void visit(const EmptyRegex* regex);
	void visit(const AnyRegex* regex);
	void visit(const ExceptAnyRegex* regex);
	void visit(const LiteralRegex* regex);
	void visit(const ArbitrarySymbolRegex* regex);
	void visit(const ReferenceRegex* regex);

	void disambiguate(const std::list<ILLkNonterminalCPtr>& alternatives);
	void disambiguatePair(ILLkNonterminalCPtr first, ILLkNonterminalCPtr second);
	void disambiguateDecisionPoints(ILLkNonterminalCPtr first, ILLkNonterminalCPtr second, LLkDecisionPoint& firstPoint, LLkDecisionPoint& secondPoint, SymbolGroupList& prefix);
	void fillDisambiguationParent(ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& alternatives);

	SymbolGroupList lookahead(ILLkNonterminalCPtr nonterminal, const SymbolGroupList& prefix);

	LLkDecisionPoint getPredictionChain(ILLkNonterminalCPtr nonterminal) const;
private:
	const unsigned long m_k;
	const MachineDefinition& m_contextMachine;
	std::map<ILLkNonterminalCPtr, LLkFlyweight> m_flyweights;

	SymbolGroupList sequentialLookahead(std::list<ILLkNonterminalCPtr>::const_iterator& sequenceIt, const std::list<ILLkNonterminalCPtr>::const_iterator& sequenceEnd, const SymbolGroupList& prefix);
	void registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& followedBy);
	void registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, std::list<ILLkNonterminalCPtr>::const_iterator followedByIt, std::list<ILLkNonterminalCPtr>::const_iterator followedByEnd);
};

