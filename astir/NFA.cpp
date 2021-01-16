#include "NFA.h"

#include <stack>
#include <algorithm>
#include <iterator>

#include "Exception.h"
#include "SyntacticTree.h"
#include "NFAAction.h"

#include "SemanticAnalysisException.h"

NFA::NFA(const AFA<NFAStateObject, NFATag>& rhs)
    : AFA<NFAStateObject, NFATag>(rhs) {}

NFA::NFA(AFA<NFAStateObject, NFATag>&& rhs)
    : AFA<NFAStateObject, NFATag>(rhs) { }

void NFA::orNFA(const NFA& rhs, bool preventSymbolClosureOptimisation) {
    this->AFA<NFAStateObject, NFATag>::orAFA(rhs, preventSymbolClosureOptimisation);

    // finally, merge in contexts registered in the rhs NFA
    mergeInContexts(rhs);
}

void NFA::andNFA(const NFA& rhs, bool preventSymbolClosureOptimisation) {
    this->AFA<NFAStateObject, NFATag>::andAFA(rhs, preventSymbolClosureOptimisation);

    // finally, merge in contexts registered in the rhs NFA
    mergeInContexts(rhs);
}

void NFA::operator|=(const NFA& rhs) {
    this->orNFA(rhs, false);
}

void NFA::operator&=(const NFA& rhs) {
    this->andNFA(rhs, false);
}

Transition& NFA::addEmptyTransition(AFAState state, AFAState target) {
    return this->AFA<NFAStateObject, NFATag>::addEmptyTransition(state, target);
}

Transition& NFA::addEmptyTransition(AFAState state, AFAState target, const NFAActionRegister& ar) {
    const Transition transitionObject(target, std::make_shared<SymbolGroup>(), ar);
    return addTransition(state, transitionObject);
}

AFAState NFA::concentrateFinalStates() {
    return concentrateFinalStates(NFAActionRegister());
}

AFAState NFA::concentrateFinalStates(const NFAActionRegister& actions) {
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

    std::set<AFAState> newFinalStates;
    for (AFAState fs : finalStates) {
        AFAState newState = addState();
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
    if (this->finalStates.count(0) > 0) {
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

    NFA base = this->AFA<NFAStateObject, NFATag>::buildPseudoDFA();

    base.mergeInContexts(*this);

    return base;
}

std::list<Transition> NFA::calculateTransitions(const std::set<AFAState>& states) const {
    std::list<Transition> transitionsUsed;
    for (AFAState state : states) {
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

bool Transition::equals(const AFATransition<SymbolGroup>& rhs) const {
    return this->AFATransition<SymbolGroup>::equals(rhs);
    // there is no reference to actions here - if both the conditions amd targets are identical, we can optimize by merging the register files
}

bool Transition::canBeMerged() const {
    return this->AFATransition<SymbolGroup>::canBeMerged() && !(this->actions.size() > 0);
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

const NFAStateObject& NFAStateObject::operator+=(const NFAStateObject& rhs) {
    this->AFAStateObject<Transition>::operator+=(rhs);
    
    copyPayloadIn(rhs);

    return *this;
}

void NFAStateObject::copyPayloadIn(const NFAStateObject& rhs) {
    this->actions += rhs.actions;
}

void NFAStateObject::copyPayloadIn(const Transition& rhs) {
    this->actions += rhs.actions;
}

void NFAStateObject::copyPayloadOut(Transition& rhs) const {
    rhs.actions += actions;
}

const AFAStateObject<Transition>& NFAStateObject::operator+=(const AFAStateObject<Transition>& rhs) {
    return this->AFAStateObject<Transition>::operator+=(rhs);
}

void NFAStateObject::copyPayloadIn(const AFAStateObject<Transition>& rhs) {
    this->AFAStateObject<Transition>::copyPayloadIn(rhs);
}
