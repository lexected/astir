#include "NFA.h"

#include <stack>
#include "Exception.h"

NFA::NFA()
    : finalStates(), states() {
    states.emplace_back();
}

void NFA::operator|=(const NFA& rhs) {
    State stateIndexShift = this->states.size();
    auto stateCopy = rhs.states;
    for (auto& state : stateCopy) {
        for (auto& transition : state.transitions) {
            transition.target += stateIndexShift;
        }
    }

    std::set<State> finalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        finalStatesCopy.insert(stateIndexShift+finalState);
    }

    states.insert(states.end(), stateCopy.begin(), stateCopy.end());
    finalStates.insert(finalStatesCopy.begin(), finalStatesCopy.end());

    addEmptyTransition(0, stateIndexShift);
}

void NFA::operator&=(const NFA& rhs) {
    State rhsStartState = this->states.size();
    auto stateCopy = rhs.states;
    for (auto& state : stateCopy) {
        for (auto& transition : state.transitions) {
            transition.target += rhsStartState;
        }
    }

    std::set<State> finalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        finalStatesCopy.insert(rhsStartState + finalState);
    }

    states.insert(states.end(), stateCopy.begin(), stateCopy.end());
    for (const auto& finalState : this->finalStates) {
        addEmptyTransition(finalState, rhsStartState);
    }
    this->finalStates = finalStatesCopy;
}

State NFA::addState() {
    auto ret = (State)states.size();
    states.emplace_back();
    return ret;
}

void NFA::addTransition(State state, const Transition& transition) {
    states[state].transitions.push_back(transition);
}

Transition& NFA::addEmptyTransition(State state, State target) {
    addTransition(state, Transition(target, nullptr));
    return states[state].transitions.back();
}

State NFA::concentrateFinalStates() {
    auto newFinalState = addState();
    for (auto finalState : finalStates) {
        addEmptyTransition(finalState, newFinalState);
    }
    finalStates.clear();
    finalStates.insert(newFinalState);

    return newFinalState;
}
