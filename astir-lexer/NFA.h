#pragma once

#include <set>
#include <deque>
#include <vector>
#include <list>
#include <map>

#include "Regex.h"
#include "NFAAction.h"

class Machine;
class MachineComponent;

using State = size_t;

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	// a necessary part of SymbolGroup I am afraid, the SymbolGroup is the literal payload for the action
	NFAActionRegister actions;

	virtual bool contains(const SymbolGroup* symbol) const = 0;

protected:
	SymbolGroup() = default;
	SymbolGroup(const NFAActionRegister& actions)
		: actions(actions) { }
};

struct EmptySymbolGroup : public SymbolGroup {
public:
	EmptySymbolGroup()
		: SymbolGroup() { }
	EmptySymbolGroup(const NFAActionRegister & actions)
		: SymbolGroup(actions) { }

	bool contains(const SymbolGroup* symbol) const override;
protected:
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: rangeStart(0), rangeEnd(0) { }
	LiteralSymbolGroup(CharType rangeStart, CharType rangeEnd)
		: rangeStart(rangeStart), rangeEnd(rangeEnd) { }
	LiteralSymbolGroup(CharType rangeStart, CharType rangeEnd, const NFAActionRegister& actions)
		: rangeStart(rangeStart), rangeEnd(rangeEnd), SymbolGroup(actions) { }
	LiteralSymbolGroup(const LiteralSymbolGroup& lsg, const NFAActionRegister& actions)
		: rangeStart(lsg.rangeStart), rangeEnd(lsg.rangeEnd), SymbolGroup(actions) { }

	bool contains(const SymbolGroup* symbol) const override;
	bool equals(const LiteralSymbolGroup& rhs) const;
	bool disjoint(const LiteralSymbolGroup& lhs) const;
	static void disjoin(std::list<LiteralSymbolGroup>& symbolGroups, const LiteralSymbolGroup& lhs, const LiteralSymbolGroup& rhs);

	CharType rangeStart;
	CharType rangeEnd;
};

struct ArbitrarySymbolGroup : public LiteralSymbolGroup {
	ArbitrarySymbolGroup()
		: LiteralSymbolGroup(0, (char)255) { }
	ArbitrarySymbolGroup(const NFAActionRegister& actions)
		: LiteralSymbolGroup(0, (char)255, actions) { }
};

struct ProductionSymbolGroup : public SymbolGroup {
	const MachineComponent* referencedComponent;
	ProductionSymbolGroup(const MachineComponent* referencedComponent)
		: referencedComponent(referencedComponent) { }
	ProductionSymbolGroup(const MachineComponent* referencedComponent, const NFAActionRegister& actions)
		: referencedComponent(referencedComponent), SymbolGroup(actions) { }

	bool contains(const SymbolGroup* symbol) const override;
};

struct Transition {
	std::shared_ptr<SymbolGroup> condition;
	State target;

	Transition(State target)
		: target(target), condition(nullptr) { }
	Transition(State target, const std::shared_ptr<SymbolGroup>& condition)
		: target(target), condition(condition) { }
};

using TransitionList = std::list<Transition>;

struct NFAState {
	TransitionList transitions;
	NFAActionRegister actions;
};

class NFA {
public:
	std::set<State> finalStates;
	std::vector<NFAState> states;
	std::list<std::pair<std::string, std::string>> contexts; // name, type
	// 0th element of this vector is by default the initial state

	NFA();

	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	State addState();
	void addTransition(State state, const Transition& transition);
	Transition& addEmptyTransition(State state, State target);
	Transition& addEmptyTransition(State state, State target, const NFAActionRegister& ar);
	State concentrateFinalStates();
	State concentrateFinalStates(const NFAActionRegister& actions);
	void addFinalActions(const NFAActionRegister& actions);

	void registerContext(const std::string& name, const std::string& type);
	void mergeInContexts(const NFA& rhs);

	NFA buildPseudoDFA() const;

	static void calculateDisjointLiteralSymbolGroups(std::list<LiteralSymbolGroup>& symbolGroups);
	static std::list<LiteralSymbolGroup> negateLiteralSymbolGroups(const std::list<LiteralSymbolGroup>& symbolGroups);

private:
	struct DFAState {
		std::set<State> nfaStates;
		bool marked;
		NFAActionRegister actions;

		DFAState()
			: marked(false) { }
		DFAState(const std::set<State>& nfaStates, const NFAActionRegister& actions)
			: nfaStates(nfaStates), marked(false), actions(actions) { }
	};

	DFAState calculateEpsilonClosure(const std::set<State>& states) const;
	std::set<State> calculateSymbolClosure(const std::set<State>& states, const SymbolGroup* symbolOnTransition) const;
	std::list<std::shared_ptr<SymbolGroup>> calculateTransitionSymbols(const std::set<State>& states) const;
	static void calculateDisjointProductionSymbolGroups(std::list<ProductionSymbolGroup>& symbolGroups);

	State findUnmarkedState(const std::deque<DFAState>& stateMap) const;
	State findStateByNFAStateSet(const std::deque<DFAState>& stateMap, const std::set<State>& nfaSet) const;
};
