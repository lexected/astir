#pragma once

#include <set>
#include <vector>
#include <list>

#include "Regex.h"

class Machine;
class MachineComponent;

using State = size_t;

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	virtual bool contains(const SymbolGroup* symbol) const = 0;
protected:
	SymbolGroup() = default;
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: rangeStart(0), rangeEnd(0) { }
	LiteralSymbolGroup(unsigned char rangeStart, unsigned  char rangeEnd)
		: rangeStart(rangeStart), rangeEnd(rangeEnd) { }

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
};

struct ProductionSymbolGroup : public SymbolGroup {
	const MachineComponent* referencedComponent;
	ProductionSymbolGroup(const MachineComponent* referencedComponent)
		: referencedComponent(referencedComponent) { }

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

enum class ActionRegisterEntryType {
	Add,
	Remove
};

struct ActionRegisterEntry : public ActionTargetPair {
	ActionRegisterEntryType type;

	ActionRegisterEntry(ActionRegisterEntryType type, const ActionTargetPair& pair)
		: type(type), ActionTargetPair(pair.action, pair.target) { }
};

using ActionRegister = std::list<ActionRegisterEntry>;

struct NFAState {
	TransitionList transitions;
	ActionRegister actions;
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
	static void calculateDisjointProductionSymbolGroups(std::list<const MachineComponent*>& symbolGroups);

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
