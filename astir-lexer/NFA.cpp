#include "NFA.h"

#include <stack>
#include <algorithm> 

#include "Exception.h"
#include "SemanticTree.h"

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
    addTransition(state, Transition(target, std::make_shared<EmptySymbolGroup>()));
    return states[state].transitions.back();
}

Transition& NFA::addEmptyTransition(State state, State target, const ActionRegister& ar) {
    addTransition(state, Transition(target, std::make_shared<EmptySymbolGroup>(ar)));
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

void NFA::actionize(const ActionRegister& actions) {
    for (State fs : finalStates) {
        State newState = addState();
        addEmptyTransition(fs, newState, actions);
    }
}

NFA NFA::buildDFA() const {
    // the idea of the conversion algorithm is quite simple,
    // but what makes the whole conversion rather challenging
    // is the bookkeeping necessary to know what actions are
    // to be executed when an accepting state is reached

    NFA base;
    std::vector<DFAState> stateMap;

    auto initialSet = calculateEpsilonClosure(std::set<State>({ (State)0 }));
    stateMap.push_back(DFAState(initialSet));

    State unmarkedStateDfaState;
    while ((unmarkedStateDfaState = findUnmarkedState(stateMap)) != stateMap.size()) {
        auto& stateObject = stateMap[unmarkedStateDfaState];
        stateObject.marked = true;

        auto transitionSymbolPtrs = calculateTransitionSymbols(stateObject.nfaStates);
        for (const auto& transitionSymbolPtr : transitionSymbolPtrs) {
            auto advancedStateSet = calculateSymbolClosure(stateObject.nfaStates, transitionSymbolPtr.get());
            auto epsilonClosureAdvancedStateSet = calculateEpsilonClosure(advancedStateSet);
            State theCorrespondingDFAState = findStateByNFAStateSet(stateMap, epsilonClosureAdvancedStateSet);
            if (theCorrespondingDFAState == epsilonClosureAdvancedStateSet.size()) {
                theCorrespondingDFAState = base.addState();
                stateMap.emplace_back(epsilonClosureAdvancedStateSet);
                
                std::list<State> intersectionOfNFAStates;
                std::set_intersection(epsilonClosureAdvancedStateSet.cbegin(), epsilonClosureAdvancedStateSet.cend(), finalStates.cbegin(), finalStates.cend(), intersectionOfNFAStates.begin());
                if (intersectionOfNFAStates.size() > 0) {
                    base.finalStates.insert(theCorrespondingDFAState);
                }
            }

            base.addTransition(unmarkedStateDfaState, Transition(theCorrespondingDFAState, transitionSymbolPtr));
        }
    }

    return base;
}

std::set<State> NFA::calculateEpsilonClosure(const std::set<State>& states) const {
    std::stack<State> statesToCheck;
    for (const auto& state : states) {
        statesToCheck.push(state);
    }

    std::set<State> ret(states);
    while (!statesToCheck.empty()) {
        State currentState = statesToCheck.top();
        statesToCheck.pop();

        const auto& stateObject = this->states[currentState];
        for (const auto& transition : stateObject.transitions) {
            const EmptySymbolGroup* esg = dynamic_cast<const EmptySymbolGroup*>(transition.condition.get());
            if (esg == nullptr) {
                continue;
            }

            std::pair<std::set<State>::iterator, bool> insertionOutcome = ret.insert(transition.target);
            if (insertionOutcome.second) {
                statesToCheck.push(transition.target);
            }
        }
    }

    return ret;
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
    std::list<ProductionSymbolGroup> productionGroupsUsed;
    std::list<LiteralSymbolGroup> literalGroupsUsed;
    for (State state : states) {
        const auto& stateObject = this->states[state];
        for (const auto& transition : stateObject.transitions) {
            auto lsg = std::dynamic_pointer_cast<LiteralSymbolGroup>(transition.condition);
            if (lsg != nullptr) {
                literalGroupsUsed.push_back(*lsg);
                continue;
            }

            auto psg = std::dynamic_pointer_cast<ProductionSymbolGroup>(transition.condition);
            if (psg != nullptr) {
                productionGroupsUsed.push_back(*psg);
            }
        }
    }

    calculateDisjointLiteralSymbolGroups(literalGroupsUsed);
    calculateDisjointProductionSymbolGroups(productionGroupsUsed);

    std::list<std::shared_ptr<SymbolGroup>> ret;
    for (const LiteralSymbolGroup& lsg : literalGroupsUsed) {
        ret.push_back(std::make_shared<LiteralSymbolGroup>(lsg));
    }
    for (const ProductionSymbolGroup& psg : productionGroupsUsed) {
        ret.push_back(std::make_shared<ProductionSymbolGroup>(psg));
    }

    return ret;
}

void NFA::calculateDisjointLiteralSymbolGroups(std::list<LiteralSymbolGroup>& symbolGroups) {
    auto it = symbolGroups.begin();
    bool equalTransitionFound = false;
    while(it != symbolGroups.end()) {
        auto iit = it++;
        for (; iit != symbolGroups.end(); ++iit) {
            if (iit->equals(*it)) {
                equalTransitionFound = true;
                break;
            } else if (!iit->disjoint(*it)) {
                equalTransitionFound = false;
                break;
            }
        }

        if (iit != symbolGroups.end()) {
            if (equalTransitionFound) {
                it->actions += iit->actions;
            } else {
                LiteralSymbolGroup::disjoin(symbolGroups, *it, *iit);
                it = symbolGroups.erase(it);
            }
            symbolGroups.erase(iit);
        }
    }
}

std::list<LiteralSymbolGroup> NFA::negateLiteralSymbolGroups(const std::list<LiteralSymbolGroup>& symbolGroups) {
    std::list<LiteralSymbolGroup> ret;
    unsigned char lastEnd = 0;
    for (const auto lsg : symbolGroups) { 
        if (lsg.rangeStart > lastEnd+1) {
            ret.emplace_back(lastEnd+1, lsg.rangeStart - 1);
        }
        lastEnd = lsg.rangeEnd;
    }
    if (lastEnd < (unsigned char)255) {
        ret.emplace_back(lastEnd + 1, (unsigned char)255);
    }

    return ret;
}

void NFA::calculateDisjointProductionSymbolGroups(std::list<ProductionSymbolGroup>& symbolGroups) {
    auto it = symbolGroups.begin();
    bool equalTransitionFound = false;
    std::list<const Category*> categoryPath;
    while (it != symbolGroups.end()) {
        auto iit = symbolGroups.begin();
        for (; iit != symbolGroups.end(); ++iit) {
            if (iit == it) {
                continue;
            }

            if (it->referencedComponent == iit->referencedComponent) {
                equalTransitionFound = true;
                break;
            }

            if (it->referencedComponent->entails(iit->referencedComponent->name, categoryPath)) {
                break;
            }
        }

        if (iit != symbolGroups.end()) {
            if (!equalTransitionFound) {
                const MachineComponent* prevComponent = nullptr;
                for (const Category* cat : categoryPath) {
                    for (auto pair : cat->references) {
                        if (pair.second.component != prevComponent) {
                            symbolGroups.emplace_back(pair.second.component, it->actions); // no need to worry about isFromFollows heres
                        }
                    }
                    prevComponent = cat;
                }

                categoryPath.clear();
            }
            iit->actions += it->actions;
            it = symbolGroups.erase(it);
        }
    }
}

State NFA::findUnmarkedState(const std::vector<DFAState>& stateMap) const {
    size_t index;
    for (index = 0; index < stateMap.size(); ++index) {
        if (!stateMap[index].marked) {
            break;
        }
    }

    return index;
}

State NFA::findStateByNFAStateSet(const std::vector<DFAState>& stateMap, const std::set<State>& nfaSet) const {
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

    return this->rangeStart <= ls->rangeStart && this->rangeEnd >= ls->rangeEnd;
}

bool LiteralSymbolGroup::equals(const LiteralSymbolGroup& rhs) const {
    return this->rangeStart == rhs.rangeStart && this->rangeEnd == rhs.rangeEnd;
}

bool LiteralSymbolGroup::disjoint(const LiteralSymbolGroup& rhs) const {
    return this->rangeStart > rhs.rangeEnd || rhs.rangeStart > this->rangeEnd;
}

void LiteralSymbolGroup::disjoin(std::list<LiteralSymbolGroup>& symbolGroups, const LiteralSymbolGroup& lhs, const LiteralSymbolGroup& rhs) {
    if (rhs.disjoint(lhs)) {
        symbolGroups.push_back(lhs);
        symbolGroups.push_back(rhs);
    } else if(lhs.rangeEnd >= rhs.rangeStart) {
        auto mid_beg = std::max(lhs.rangeStart, rhs.rangeStart);
        auto mid_end = std::min(lhs.rangeEnd, rhs.rangeEnd);
        // thanks to the first condition we have mid_beg <= mid_end

        auto bottom_beg = std::min(lhs.rangeStart, rhs.rangeStart);
        auto bottom_end = mid_beg - 1;

        auto top_beg = std::min(lhs.rangeEnd, rhs.rangeEnd) + 1;
        auto top_end = std::max(lhs.rangeEnd, rhs.rangeEnd);

        if (bottom_beg <= bottom_end) {
            symbolGroups.emplace_back(bottom_beg, bottom_end, bottom_beg == lhs.rangeStart ? lhs.actions : rhs.actions);
        }

        ActionRegister actionsUnionized = lhs.actions + rhs.actions;
        symbolGroups.emplace_back(mid_beg, mid_end, actionsUnionized);

        if (top_beg <= top_end) {
            symbolGroups.emplace_back(top_beg, top_end, top_beg == lhs.rangeEnd ? rhs.actions : lhs .actions);
        }
    }
}

bool ProductionSymbolGroup::contains(const SymbolGroup* symbol) const {
    const ProductionSymbolGroup* psg = dynamic_cast<const ProductionSymbolGroup*>(symbol);
    if (psg == nullptr) {
        return false;
    }

    return referencedComponent->entails(psg->referencedComponent->name);
}

ActionRegister ActionRegister::operator+(const ActionRegister& rhs) const {
    ActionRegister ret(*this);

    for (const auto& are : rhs) {
        auto it = std::find_if(begin(), end(), [are](const ActionRegisterEntry& entry) {
            return entry.action == are.action && are.targetComponent == entry.targetComponent;
            });
        if (it == end()) {
            ret.push_back(are);
        }
    }

    return ret;
}

const ActionRegister& ActionRegister::operator+=(const ActionRegister& rhs) {
    for (const auto& are : rhs) {
        auto it = std::find_if(begin(), end(), [&are](const ActionRegisterEntry& entry) {
            return entry.action == are.action && are.targetComponent == entry.targetComponent;
            });
        if (it == end()) {
            this->push_back(are);
        }
    }

    return *this;
}

bool EmptySymbolGroup::contains(const SymbolGroup* symbol) const {
    return false;
}
