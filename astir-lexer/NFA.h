#pragma once

#include <set>
#include <deque>
#include <vector>
#include <list>
#include <map>

#include "Regex.h"
#include "NFAAction.h"

using State = size_t;
using SymbolIndex = size_t;

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	virtual bool equals(const SymbolGroup* rhs) const = 0;
	virtual bool disjoint(const SymbolGroup* rhs) const = 0;
	virtual std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) = 0;

	virtual std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const = 0;

protected:
	SymbolGroup() = default;
};

struct EmptySymbolGroup : public SymbolGroup {
public:
	EmptySymbolGroup()
		: SymbolGroup() { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
protected:
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: LiteralSymbolGroup(0, 0) { }
	LiteralSymbolGroup(CharType rangeStart, CharType rangeEnd)
		: SymbolGroup(), rangeStart(rangeStart), rangeEnd(rangeEnd), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }
	LiteralSymbolGroup(const LiteralSymbolGroup& lsg)
		: LiteralSymbolGroup(lsg.rangeStart, lsg.rangeEnd) { }
	
	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	CharType rangeStart;
	CharType rangeEnd;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};

struct ProductionStatement;
struct TerminalSymbolGroup : public SymbolGroup {
	std::list<const ProductionStatement*> referencedProductions;
	TerminalSymbolGroup(const std::list<const ProductionStatement*>& referencedProductions)
		: SymbolGroup(), referencedProductions(referencedProductions), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};

struct Transition {
	State target;
	NFAActionRegister actions;
	std::shared_ptr<SymbolGroup> condition;
	bool doNotOptimizeTargetIntoSymbolClosure;

	Transition(State target)
		: target(target), condition(nullptr), doNotOptimizeTargetIntoSymbolClosure(false) { }
	Transition(State target, const std::shared_ptr<SymbolGroup>& condition, const NFAActionRegister& actions, bool doNotOptimizeTargetIntoSymbolClosure = false)
		: target(target), condition(condition), actions(actions), doNotOptimizeTargetIntoSymbolClosure(doNotOptimizeTargetIntoSymbolClosure) { }
	Transition(State target, const std::shared_ptr<SymbolGroup>& condition)
		: Transition(target, condition, NFAActionRegister()) { }

	bool equals(const Transition& rhs) const;
	bool alignedSymbolWise(const Transition& rhs) const;
	std::list<Transition> disjoinFrom(const Transition& rhs);
};

using TransitionList = std::list<Transition>;

struct NFAState {
	TransitionList transitions;
	NFAActionRegister actions;
};

using CapturePointId = size_t;

class NFA {
public:
	std::set<State> finalStates;
	std::vector<NFAState> states; // 0th element of this vector is by default the initial state
	std::list<std::pair<std::string, std::string>> contexts; // parent context name, subcontext name (also the type)

	NFA();

	void orNFA(const NFA& rhs, bool preventSymbolClosureOptimisation);
	void andNFA(const NFA& rhs, bool preventSymbolClosureOptimisation);
	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	State addState();
	void addTransition(State state, const Transition& transition);
	Transition& addEmptyTransition(State state, State target);
	Transition& addEmptyTransition(State state, State target, const NFAActionRegister& ar);
	void registerContext(const std::string& parentContextName, const std::string& name);

	void addFinalActions(const NFAActionRegister& actions);
	void addInitialActions(const NFAActionRegister& actions);
	State concentrateFinalStates();
	State concentrateFinalStates(const NFAActionRegister& actions);

	NFA buildPseudoDFA() const;

	static void calculateDisjointTransitions(std::list<Transition>& symbolGroups);
	static std::list<std::shared_ptr<LiteralSymbolGroup>> makeComplementSymbolGroups(const std::list<std::shared_ptr<SymbolGroup>>& symbolGroups);

private:
	void mergeInContexts(const NFA& rhs);

	struct DFAState {
		std::set<State> nfaStates;
		bool marked;
		NFAActionRegister actions;

		DFAState()
			: marked(false) { }
		DFAState(const std::set<State>& nfaStates, const NFAActionRegister& actions)
			: nfaStates(nfaStates), marked(false), actions(actions) { }
	};

	struct SymbolClosure {
		std::shared_ptr<SymbolGroup> symbols;
		std::set<State> states;
		NFAActionRegister actions;

		SymbolClosure()
			: symbols(nullptr) { }
		SymbolClosure(const std::shared_ptr<SymbolGroup>& symbols, const std::set<State>& states)
			: symbols(symbols), states(states) { }
		SymbolClosure(const std::shared_ptr<SymbolGroup>& symbols, const std::set<State>& states, const NFAActionRegister& actions)
			: symbols(symbols), states(states), actions(actions) { }
	};

	DFAState calculateEpsilonClosure(const std::set<State>& states) const;
	std::list<NFA::SymbolClosure> calculateSymbolClosures(const std::list<Transition>& transitions) const;
	std::list<Transition> calculateTransitions(const std::set<State>& states) const;
	

	State findUnmarkedState(const std::deque<DFAState>& stateMap) const;
	State findStateByNFAStateSet(const std::deque<DFAState>& stateMap, const std::set<State>& nfaSet) const;
};
