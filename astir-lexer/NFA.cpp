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

    mergeInContexts(rhs);
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

    mergeInContexts(rhs);
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
    addTransition(state, Transition(target, std::make_shared<EmptySymbolGroup>(ar)));
    return states[state].transitions.back();
}

State NFA::concentrateFinalStates() {
    return concentrateFinalStates(NFAActionRegister());
}

State NFA::concentrateFinalStates(const NFAActionRegister& actions) {
    auto newFinalState = addState();
    for (auto finalState : finalStates) {
        addEmptyTransition(finalState, newFinalState, actions);
    }
    finalStates.clear();
    finalStates.insert(newFinalState);

    return newFinalState;
}

void NFA::addFinalActions(const NFAActionRegister& actions) {
    std::set<State> newFinalStates;
    for (State fs : finalStates) {
        State newState = addState();
        addEmptyTransition(fs, newState, actions);
        newFinalStates.insert(newState);
    }

    finalStates = newFinalStates;
}

void NFA::addInitialTransitionActions(const NFAActionRegister& actions) {
    for (Transition& transition : this->states[0].transitions) {
        transition.condition->actions += actions;
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

        auto transitionSymbolPtrs = calculateTransitionSymbols(stateObject.nfaStates);
        for (const auto& transitionSymbolPtr : transitionSymbolPtrs) {
            auto advancedStateSet = calculateSymbolClosure(stateObject.nfaStates, transitionSymbolPtr.get());
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

            base.addTransition(unmarkedStateDfaState, Transition(theCorrespondingDFAStateIndex, transitionSymbolPtr));
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

            accumulatedActions += esg->actions;

            std::pair<std::set<State>::iterator, bool> insertionOutcome = ret.insert(transition.target);
            if (insertionOutcome.second) {
                statesToCheck.push(transition.target);
            }
        }
    }

    return NFA::DFAState(ret, accumulatedActions);
}

std::set<State> NFA::calculateSymbolClosure(const std::set<State>& states, const SymbolGroup* symbolOnTransition) const {
    std::set<State> reachableStates;
    for(const auto& state : states) {
        const auto& stateObject = this->states[state];
        for (const auto& transition : stateObject.transitions) {
            if (transition.condition->contains(symbolOnTransition)) {
                reachableStates.insert(transition.target);
            }
        }
    }

    return reachableStates;
}

std::list<std::shared_ptr<SymbolGroup>> NFA::calculateTransitionSymbols(const std::set<State>& states) const {
    std::list<std::shared_ptr<SymbolGroup>> symbolGroupsUsed;
    for (State state : states) {
        const auto& stateObject = this->states[state];
        for (const auto& transition : stateObject.transitions) {
            const EmptySymbolGroup* esg = dynamic_cast<const EmptySymbolGroup*>(transition.condition.get());
            if(esg == nullptr) { // we ignore only the empty transitions which were (hopefully?) sorted out by the process of calculating epsilon closure
                symbolGroupsUsed.push_back(transition.condition);
            }
        }
    }

    calculateDisjointSymbolGroups(symbolGroupsUsed);

    return symbolGroupsUsed;
}

void NFA::calculateDisjointSymbolGroups(std::list<std::shared_ptr<SymbolGroup>>& symbolGroups) {
    auto it = symbolGroups.begin();
    bool equalAndEquallyActionedTransitionFound = false;
    while(it != symbolGroups.end()) {
        auto iit = it;
        ++iit;
        for (; iit != symbolGroups.end(); ++iit) {
            if ((*iit)->equals(it->get())) {
                if((*iit)->actions == (*it)->actions) {
                    equalAndEquallyActionedTransitionFound = true;
                } else {
                    // in case of two equal transition conditions that only differ in actions associated with them, we leave the two be and
                    continue;
                }
                break;
            } else if (!(*iit)->disjoint(it->get())) {
                equalAndEquallyActionedTransitionFound = false;
                break;
            }
        }

        if (iit != symbolGroups.end()) {
            if (equalAndEquallyActionedTransitionFound) {
                it = symbolGroups.erase(it);
            } else {
                auto newSymbolGroups = (*it)->disjoinFrom(*iit);
                symbolGroups.merge(newSymbolGroups);
            }
        } else {
            ++it;
        }
    }
}

std::list<std::shared_ptr<LiteralSymbolGroup>> NFA::makeComplementSymbolGroups(const std::list<std::shared_ptr<SymbolGroup>>& symbolGroups) {
    return std::list<std::shared_ptr<LiteralSymbolGroup>>();
}
/*
std::list<LiteralSymbolGroup> NFA::negateLiteralSymbolGroups(const std::list<LiteralSymbolGroup>& symbolGroups) {
    std::list<LiteralSymbolGroup> ret;
    ComputationCharType lastEnd = 0;
    for (const auto lsg : symbolGroups) { 
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


bool LiteralSymbolGroup::contains(const SymbolGroup* symbol) const {
    const LiteralSymbolGroup* ls = dynamic_cast<const LiteralSymbolGroup*>(symbol);
    if (ls == nullptr) {
        return false;
    }

    return 
        this->rangeStart <= ls->rangeStart && this->rangeEnd >= ls->rangeEnd
        && this->actions == symbol->actions
        ;
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

std::list<std::shared_ptr<SymbolGroup>> LiteralSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhsUncast) {
    if (this->disjoint(rhsUncast.get())) {
        return std::list<std::shared_ptr<SymbolGroup>>({ rhsUncast });
    }

    std::list<std::shared_ptr<SymbolGroup>> ret;
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
            ret.push_back(std::make_shared<LiteralSymbolGroup>((CharType)bottom_beg, (CharType)bottom_end, bottom_beg == this->rangeStart ? this->actions : rhs->actions));
        }

        this->rangeStart = (CharType)mid_beg;
        this->rangeEnd = (CharType)mid_end;
        if (!this->actions.empty() || !rhs->actions.empty()) {
            ret.push_back(std::make_shared<LiteralSymbolGroup>((CharType)mid_beg, (CharType)mid_end, rhs->actions));
        }

        if (top_beg <= top_end) {
            ret.push_back(std::make_shared<LiteralSymbolGroup>((CharType)top_beg, (CharType)top_end, top_beg == this->rangeEnd ? rhs->actions : this->actions));
        }
    }

    return ret;
}


bool ProductionSymbolGroup::contains(const SymbolGroup* symbol) const {
    const ProductionSymbolGroup* psg = dynamic_cast<const ProductionSymbolGroup*>(symbol);
    if (psg == nullptr) {
        return false;
    }

    return
        std::includes(this->referencedComponents.cbegin(), this->referencedComponents.cend(), psg->referencedComponents.cbegin(), psg->referencedComponents.cend())
        ;
}

bool EmptySymbolGroup::contains(const SymbolGroup* rhs) const {
    return dynamic_cast<const EmptySymbolGroup*>(rhs) != nullptr;
}

bool EmptySymbolGroup::equals(const SymbolGroup* rhs) const {
    return dynamic_cast<const EmptySymbolGroup*>(rhs) != nullptr;
}

bool EmptySymbolGroup::disjoint(const SymbolGroup* rhs) const {
    return dynamic_cast<const EmptySymbolGroup*>(rhs) == nullptr;
}

std::list<std::shared_ptr<SymbolGroup>> EmptySymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) {
    if (dynamic_cast<EmptySymbolGroup*>(rhs.get()) == nullptr) {
        return std::list<std::shared_ptr<SymbolGroup>>({ rhs });
    } else {
        return std::list<std::shared_ptr<SymbolGroup>>();
    }
}

bool ProductionSymbolGroup::equals(const SymbolGroup* rhs) const {
    const ProductionSymbolGroup* rhsCast = dynamic_cast<const ProductionSymbolGroup*>(rhs);
    if (rhsCast == nullptr) {
        return false;
    } else {
        return this->referencedComponents == rhsCast->referencedComponents;
    }
}

bool ProductionSymbolGroup::disjoint(const SymbolGroup* rhs) const {
    const ProductionSymbolGroup* rhsCast = dynamic_cast<const ProductionSymbolGroup*>(rhs);
    if (rhsCast == nullptr) {
        return true;
    } else {
        for (const auto referencedComponentPtr : referencedComponents) {
            auto fit = std::find_if(rhsCast->referencedComponents.cbegin(), rhsCast->referencedComponents.cend(), [referencedComponentPtr](const MachineComponent* rhsComponentPtr) {
                return referencedComponentPtr->name == rhsComponentPtr->name;
                });
            if (fit != rhsCast->referencedComponents.cend()) {
                return false;
            }
        }

        return true;
    }
}

std::list<std::shared_ptr<SymbolGroup>> ProductionSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhsUncast) {
    const ProductionSymbolGroup* rhs = dynamic_cast<const ProductionSymbolGroup*>(rhsUncast.get());
    if (rhs == nullptr) {
        return std::list<std::shared_ptr<SymbolGroup>>({ rhsUncast });
    }

    std::list<const MachineComponent*> sharedComponents;
    std::list<const MachineComponent*> excludedComponents;
    for (auto it = referencedComponents.begin(); it != referencedComponents.end(); ) {
        auto fit = std::find_if(rhs->referencedComponents.cbegin(), rhs->referencedComponents.cend(), [it](const MachineComponent* rhsComponentPtr) {
            return (*it)->name == rhsComponentPtr->name;
            });
        if (fit != rhs->referencedComponents.cend()) {
            sharedComponents.push_back(*fit);
            it = referencedComponents.erase(it);
        } else {
            excludedComponents.push_back(*fit);
            ++it;
        }
    }

    if (sharedComponents.empty()) {
        return std::list<std::shared_ptr<SymbolGroup>>({ rhsUncast });
    } else {
        std::list<std::shared_ptr<SymbolGroup>> ret;
        if (this->actions == rhs->actions) {
            ret.push_back(std::make_shared<ProductionSymbolGroup>(sharedComponents, this->actions));
        } else {
            ret.push_back(std::make_shared<ProductionSymbolGroup>(sharedComponents, this->actions));
            ret.push_back(std::make_shared<ProductionSymbolGroup>(sharedComponents, rhs->actions));
        }
        ret.push_back(std::make_shared<ProductionSymbolGroup>(excludedComponents, rhs->actions));
        
        return ret;
    }
}
