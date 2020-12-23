/* THE ABSTRACT FINITE AUTOMATON */
#pragma once

#include <set>
#include <list>
#include <vector>
#include <memory>



using AFAState = size_t;

class AFACondition {
public:
	virtual bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const = 0;

protected:
	AFACondition() = default;
};

class EmptyAFACondition : public AFACondition {
public:
	EmptyAFACondition() = default;

	bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const override;
};

template <class ConditionType>
class AFATransition {
public:
	AFAState target;
	std::shared_ptr<ConditionType> condition;
    bool doNotOptimizeTargetIntoConditionClosure;

	AFATransition(AFAState target)
		: target(target), condition(nullptr), doNotOptimizeTargetIntoConditionClosure(false) { }
	AFATransition(AFAState target, const std::shared_ptr<ConditionType>& condition)
		: target(target), condition(condition), doNotOptimizeTargetIntoConditionClosure(false) { }
    AFATransition(AFAState target, const std::shared_ptr<ConditionType>& condition, bool doNotOptimizeTargetIntoConditionClosure)
        : target(target), condition(condition), doNotOptimizeTargetIntoConditionClosure(doNotOptimizeTargetIntoConditionClosure) { }

	virtual bool equals(const AFATransition& transition);
};

template<class ConditionType>
inline bool AFATransition<ConditionType>::equals(const AFATransition& transition) {
    if (doNotOptimizeTargetIntoConditionClosure) {
        return false;
    }

    return transition.target == this->target && transition.condition.equals(this->condition);
}

template <class TransitionType>
class AFAStateObject {
public:
	std::list<TransitionType> transitions;

    virtual const AFAStateObject<TransitionType>& operator+=(const AFAStateObject<TransitionType>& rhs);
};

template<class TransitionType>
inline const AFAStateObject<TransitionType>& AFAStateObject<TransitionType>::operator+=(const AFAStateObject<TransitionType>& rhs) {
    this->transitions.insert(this->transitions.cend(), rhs.transitions.cbegin(), rhs.transitions.cend());
}

template <class StateObjectType, class TransitionType>
class AFA {
public:
	std::set<AFAState> finalStates;
	std::vector<StateObjectType> states; // 0th element of this vector is by default the initial state

	AFA();

	void orAFA(const AFA& rhs, bool preventConditionClosureOptimisation = false);
	void andAFA(const AFA& rhs, bool preventConditionClosureOptimisation = false);

	AFAState addState();
	void addTransition(AFAState state, const TransitionType& transition);
    TransitionType& addEmptyTransition(AFAState state, AFAState target);
};

template<class StateObjectType, class ConditionType>
inline AFA<StateObjectType, ConditionType>::AFA() : finalStates(), states() {
	states.emplace_back();
}

template<class StateObjectType, class TransitionType>
inline void AFA<StateObjectType, TransitionType>::orAFA(const AFA& rhs, bool preventConditionClosureOptimisation) {
    AFAState stateIndexShift = this->states.size() - 1; //-1 as we are skipping the rhs 0th state

    // first, transform the state indices of the states referenced as targets of rhs transitions
    auto rhsStateCopy = rhs.states;
    auto scit = rhsStateCopy.begin();
    // handle the 0th state
    for (auto& transition : scit->transitions) {
        transition.target += stateIndexShift;
        transition.doNotOptimizeTargetIntoConditionClosure |= preventConditionClosureOptimisation;
    }
    ++scit;
    // handle all other states
    for (; scit != rhsStateCopy.end(); ++scit) {
        for (auto& transition : scit->transitions) {
            transition.target += stateIndexShift;
        }
    }

    // merge the 0th rhs state into the this->0th state
    scit = rhsStateCopy.begin();
    auto& this0thState = this->states[0];
    this0thState += *scit;
    ++scit;

    // add all the other rhs states to this->states
    this->states.insert(this->states.cend(), scit, rhsStateCopy.end());

    // handle the translation of finality of states
    std::set<AFAState> finalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        if (finalState == 0) {
            finalStatesCopy.insert(0);
        } else {
            finalStatesCopy.insert(stateIndexShift + finalState);
        }
    }
    finalStates.insert(finalStatesCopy.begin(), finalStatesCopy.end());
}

template<class StateObjectType, class TransitionType>
inline void AFA<StateObjectType, TransitionType>::andAFA(const AFA& rhs, bool preventConditionClosureOptimisation) {
    AFAState stateIndexShift = this->states.size() - 1; //-1 as we are skipping the rhs 0th state

    // first, transform the state indices of the states referenced as targets of rhs transitions
    auto rhsStateCopy = rhs.states;
    auto scit = rhsStateCopy.begin();
    // handle the 0th state
    for (auto& transition : scit->transitions) {
        transition.target += stateIndexShift;
        transition.doNotOptimizeTargetIntoConditionClosure |= preventConditionClosureOptimisation;
    }
    ++scit;
    // handle all other states
    for (; scit != rhsStateCopy.end(); ++scit) {
        for (auto& transition : scit->transitions) {
            transition.target += stateIndexShift;
        }
    }

    // merge the 0th rhs state into every final state
    for (const AFAState finalState : finalStates) {
        auto& finalStateObject = this->states[finalState];
        scit = rhsStateCopy.begin();
        finalStateObject += *scit;
    }
    ++scit;

    // add all the other rhs states to this->states
    this->states.insert(this->states.cend(), scit, rhsStateCopy.end());

    // build the set of new final states and set it to be such
    std::set<AFAState> rhsFinalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        rhsFinalStatesCopy.insert(stateIndexShift + finalState);
    }
    this->finalStates = rhsFinalStatesCopy;
}

template<class StateObjectType, class TransitionType>
inline AFAState AFA<StateObjectType, TransitionType>::addState() {
	auto ret = (AFAState)states.size();
	states.emplace_back();
	return ret;
}

template<class StateObjectType, class TransitionType>
inline void AFA<StateObjectType, TransitionType>::addTransition(AFAState state, const TransitionType& transition) {
	states[state].transitions.push_back(transition);
}

template<class StateObjectType, class TransitionType>
inline TransitionType& AFA<StateObjectType, TransitionType>::addEmptyTransition(AFAState state, AFAState target) {
	addTransition(state, TransitionType(target, std::make_shared<EmptyAFACondition>()));
	return states[state].transitions.back();
}


