#include "NFA.h"

#include <stack>
#include <algorithm>
#include <iterator>

#include "Exception.h"
#include "SemanticTree.h"
#include "NFAAction.h"

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
        transition.doNotOptimizeTargetIntoSymbolClosure = preventSymbolClosureOptimisation;
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
        transition.doNotOptimizeTargetIntoSymbolClosure = preventSymbolClosureOptimisation;
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
    for (Transition& transition : this->states[0].transitions) {
        transition.actions += actions;
    }

    // now this may sound like cheating but I promise you it is not
    // in fact, it makes perfect sense - in the specific case when we might be done before we ever consume anything,
    // we want to make sure that the initial actions are performed
    if (this->finalStates.contains(0)) {
        this->states[0].actions += actions;
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
        for (; iit != transitions.end(); ++iit) {
            if (iit->equals(*it)) {
                break;
            } else if (!iit->alignedSymbolWise(*it)) {
                auto newTransitions = it->disjoinFrom(*iit);
                transitions.insert(transitions.end(), newTransitions.cbegin(), newTransitions.cend());
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

std::list<std::shared_ptr<LiteralSymbolGroup>> NFA::makeComplementSymbolGroups(const std::list<std::shared_ptr<SymbolGroup>>& symbolGroups) {
    return std::list<std::shared_ptr<LiteralSymbolGroup>>();
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

bool LiteralSymbolGroup::equals(const SymbolGroup* rhs) const {
    const LiteralSymbolGroup* rhsCast = dynamic_cast<const LiteralSymbolGroup*>(rhs);
    if (rhsCast == nullptr) {
        return false;
    } else {
        return this->rangeStart == rhsCast->rangeStart && this->rangeEnd == rhsCast->rangeEnd;
    }
}

bool LiteralSymbolGroup::disjoint(const SymbolGroup* rhs) const {
    const LiteralSymbolGroup* rhsCast = dynamic_cast<const LiteralSymbolGroup*>(rhs);
    if (rhsCast == nullptr) {
        return true;
    } else {
        return this->rangeStart > rhsCast->rangeEnd || rhsCast->rangeStart > this->rangeEnd;
    }
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >> LiteralSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhsUncast) {
    if (rhsUncast->equals(this)) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
    } else if (this->disjoint(rhsUncast.get())) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >>({ { rhsUncast, true } });
    }

    std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >> ret;
    const LiteralSymbolGroup* rhs = dynamic_cast<LiteralSymbolGroup*>(rhsUncast.get());
    if (this->rangeEnd >= rhs->rangeStart) {
        ComputationCharType mid_beg = std::max(this->rangeStart, rhs->rangeStart);
        ComputationCharType mid_end = std::min(this->rangeEnd, rhs->rangeEnd);
        // thanks to the first condition we already have mid_beg <= mid_end

        ComputationCharType bottom_beg = std::min(this->rangeStart, rhs->rangeStart);
        ComputationCharType bottom_end = (short)mid_beg - 1;

        ComputationCharType top_beg = std::min(this->rangeEnd, rhs->rangeEnd) + 1;
        ComputationCharType top_end = std::max(this->rangeEnd, rhs->rangeEnd);

        if (bottom_beg <= bottom_end) {
            ret.emplace_back(std::make_shared<LiteralSymbolGroup>((CharType)bottom_beg, (CharType)bottom_end), bottom_beg != this->rangeStart);
        }

        this->rangeStart = (CharType)mid_beg;
        this->rangeEnd = (CharType)mid_end;
        ret.emplace_back(std::make_shared<LiteralSymbolGroup>((CharType)mid_beg, (CharType)mid_end), true);

        if (top_beg <= top_end) {
            ret.emplace_back(std::make_shared<LiteralSymbolGroup>((CharType)top_beg, (CharType)top_end), top_beg == this->rangeEnd);
        }
    }

    return ret;
}

std::shared_ptr<std::list<SymbolIndex>> LiteralSymbolGroup::retrieveSymbolIndices() const {
    if (m_symbolIndicesFlyweight->empty()) {
        for (ComputationCharType it = rangeStart; it <= (ComputationCharType)rangeEnd; ++it) {
            m_symbolIndicesFlyweight->push_back(it);
        }
    }
    
    return m_symbolIndicesFlyweight;
}

bool EmptySymbolGroup::equals(const SymbolGroup* rhs) const {
    return dynamic_cast<const EmptySymbolGroup*>(rhs) != nullptr;
}

bool EmptySymbolGroup::disjoint(const SymbolGroup* rhs) const {
    return dynamic_cast<const EmptySymbolGroup*>(rhs) == nullptr;
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> EmptySymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) {
    if (rhs->equals(this)) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
    }

    if (dynamic_cast<EmptySymbolGroup*>(rhs.get()) == nullptr) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >>({ { rhs, true } });
    } else {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
    }
}

std::shared_ptr<std::list<SymbolIndex>> EmptySymbolGroup::retrieveSymbolIndices() const {
    return std::shared_ptr<std::list<SymbolIndex>>();
}

bool TerminalSymbolGroup::equals(const SymbolGroup* rhs) const {
    const TerminalSymbolGroup* rhsCast = dynamic_cast<const TerminalSymbolGroup*>(rhs);
    if (rhsCast == nullptr) {
        return false;
    } else {
        return this->referencedProductions == rhsCast->referencedProductions;
    }
}

bool TerminalSymbolGroup::disjoint(const SymbolGroup* rhs) const {
    const TerminalSymbolGroup* rhsCast = dynamic_cast<const TerminalSymbolGroup*>(rhs);
    if (rhsCast == nullptr) {
        return true;
    } else {
        for (const auto referencedComponentPtr : referencedProductions) {
            auto fit = std::find_if(rhsCast->referencedProductions.cbegin(), rhsCast->referencedProductions.cend(), [referencedComponentPtr](const MachineComponent* rhsComponentPtr) {
                return referencedComponentPtr->name == rhsComponentPtr->name;
                });
            if (fit != rhsCast->referencedProductions.cend()) {
                return false;
            }
        }

        return true;
    }
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> TerminalSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhsUncast) {
    if (rhsUncast->equals(this)) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
    }

    const TerminalSymbolGroup* rhs = dynamic_cast<const TerminalSymbolGroup*>(rhsUncast.get());
    if (rhs == nullptr) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhsUncast, true } });
    }

    std::list<const Production*> sharedComponents;
    std::list<const Production*> excludedComponents;
    for (auto it = referencedProductions.begin(); it != referencedProductions.end(); ) {
        auto fit = std::find_if(rhs->referencedProductions.cbegin(), rhs->referencedProductions.cend(), [it](const Production* rhsComponentPtr) {
            return (*it)->name == rhsComponentPtr->name;
            });
        if (fit != rhs->referencedProductions.cend()) {
            sharedComponents.push_back(*fit);
            it = referencedProductions.erase(it);
        } else {
            excludedComponents.push_back(*fit);
            ++it;
        }
    }

    if (sharedComponents.empty()) {
        return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhsUncast, true } });
    } else {
        std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> ret;
        ret.emplace_back(std::make_shared<TerminalSymbolGroup>(sharedComponents), false);
        ret.emplace_back(std::make_shared<TerminalSymbolGroup>(sharedComponents), true);
        ret.emplace_back(std::make_shared<TerminalSymbolGroup>(excludedComponents), true);
        
        return ret;
    }
}

std::shared_ptr<std::list<SymbolIndex>> TerminalSymbolGroup::retrieveSymbolIndices() const {
    if (m_symbolIndicesFlyweight->empty()) {
        for (const Production* referencedComponentPtr : referencedProductions) {
            m_symbolIndicesFlyweight->push_back(referencedComponentPtr->typeIndex);
        }
    }

    return m_symbolIndicesFlyweight;
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
