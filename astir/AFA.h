/* THE ABSTRACT FINITE AUTOMATON */
#pragma once

#include <set>
#include <list>
#include <vector>
#include <memory>
#include <deque>
#include <algorithm>
#include <iterator>
#include <stack>

#include "AFACondition.h"

using AFAState = size_t;

template <class ConditionType>
class AFATransition {
public:
    typedef typename ConditionType CondType;

	AFAState target;
	std::shared_ptr<ConditionType> condition;
    bool doNotOptimizeTargetIntoConditionClosure;

	AFATransition(AFAState target)
		: target(target), condition(nullptr), doNotOptimizeTargetIntoConditionClosure(false) { }
	AFATransition(AFAState target, const std::shared_ptr<ConditionType>& condition)
		: target(target), condition(condition), doNotOptimizeTargetIntoConditionClosure(false) { }
    AFATransition(AFAState target, const std::shared_ptr<ConditionType>& condition, bool doNotOptimizeTargetIntoConditionClosure)
        : target(target), condition(condition), doNotOptimizeTargetIntoConditionClosure(doNotOptimizeTargetIntoConditionClosure) { }

	virtual bool equals(const AFATransition<ConditionType>& transition) const;
    virtual bool canBeMerged() const;
};

template<class ConditionType>
inline bool AFATransition<ConditionType>::equals(const AFATransition<ConditionType>& transition) const {
    if (doNotOptimizeTargetIntoConditionClosure) {
        return false;
    }

    return transition.target == this->target && transition.condition->equals(this->condition);
}

template<class ConditionType>
inline bool AFATransition<ConditionType>::canBeMerged() const {
    return !doNotOptimizeTargetIntoConditionClosure;
}

template <class TransitionType>
class AFAStateObject {
public:
    typedef typename TransitionType TransType;
    typedef typename TransitionType::CondType CondType;

	std::list<TransitionType> transitions;

    virtual const AFAStateObject<TransitionType>& operator+=(const AFAStateObject<TransitionType>& rhs);
    virtual void copyPayloadIn(const AFAStateObject<TransitionType>& rhs) { }
    virtual void copyPayloadIn(const TransitionType& rhs) { }
    virtual void copyPayloadOut(TransitionType& rhs) const { }
};

template<class TransitionType>
inline const AFAStateObject<TransitionType>& AFAStateObject<TransitionType>::operator+=(const AFAStateObject<TransitionType>& rhs) {
    this->transitions.insert(this->transitions.cend(), rhs.transitions.cbegin(), rhs.transitions.cend());
    return *this;
}

template <class StateObjectType>
class AFA {
public:
    typedef typename StateObjectType::TransType TransType;
    typedef typename TransType::CondType CondType;

	std::set<AFAState> finalStates;
	std::vector<StateObjectType> states; // 0th element of this vector is by default the initial state

	AFA();

	void orAFA(const AFA& rhs, bool preventConditionClosureOptimisation = false);
	void andAFA(const AFA& rhs, bool preventConditionClosureOptimisation = false);

	AFAState addState();
    TransType& addTransition(AFAState state, const TransType& transition);
    TransType& addEmptyTransition(AFAState state, AFAState target);

    AFA<StateObjectType> buildPseudoDFA() const;

private:
    struct InterimDFAState {
        std::set<AFAState> nfaStates;
        bool marked;
        StateObjectType statePayload;

        InterimDFAState()
            : marked(false) { }
        InterimDFAState(const std::set<AFAState>& nfaStates, const StateObjectType& statePayload)
            : nfaStates(nfaStates), marked(false), statePayload(statePayload) { }
    };

    struct ConditionClosure {
        std::shared_ptr<CondType> condition;
        std::set<AFAState> states;
        StateObjectType statePayload;

        ConditionClosure()
            : condition(nullptr) { }
        ConditionClosure(const std::shared_ptr<CondType>& condition, const std::set<AFAState>& states)
            : condition(condition), states(states) { }
        ConditionClosure(const std::shared_ptr<CondType>& condition, const std::set<AFAState>& states, const StateObjectType& statePayload)
            : condition(condition), states(states), statePayload(statePayload) { }
        ConditionClosure(const std::shared_ptr<CondType>& condition, const std::set<AFAState>& states, const TransType& transPayload)
            : condition(condition), states(states), statePayload() {
            statePayload.copyPayloadIn(transPayload);
        }

        TransType makeTransition(AFAState target) const;
    };

    InterimDFAState calculateEpsilonClosure(const std::set<AFAState>& states) const;
    std::list<ConditionClosure> calculateConditionClosures(const std::list<TransType>& transitions) const;
    virtual std::list<TransType> calculateTransitions(const std::set<AFAState>& states) const;


    AFAState findUnmarkedState(const std::deque<InterimDFAState>& stateMap) const;
    AFAState findStateByNFAStateSet(const std::deque<InterimDFAState>& stateMap, const std::set<AFAState>& nfaSet) const;
};

template<class StateObjectType>
inline AFA<StateObjectType>::AFA() : finalStates(), states() {
	states.emplace_back();
}

template<class StateObjectType>
inline void AFA<StateObjectType>::orAFA(const AFA& rhs, bool preventConditionClosureOptimisation) {
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

template<class StateObjectType>
inline void AFA<StateObjectType>::andAFA(const AFA& rhs, bool preventConditionClosureOptimisation) {
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

template<class StateObjectType>
inline AFAState AFA<StateObjectType>::addState() {
	auto ret = (AFAState)states.size();
	states.emplace_back();
	return ret;
}

template<class StateObjectType>
inline typename AFA<StateObjectType>::TransType& AFA<StateObjectType>::addTransition(AFAState state, const typename TransType& transition) {
    auto& stateObject = states[state];
    stateObject.transitions.push_back(transition);
    return stateObject.transitions.back();
}

template<class StateObjectType>
inline typename AFA<StateObjectType>::TransType& AFA<StateObjectType>::addEmptyTransition(AFAState state, AFAState target) {
    return addTransition(state, typename AFA<StateObjectType>::TransType(target, std::make_shared<CondType>()));
}

template<class StateObjectType>
inline AFA<StateObjectType> AFA<StateObjectType>::buildPseudoDFA() const {
    // the idea of the conversion algorithm is quite simple,
    // but what makes the whole conversion rather challenging
    // is the bookkeeping necessary to know what state payload
    // is to be carried around

    AFA<StateObjectType> base;
    std::deque<InterimDFAState> stateMap; // must be a deque, not a vector!

    InterimDFAState initialState = calculateEpsilonClosure(std::set<AFAState>({ (AFAState)0 }));
    base.states[0].copyPayloadIn(initialState.statePayload);
    stateMap.push_back(initialState);

    AFAState unmarkedStateDfaState;
    while ((unmarkedStateDfaState = findUnmarkedState(stateMap)) != stateMap.size()) {
        auto& stateObject = stateMap[unmarkedStateDfaState];
        stateObject.marked = true;

        auto transitions = calculateTransitions(stateObject.nfaStates);
        auto conditionClosures = calculateConditionClosures(transitions);
        for (const auto& conditionClosure : conditionClosures) {
            const auto& advancedStateSet = conditionClosure.states;
            auto epsilonClosureInterimDFAState = calculateEpsilonClosure(advancedStateSet);
            AFAState theCorrespondingDFAStateIndex = findStateByNFAStateSet(stateMap, epsilonClosureInterimDFAState.nfaStates);
            if (theCorrespondingDFAStateIndex == base.states.size()) {
                theCorrespondingDFAStateIndex = base.addState();
                base.states[theCorrespondingDFAStateIndex].copyPayloadIn(epsilonClosureInterimDFAState.statePayload);
                stateMap.emplace_back(epsilonClosureInterimDFAState);

                std::set<AFAState> intersectionOfNFAStates;
                std::set_intersection(epsilonClosureInterimDFAState.nfaStates.cbegin(), epsilonClosureInterimDFAState.nfaStates.cend(), finalStates.cbegin(), finalStates.cend(), std::inserter(intersectionOfNFAStates, intersectionOfNFAStates.begin()));
                if (intersectionOfNFAStates.size() > 0) {
                    base.finalStates.insert(theCorrespondingDFAStateIndex);
                }
            }

            base.addTransition(unmarkedStateDfaState, conditionClosure.makeTransition(theCorrespondingDFAStateIndex));
        }
    }

    return base;
}

template<class StateObjectType>
inline typename AFA<StateObjectType>::InterimDFAState AFA<StateObjectType>::calculateEpsilonClosure(const std::set<AFAState>& states) const {
    StateObjectType payloadAccumulator;
    std::stack<AFAState> statesToCheck;
    for (const auto& state : states) {
        statesToCheck.push(state);
    }

    std::set<AFAState> ret(states);
    while (!statesToCheck.empty()) {
        AFAState currentState = statesToCheck.top();
        statesToCheck.pop();

        const auto& stateObject = this->states[currentState];
        payloadAccumulator.copyPayloadIn(stateObject);
        for (const auto& transition : stateObject.transitions) {
            if (!transition.condition->isEmpty()) {
                continue;
            }

            payloadAccumulator.copyPayloadIn(transition);

            std::pair<std::set<AFAState>::iterator, bool> insertionOutcome = ret.insert(transition.target);
            if (insertionOutcome.second) {
                statesToCheck.push(transition.target);
            }
        }
    }

    return AFA::InterimDFAState(ret, payloadAccumulator);
}

template<class StateObjectType>
inline std::list<typename AFA<StateObjectType>::ConditionClosure> AFA<StateObjectType>::calculateConditionClosures(const std::list<TransType>& transitions) const {
    std::list<ConditionClosure> generalClosures;
    std::list<ConditionClosure> individualClosures;
    for (const auto& transition : transitions) {
        if (transition.condition->isEmpty()) {
            continue;
        }

        if (!transition.canBeMerged()) {
            individualClosures.emplace_back(transition.condition, std::set<AFAState> { transition.target }, transition);
        } else {
            auto fit = std::find_if(generalClosures.begin(), generalClosures.end(), [&transition](const ConditionClosure& cc) {
                return cc.condition->equals(transition.condition.get());
                });

            if (fit == generalClosures.end()) {
                generalClosures.emplace_back(transition.condition, std::set<AFAState> { transition.target }, transition);
            } else {
                fit->states.insert(transition.target);
                fit->statePayload.copyPayloadIn(transition);
            }
        }
    }

    individualClosures.insert(individualClosures.end(), generalClosures.cbegin(), generalClosures.cend());

    return individualClosures;
}

template<class StateObjectType>
inline std::list<typename AFA<StateObjectType>::TransType> AFA<StateObjectType>::calculateTransitions(const std::set<AFAState>& states) const {
    std::list<TransType> transitionsUsed;
    for (AFAState state : states) {
        const auto& stateObject = this->states[state];
        for (const auto& transition : stateObject.transitions) {
            auto fit = std::find_if(transitionsUsed.cbegin(), transitionsUsed.cend(), [&transition](const auto& t) {
                return t.equals(transition);
                });
            if (fit == transitionsUsed.cend()) {
                transitionsUsed.push_back(transition);
            }
        }
    }

    return transitionsUsed;
}

template<class StateObjectType>
inline AFAState AFA<StateObjectType>::findUnmarkedState(const std::deque<InterimDFAState>& stateMap) const {
    AFAState index;
    for (index = 0; index < stateMap.size(); ++index) {
        if (!stateMap[index].marked) {
            break;
        }
    }

    return index;
}

template<class StateObjectType>
inline AFAState AFA<StateObjectType>::findStateByNFAStateSet(const std::deque<InterimDFAState>& stateMap, const std::set<AFAState>& nfaSet) const {
    AFAState index;
    for (index = 0; index < stateMap.size(); ++index) {
        if (stateMap[index].nfaStates == nfaSet) {
            break;
        }
    }

    return index;
}

template<class StateObjectType>
inline typename AFA<StateObjectType>::TransType AFA<StateObjectType>::ConditionClosure::makeTransition(AFAState target) const {
    auto ret = TransType(target, condition);
    statePayload.copyPayloadOut(ret);
    return ret;
}
