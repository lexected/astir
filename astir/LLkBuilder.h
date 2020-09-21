#pragma once

#include "MachineStatement.h"
#include "Regex.h"

#include <map>

#include "ILLkFirstable.h"
#include "SymbolGroup.h"
#include "LLkFirster.h"

struct LLkNonterminalContext {
	ILLkNonterminalCPtr parent;
	std::list<ILLkFirstableCPtr> followedBy;

	LLkNonterminalContext()
		: parent(nullptr) { }
	LLkNonterminalContext(ILLkNonterminalCPtr parent)
		: parent(parent) { }
	LLkNonterminalContext(ILLkNonterminalCPtr parent, const std::list<ILLkFirstableCPtr>& followedBy)
		: parent(parent), followedBy(followedBy) { }
};

struct LLkTransition;
struct LLkDecisionPoint {
	std::list<std::shared_ptr<LLkTransition>> transitions;

	SymbolGroupList computeConditionSymbols() const;

	LLkDecisionPoint& operator+=(const LLkDecisionPoint& rhs);
	size_t maxDepth() const;
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

	void visitRootDisjunction(const std::list<std::shared_ptr<TypeFormingStatement>>& rootDisjunction);

	void visit(const CategoryStatement* categoryStatement);
	void visit(const RuleStatement* ruleStatement);

	void visit(const DisjunctiveRegex* regex);
	void visit(const ConjunctiveRegex* regex);

	void visit(const RepetitiveRegex* regex); 
	void visit(const ReferenceRegex* regex);

	void disambiguate(const std::list<ILLkNonterminalCPtr>& alternatives);
	void disambiguatePair(ILLkNonterminalCPtr first, ILLkNonterminalCPtr second);
	void disambiguateDecisionPoints(ILLkNonterminalCPtr first, ILLkNonterminalCPtr second, LLkDecisionPoint& firstPoint, LLkDecisionPoint& secondPoint, SymbolGroupList& prefix);
	void fillDisambiguationParent(ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& alternatives);

	SymbolGroupList lookahead(ILLkFirstableCPtr nonterminal, const SymbolGroupList& prefix);

	LLkDecisionPoint getDecisionTree(ILLkFirstableCPtr firstable);

	const MachineDefinition& contextMachine() const { return m_contextMachine; }
	LLkFirster& firster() { return m_firster; }
	const std::map<ILLkNonterminalCPtr, LLkFlyweight>& flyweights() const { return m_flyweights; }
private:
	const unsigned long m_k;
	const MachineDefinition& m_contextMachine;
	std::map<ILLkNonterminalCPtr, LLkFlyweight> m_flyweights;
	LLkFirster m_firster;

	SymbolGroupList sequentialLookahead(std::list<ILLkFirstableCPtr>::const_iterator& sequenceIt, const std::list<ILLkFirstableCPtr>::const_iterator& sequenceEnd, const SymbolGroupList& prefix);
	void registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, const std::list<ILLkFirstableCPtr>& followedBy);
	void registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, std::list<ILLkFirstableCPtr>::const_iterator followedByIt, std::list<ILLkFirstableCPtr>::const_iterator followedByEnd);
};

