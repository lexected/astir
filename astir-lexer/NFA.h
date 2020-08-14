#pragma once

#include <set>
#include <vector>
#include <list>

#include "Regex.h"

using State = size_t;

struct Transition {
	const PrimitiveRegex* condition;
	State target;

	Transition(State target)
		: target(target), condition(nullptr) { }
	Transition(State target, const PrimitiveRegex* condition)
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

/*
	THIS WHOLE THING IS ***SCREAMING*** FOR A VISITOR PATTERN!!!
*/

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
};
