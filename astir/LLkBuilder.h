#pragma once

#include "MachineStatement.h"
#include "Regex.h"

#include <map>

#include "ILLkFirstable.h"
#include "SymbolGroup.h"

struct LLkNonterminalContext {
	ILLkFirstableCPtr parent;
	std::list<ILLkFirstableCPtr> followedBy;

	LLkNonterminalContext()
		: parent(nullptr) { }
	LLkNonterminalContext(ILLkFirstableCPtr parent)
		: parent(parent) { }
	LLkNonterminalContext(ILLkFirstableCPtr parent, const std::list<ILLkFirstableCPtr>& followedBy)
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

	void disambiguate(const std::list<ILLkFirstableCPtr>& alternatives);
	void disambiguatePair(ILLkFirstableCPtr first, ILLkFirstableCPtr second);
	void disambiguateDecisionPoints(ILLkFirstableCPtr first, ILLkFirstableCPtr second, LLkDecisionPoint& firstPoint, LLkDecisionPoint& secondPoint, SymbolGroupList& prefix);
	void fillDisambiguationParent(ILLkFirstableCPtr parent, const std::list<ILLkFirstableCPtr>& alternatives);

	SymbolGroupList lookahead(ILLkFirstableCPtr nonterminal, const SymbolGroupList& prefix);

	const MachineDefinition& contextMachine() const { return m_contextMachine; }
	const std::map<ILLkFirstableCPtr, LLkFlyweight>& flyweights() const { return m_flyweights; }
private:
	const unsigned long m_k;
	const MachineDefinition& m_contextMachine;
	std::map<ILLkFirstableCPtr, LLkFlyweight> m_flyweights;

	SymbolGroupList sequentialLookahead(std::list<ILLkFirstableCPtr>::const_iterator& sequenceIt, const std::list<ILLkFirstableCPtr>::const_iterator& sequenceEnd, const SymbolGroupList& prefix);
	void registerContextAppearance(ILLkFirstableCPtr target, ILLkFirstableCPtr parent, const std::list<ILLkFirstableCPtr>& followedBy);
	void registerContextAppearance(ILLkFirstableCPtr target, ILLkFirstableCPtr parent, std::list<ILLkFirstableCPtr>::const_iterator followedByIt, std::list<ILLkFirstableCPtr>::const_iterator followedByEnd);
};

