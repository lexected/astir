#pragma once

#include <set>
#include <vector>
#include <list>

#include "Regex.h"

class Machine;
class MachineComponent;
struct Field;

using State = size_t;

struct ActionRegisterEntry {
	RegexAction action;
	const Field* targetComponent;

	ActionRegisterEntry(RegexAction action)
		: action(action), targetComponent(nullptr) { }
	ActionRegisterEntry(RegexAction action, const Field* targetComponent)
		: action(action), targetComponent(targetComponent) { }
};

class ActionRegister : public std::list<ActionRegisterEntry> {
public:
	ActionRegister() = default;

	ActionRegister operator+(const ActionRegister& rhs) const;
	const ActionRegister& operator+=(const ActionRegister& rhs);
};

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	ActionRegister actions;

	virtual bool contains(const SymbolGroup* symbol) const = 0;
protected:
	SymbolGroup() = default;
	SymbolGroup(const ActionRegister& actions)
		: actions(actions) { }
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: rangeStart(0), rangeEnd(0) { }
	LiteralSymbolGroup(unsigned char rangeStart, unsigned  char rangeEnd)
		: rangeStart(rangeStart), rangeEnd(rangeEnd) { }
	LiteralSymbolGroup(unsigned char rangeStart, unsigned  char rangeEnd, const ActionRegister& actions)
		: rangeStart(rangeStart), rangeEnd(rangeEnd), SymbolGroup(actions) { }
	LiteralSymbolGroup(const LiteralSymbolGroup& lsg, const ActionRegister& actions)
		: rangeStart(lsg.rangeStart), rangeEnd(lsg.rangeEnd), SymbolGroup(actions) { }

	bool contains(const SymbolGroup* symbol) const override;
	bool equals(const LiteralSymbolGroup& rhs) const;
	bool disjoint(const LiteralSymbolGroup& lhs) const;
	static void disjoin(std::list<LiteralSymbolGroup>& symbolGroups, const LiteralSymbolGroup& lhs, const LiteralSymbolGroup& rhs);

	unsigned char rangeStart;
	unsigned char rangeEnd;
};

struct ArbitrarySymbolGroup : public LiteralSymbolGroup {
	ArbitrarySymbolGroup()
		: LiteralSymbolGroup(0, (char)255) { }
	ArbitrarySymbolGroup(const ActionRegister& actions)
		: LiteralSymbolGroup(0, (char)255, actions) { }
};

struct ProductionSymbolGroup : public SymbolGroup {
	const MachineComponent* referencedComponent;
	ProductionSymbolGroup(const MachineComponent* referencedComponent)
		: referencedComponent(referencedComponent) { }
	ProductionSymbolGroup(const MachineComponent* referencedComponent, const ActionRegister& actions)
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
};

class NFA {
public:
	std::set<State> finalStates;
	std::vector<NFAState> states;
	// 0th element of this vector is by default the initial state

	NFA();

	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	State addState();
	void addTransition(State state, const Transition& transition);
	Transition& addEmptyTransition(State state, State target);
	State concentrateFinalStates();

	NFA buildDFA() const;

	static void calculateDisjointLiteralSymbolGroups(std::list<LiteralSymbolGroup>& symbolGroups);
	static std::list<LiteralSymbolGroup> negateLiteralSymbolGroups(const std::list<LiteralSymbolGroup>& symbolGroups);
private:
	std::set<State> calculateEpsilonClosure(const std::set<State>& states) const;
	std::set<State> calculateSymbolClosure(const std::set<State>& states, const SymbolGroup* symbolOnTransition) const;
	std::list<std::shared_ptr<SymbolGroup>> calculateTransitionSymbols(const std::set<State>& states) const;
	static void calculateDisjointProductionSymbolGroups(std::list<ProductionSymbolGroup>& symbolGroups);

	struct DFAState {
		std::set<State> nfaStates;
		bool marked;

		DFAState()
			: marked(false) { }
		DFAState(const std::set<State>& nfaStates)
			: nfaStates(nfaStates), marked(false) { }
	};
	State findUnmarkedState(const std::vector<DFAState>& stateMap) const;
	State findStateByNFAStateSet(const std::vector<DFAState>& stateMap, const std::set<State>& nfaSet) const;
};
