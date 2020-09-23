#include "NFA.h"

#include <stack>
#include <algorithm>
#include <iterator>

#include "Exception.h"
#include "SyntacticTree.h"
#include "NFAAction.h"

#include "SemanticAnalysisException.h"

NFA::NFA()
    : finalStates(), states() {
    states.emplace_back();
}

void NFA::orNFA(const NFA& rhs, bool preventSymbolClosureOptimisation) {
    State stateIndexShift = this->states.size() - 1; //-1 as we are skipping the rhs 0th state

    // first, transform the state indices of the states referenced as targets of rhs transitions
    auto rhsStateCopy = rhs.states;
    auto scit = rhsStateCopy.begin();
    // handle the 0th state
    for (auto& transition : scit->transitions) {
        transition.target += stateIndexShift;
        transition.doNotOptimizeTargetIntoSymbolClosure |= preventSymbolClosureOptimisation;
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
    this0thState.actions += scit->actions;
    this0thState.transitions.insert(this0thState.transitions.cend(), scit->transitions.cbegin(), scit->transitions.cend());
    ++scit;

    // add all the other rhs states to this->states
    this->states.insert(this->states.cend(), scit, rhsStateCopy.end());

    // handle the translation of finality of states
    std::set<State> finalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        if (finalState == 0) {
            finalStatesCopy.insert(0);
        } else {
            finalStatesCopy.insert(stateIndexShift + finalState);
        }
    }
    finalStates.insert(finalStatesCopy.begin(), finalStatesCopy.end());

    // finally, merge in contexts registered in the rhs NFA
    mergeInContexts(rhs);
}

void NFA::andNFA(const NFA& rhs, bool preventSymbolClosureOptimisation) {
    State stateIndexShift = this->states.size() - 1; //-1 as we are skipping the rhs 0th state

    // first, transform the state indices of the states referenced as targets of rhs transitions
    auto rhsStateCopy = rhs.states;
    auto scit = rhsStateCopy.begin();
    // handle the 0th state
    for (auto& transition : scit->transitions) {
        transition.target += stateIndexShift;
        transition.doNotOptimizeTargetIntoSymbolClosure |= preventSymbolClosureOptimisation;
    }
    ++scit;
    // handle all other states
    for (; scit != rhsStateCopy.end(); ++scit) {
        for (auto& transition : scit->transitions) {
            transition.target += stateIndexShift;
        }
    }

    // merge the 0th rhs state into every final state
    for (const State finalState : finalStates) {
        auto& finalStateObject = this->states[finalState];
        scit = rhsStateCopy.begin();
        finalStateObject.actions += scit->actions;
        finalStateObject.transitions.insert(finalStateObject.transitions.cend(), scit->transitions.cbegin(), scit->transitions.cend());
    }
    ++scit;

    // add all the other rhs states to this->states
    this->states.insert(this->states.cend(), scit, rhsStateCopy.end());

    // build the set of new final states and set it to be such
    std::set<State> rhsFinalStatesCopy;
    for (const auto& finalState : rhs.finalStates) {
        rhsFinalStatesCopy.insert(stateIndexShift + finalState);
    }
    this->finalStates = rhsFinalStatesCopy;

    // finally, merge in contexts registered in the rhs NFA
    mergeInContexts(rhs);
}

void NFA::operator|=(const NFA& rhs) {
    this->orNFA(rhs, false);
}

void NFA::operator&=(const NFA& rhs) {
    this->andNFA(rhs, false);
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
    addTransition(state, Transition(target, std::make_shared<EmptySymbolGroup>()));
    return states[state].transitions.back();
}

Transition& NFA::addEmptyTransition(State state, State target, const NFAActionRegister& ar) {
    addTransition(state, Transition(target, std::make_shared<EmptySymbolGroup>(), ar));
    return states[state].transitions.back();
}

State NFA::concentrateFinalStates() {
    return concentrateFinalStates(NFAActionRegister());
}

State NFA::concentrateFinalStates(const NFAActionRegister& actions) {
    if (finalStates.size() == 1 && actions.empty()) {
        return *finalStates.cbegin();
    }

    auto newFinalState = addState();
    for (auto finalState : finalStates) {
        addEmptyTransition(finalState, newFinalState, actions);
    }
    finalStates.clear();
    finalStates.insert(newFinalState);

    return newFinalState;
}

void NFA::addFinalActions(const NFAActionRegister& actions) {
    if (actions.empty()) {
        return;
    }

    std::set<State> newFinalStates;
    for (State fs : finalStates) {
        State newState = addState();
        addEmptyTransition(fs, newState, actions);
        newFinalStates.insert(newState);
    }

    finalStates = newFinalStates;
}

void NFA::addInitialActions(const NFAActionRegister& actions) {
    // should probably be called 'prependInitialActions' rather than 'addInitialActions', 'cause the "prepension" is crucial
    for (Transition& transition : this->states[0].transitions) {
        auto newActions = actions;
        newActions += transition.actions;
        transition.actions = newActions;
    }

    // now this may sound like cheating but I promise you it is not
    // in fact, it makes perfect sense - in the specific case when we might be done before we ever consume anything,
    // we want to make sure that the initial actions are performed
    if (this->finalStates.contains(0)) {
        auto newActions = actions;
        newActions += this->states[0].actions;
        this->states[0].actions = newActions;
    }
}

void NFA::registerContext(const std::string& parentName, const std::string& name) {
    auto it = std::find(contexts.begin(), contexts.end(), std::pair<std::string, std::string>(parentName, name));
    if (it != contexts.end()) {
        throw SemanticAnalysisException("Re-registering context '" + name + "' within parent context '" + name + "'");
    }

    contexts.push_back(std::pair<std::string, std::string>(parentName, name));
}

void NFA::mergeInContexts(const NFA& rhs) {
    for (const auto& contextPair : rhs.contexts) {
        registerContext(contextPair.first, contextPair.second);
    }
}

NFA NFA::buildPseudoDFA() const {
    // the idea of the conversion algorithm is quite simple,
    // but what makes the whole conversion rather challenging
    // is the bookkeeping necessary to know what actions are
    // to be executed when an accepting state is reached

    NFA base;
    std::deque<DFAState> stateMap; // must be a deque, not a vector!

    DFAState initialState = calculateEpsilonClosure(std::set<State>({ (State)0 }));
    base.states[0].actions = initialState.actions;
    stateMap.push_back(initialState);

    State unmarkedStateDfaState;
    while ((unmarkedStateDfaState = findUnmarkedState(stateMap)) != stateMap.size()) {
        auto& stateObject = stateMap[unmarkedStateDfaState];
        stateObject.marked = true;

        auto transitions = calculateTransitions(stateObject.nfaStates);
        auto symbolClosures = calculateSymbolClosures(transitions);
        for (const auto& symbolClosure : symbolClosures) {
            const auto& advancedStateSet = symbolClosure.states;
            auto epsilonClosureDFAState = calculateEpsilonClosure(advancedStateSet);
            State theCorrespondingDFAStateIndex = findStateByNFAStateSet(stateMap, epsilonClosureDFAState.nfaStates);
            if (theCorrespondingDFAStateIndex == base.states.size()) {
                theCorrespondingDFAStateIndex = base.addState();
                base.states[theCorrespondingDFAStateIndex].actions = epsilonClosureDFAState.actions; 
                stateMap.emplace_back(epsilonClosureDFAState);
                
                std::set<State> intersectionOfNFAStates;
                std::set_intersection(epsilonClosureDFAState.nfaStates.cbegin(), epsilonClosureDFAState.nfaStates.cend(), finalStates.cbegin(), finalStates.cend(), std::inserter(intersectionOfNFAStates, intersectionOfNFAStates.begin()));
                if (intersectionOfNFAStates.size() > 0) {
                    base.finalStates.insert(theCorrespondingDFAStateIndex);
                }
            }

            base.addTransition(unmarkedStateDfaState, Transition(theCorrespondingDFAStateIndex, symbolClosure.symbols, symbolClosure.actions));
        }
    }

    base.mergeInContexts(*this);

    return base;
}

NFA::DFAState NFA::calculateEpsilonClosure(const std::set<State>& states) const {
    NFAActionRegister accumulatedActions;
    std::stack<State> statesToCheck;
    for (const auto& state : states) {
        statesToCheck.push(state);
    }

    std::set<State> ret(states);
    while (!statesToCheck.empty()) {
        State currentState = statesToCheck.top();
        statesToCheck.pop();

        const auto& stateObject = this->states[currentState];
        accumulatedActions += stateObject.actions;
        for (const auto& transition : stateObject.transitions) {
            const EmptySymbolGroup* esg = dynamic_cast<const EmptySymbolGroup*>(transition.condition.get());
            if (esg == nullptr) {
                continue;
            }

            accumulatedActions += transition.actions;

            std::pair<std::set<State>::iterator, bool> insertionOutcome = ret.insert(transition.target);
            if (insertionOutcome.second) {
                statesToCheck.push(transition.target);
            }
        }
    }

    return NFA::DFAState(ret, accumulatedActions);
}

std::list<NFA::SymbolClosure> NFA::calculateSymbolClosures(const std::list<Transition>& transitions) const {
    std::list<NFA::SymbolClosure> generalClosures;
    std::list<NFA::SymbolClosure> individualClosures;
    for(const auto& transition : transitions) {
        const EmptySymbolGroup* esg = dynamic_cast<const EmptySymbolGroup*>(transition.condition.get());
        if (esg != nullptr) {
            continue;
        }

        if (transition.doNotOptimizeTargetIntoSymbolClosure || !transition.actions.empty() /* this is vital! */) {
            individualClosures.emplace_back(transition.condition, std::set<State> { transition.target }, transition.actions);
        } else {
            auto fit = std::find_if(generalClosures.begin(), generalClosures.end(), [&transition](const SymbolClosure& sc) {
                return sc.symbols->equals(transition.condition.get());
            });

            if (fit == generalClosures.end()) {
                generalClosures.emplace_back(transition.condition, std::set<State> { transition.target }, transition.actions);
            } else {
                fit->states.insert(transition.target);
                fit->actions += transition.actions;
            }
        }
    }

    individualClosures.insert(individualClosures.end(), generalClosures.cbegin(), generalClosures.cend());

    return individualClosures;
}

std::list<Transition> NFA::calculateTransitions(const std::set<State>& states) const {
    std::list<Transition> transitionsUsed;
    for (State state : states) {
        const auto& stateObject = this->states[state];
        transitionsUsed.insert(transitionsUsed.end(), stateObject.transitions.cbegin(), stateObject.transitions.cend());
    }

    calculateDisjointTransitions(transitionsUsed);

    return transitionsUsed;
}

void NFA::calculateDisjointTransitions(std::list<Transition>& transitions) {
    auto it = transitions.begin();
    while(it != transitions.end()) {
        auto iit = it;
        ++iit;
        for (; iit != transitions.end();) {
            if (iit->equals(*it)) {
                break;
            } else if (!iit->alignedSymbolWise(*it)) {
                auto newTransitions = it->disjoinFrom(*iit);
                transitions.insert(transitions.end(), newTransitions.cbegin(), newTransitions.cend());
                iit = transitions.erase(iit);
            } else {
                ++iit;
            }
        }

        if (iit != transitions.end()) {
            iit->actions += it->actions;
            it = transitions.erase(it);
        } else {
            ++it;
        }
    }
}

std::list<std::shared_ptr<ByteSymbolGroup>> NFA::makeComplementSymbolGroups(const std::list<std::shared_ptr<SymbolGroup>>& symbolGroups) {
    return std::list<std::shared_ptr<ByteSymbolGroup>>();
}
/*
std::list<LiteralSymbolGroup> NFA::negateLiteralSymbolGroups(const std::list<LiteralSymbolGroup>& transitions) {
    std::list<LiteralSymbolGroup> ret;
    ComputationCharType lastEnd = 0;
    for (const auto lsg : transitions) { 
        if (lsg.rangeStart > lastEnd+1) {
            ret.emplace_back(lastEnd+1, lsg.rangeStart - 1);
        }
        lastEnd = lsg.rangeEnd;
    }
    if (lastEnd < 255) {
        ret.emplace_back((CharType)(lastEnd + 1), (CharType)255);
    }

    return ret;
}
*/

State NFA::findUnmarkedState(const std::deque<DFAState>& stateMap) const {
    size_t index;
    for (index = 0; index < stateMap.size(); ++index) {
        if (!stateMap[index].marked) {
            break;
        }
    }

    return index;
}

State NFA::findStateByNFAStateSet(const std::deque<DFAState>& stateMap, const std::set<State>& nfaSet) const {
    size_t index;
    for (index = 0; index < stateMap.size(); ++index) {
        if (stateMap[index].nfaStates == nfaSet) {
            break;
        }
    }

    return index;
}

bool Transition::equals(const Transition& rhs) const {
    if (doNotOptimizeTargetIntoSymbolClosure) {
        return false;
    }

    return target == rhs.target && condition->equals(rhs.condition.get());
    // there is no reference to actions here - if both the conditions amd targets are identical, we can optimize by merging the register files
}

bool Transition::alignedSymbolWise(const Transition& rhs) const {
    return condition->disjoint(rhs.condition.get()) || rhs.condition->equals(this->condition.get());
}

std::list<Transition> Transition::disjoinFrom(const Transition& rhs) {
    std::list<Transition> transitions;

    auto newSymbolGroupPairs = this->condition->disjoinFrom(rhs.condition);
    for (const auto& nsgPair : newSymbolGroupPairs) {
        Transition t(nsgPair.second ? rhs : *this);
        t.condition = nsgPair.first;
        transitions.emplace_back(std::move(t));
    }

    return transitions;
}
