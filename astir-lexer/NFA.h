#pragma once

#include <set>
#include <vector>
#include <list>

#include "SpecificationFile.h"

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

struct NFAComponent {
	
};

/*
	THIS WHOLE THING IS ***SCREAMING*** FOR A VISITOR PATTERN!!!
*/

class NFA {
public:
	std::set<State> finalStates;
	std::vector<TransitionList> transitionsByState;
	// 0th element of this vector is by default the initial state

	NFA();

	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	State addState();
	void addTransition(State state, const Transition& transition);
	Transition& addEmptyTransition(State state, State target);
	State concentrateFinalStates();

	static NFA buildFromRegex(const DisjunctiveRegex* regex);
	static NFA buildFromRegex(const ConjunctiveRegex* regex);
	
	static NFA buildFromRegex(const RootRegex* regex);
	static NFA buildFromRegex(const RepetitiveRegex* regex);
	static NFA buildFromRegex(const LookaheadRegex* regex);
	static NFA buildFromRegex(const ActionAtomicRegex* regex);
	
	static NFA buildFromRegex(const AtomicRegex* regex);
	static NFA buildFromRegex(const PrimitiveRegex* regex);
};

