#include "NFABuilder.h"
#include "Exception.h"

#include <stack>

#include "SemanticTree.h"

NFA NFABuilder::visit(const Category* category) const {
    NFA base;

    const std::string assignmentTarget = m_generationContextPath + "__" + category->name;
    std::string generationContextPathPrefix = assignmentTarget + "__";

    for (const auto referencePair : category->references) {
        if (referencePair.second.isAFollowsReference) {
            throw SemanticAnalysisException("What is going on??");

            // unlikely to ever happen but ... actually won't ever happen at all ... right, I'm just not keen on deleting this
            /*NFA modification;
            State newState = modification.addState();
            modification.finalStates.insert(newState);
            modification.addTransition(0, Transition(newState, std::make_shared<ProductionSymbolGroup>(referencePair.second.component)));

            base |= modification;*/
        } else {
            const std::string assignmentSource = generationContextPathPrefix + referencePair.first;
            NFABuilder contextualizedBuilder(m_contextMachine, referencePair.second.component, assignmentSource);
            
            base.addContextedAlternative(referencePair.second.component->accept(contextualizedBuilder), assignmentTarget, assignmentSource);
        }
    }

    return base;
}

NFA NFABuilder::visit(const Rule* rule) const {
    NFABuilder contextualizedBuilder(this->m_contextMachine, rule, m_generationContextPath + "__" + rule->name);
    return rule->regex->accept(contextualizedBuilder);
}

NFA NFABuilder::visit(const DisjunctiveRegex* regex) const {
    NFA base;
    for (const auto& conjunctiveRegex : regex->disjunction) {
        base |= conjunctiveRegex->accept(*this);
    }
    return base;
}

NFA NFABuilder::visit(const ConjunctiveRegex* regex) const {
    NFA base;
    base.finalStates.insert(0);

    for (const auto& rootRegex : regex->conjunction) {
        base &= rootRegex->accept(*this);
    }
    return base;
}

NFA NFABuilder::visit(const RepetitiveRegex* regex) const {
    if (regex->minRepetitions == regex->INFINITE_REPETITIONS) {
        throw Exception("Can not create a machine for a regex with minimum of infinitely many repetitions");
    }

    NFA theMachine = regex->accept(*this);

    NFA base;
    base.finalStates.insert(0);
    for (unsigned long i = 0; i < regex->minRepetitions; ++i) {
        base &= theMachine;
    }

    if (regex->maxRepetitions == regex->INFINITE_REPETITIONS) {
        auto theVeryFinalState = theMachine.addState();
        for (const auto& finalState : theMachine.finalStates) {
            theMachine.addEmptyTransition(finalState, 0);
            theMachine.addEmptyTransition(finalState, theVeryFinalState);
        }

        NFA theMachineSTAR;
        theMachineSTAR.finalStates.insert(0);
        theMachineSTAR &= theMachine;
        ++theVeryFinalState;
        theMachineSTAR.addEmptyTransition(0, theVeryFinalState);

        base &= theMachineSTAR;
    } else {
        theMachine.concentrateFinalStates();
        auto lastBaseFinalState = base.concentrateFinalStates();

        std::stack<State> interimConcentratedFinalStates({ lastBaseFinalState });
        for (unsigned long i = regex->minRepetitions; i < regex->maxRepetitions; ++i) {
            base &= theMachine;
            auto interimFinalState = *base.finalStates.begin();
            interimConcentratedFinalStates.push(interimFinalState);
        }
        auto theVeryFinalState = interimConcentratedFinalStates.top();
        interimConcentratedFinalStates.pop();
        while (!interimConcentratedFinalStates.empty()) {
            auto stateToConnect = interimConcentratedFinalStates.top();
            interimConcentratedFinalStates.pop();

            base.addEmptyTransition(stateToConnect, theVeryFinalState);
        }
    }

    return base;
}

NFA NFABuilder::visit(const LookaheadRegex* regex) const {
    //TODO: implement LookaheadRegex handling
    throw Exception("LookaheadRegexes not supported at the moment");
}

NFA NFABuilder::visit(const AnyRegex* regex) const {
    NFA base;
    auto newState = base.addState();

    auto actionRegister = computeActionRegisterEntries(regex->actions);

    auto literalGroups = computeLiteralGroups(regex);
    for (auto& literalSymbolGroup : literalGroups) {
        base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, actionRegister)));
    }

    base.finalStates.insert(newState);
    return base;
}

NFA NFABuilder::visit(const ExceptAnyRegex* regex) const {
    NFA base;
    auto newState = base.addState();

    auto literalGroups = computeLiteralGroups(regex);
    NFA::calculateDisjointLiteralSymbolGroups(literalGroups);
    auto negatedGroups = NFA::negateLiteralSymbolGroups(literalGroups);

    auto actionRegister = computeActionRegisterEntries(regex->actions);

    for (auto& literalSymbolGroup : negatedGroups) {
        base.addTransition(0, Transition(newState, std::make_shared<LiteralSymbolGroup>(literalSymbolGroup, actionRegister)));
    }

    base.finalStates.insert(newState);

    return base;
}

NFA NFABuilder::visit(const LiteralRegex* regex) const {
    NFA base;
    
    auto actionRegister = computeActionRegisterEntries(regex->actions);

    State prevState = 0;
    for(unsigned char c : regex->literal) {   
        State newState = base.addState();
        base.addTransition(prevState, Transition(newState, std::make_shared<LiteralSymbolGroup>(c, c, actionRegister)));

        prevState = newState;
    }

    base.finalStates.insert(prevState);

    return base;
}

NFA NFABuilder::visit(const ArbitraryLiteralRegex* regex) const {
    NFA base;

    auto actionRegister = computeActionRegisterEntries(regex->actions);

    auto newState = base.addState();
    base.addTransition(0, Transition(newState, std::make_shared<ArbitrarySymbolGroup>(actionRegister)));
    base.finalStates.insert(newState);

    return base;
}


NFA NFABuilder::visit(const ReferenceRegex* regex) const {
    NFA base;
    const State newBaseState = base.addState();
    base.finalStates.insert(newBaseState);

    bool follows;
    auto component = this->m_contextMachine.findMachineComponent(regex->referenceName, &follows);
    if (follows) {
        auto actionRegister = computeActionRegisterEntries(regex->actions);
        base.addTransition(0, Transition(newBaseState, std::make_shared<ProductionSymbolGroup>(component, actionRegister)));
    } else {
        const std::string targetPath = m_generationContextPath + "__" + regex->referenceName;
        auto actionRegister = computeActionRegisterEntries(regex->actions, targetPath, true);

        NFABuilder contextualizedBuilder(m_contextMachine, component, m_generationContextPath);
        NFA subNfa = component->accept(contextualizedBuilder);
        subNfa.addFinalActions(actionRegister);

        
        NFAActionRegister createContextActionRegister;
        createContextActionRegister.emplace_back(NFAActionType::CreateContext, m_generationContextPath, targetPath);
        // within the current context context (m_generationContextPath) create a new context (targetPath)
        base.addEmptyTransition(0, newBaseState, createContextActionRegister);
        base &= subNfa;
    }

    return base;
}

NFA NFABuilder::visit(const LineEndRegex* regex) const {
    NFA base;

    auto actionRegister = computeActionRegisterEntries(regex->actions);

    auto lineFeedState = base.addState();
    base.addTransition(0, Transition(lineFeedState, std::make_shared<LiteralSymbolGroup>('\n', '\n', actionRegister)));
    base.finalStates.insert(lineFeedState);
    auto carriageReturnState = base.addState();
    base.addTransition(0, Transition(carriageReturnState, std::make_shared<LiteralSymbolGroup>('\r', '\r')));
    base.addTransition(carriageReturnState, Transition(lineFeedState, std::make_shared<LiteralSymbolGroup>('\n', '\n', actionRegister)));

    return base;
}

std::list<LiteralSymbolGroup> NFABuilder::computeLiteralGroups(const AnyRegex* regex) const {
    std::list<LiteralSymbolGroup> literalGroup;

    for (const auto& literal : regex->literals) {
        for (const auto& c : literal) {
            literalGroup.emplace_back(c, c);
        }
    }
    for (const auto& range : regex->ranges) {
        unsigned char beginning = (unsigned char)range.start;
        unsigned char end = (unsigned char)range.end;
        literalGroup.emplace_back(beginning, end);
    }

    return literalGroup;
}

NFAActionRegister NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions) const {
    return computeActionRegisterEntries(actions, "", false);
}

NFAActionRegister NFABuilder::computeActionRegisterEntries(const std::list<RegexAction>& actions, const std::string& subcontextPath, bool setToAssignWhereNecessary) const {
    NFAActionRegister ret;

    for (const RegexAction& atp : actions) {
        if (setToAssignWhereNecessary) {
            if (atp.type == RegexActionType::Set) {
                ret.emplace_back(NFAActionType::AssignContext, subcontextPath, m_generationContextPath + "__" + atp.target);
                // within the subcontext (subcontextPath) we make the assignment of itself to the upper context (m_generationContextPath)
            } else if (atp.type == RegexActionType::Append) {
                ret.emplace_back(NFAActionType::AppendContext, subcontextPath, m_generationContextPath + "__" + atp.target);
                // within the subcontext (subcontextPath) we make the appendment (?) of itself to the target field (m_generationContextPath + "__" + atp.target) of some upper context
            } else if (atp.type == RegexActionType::Prepend) {
                ret.emplace_back(NFAActionType::PrependContext, subcontextPath, m_generationContextPath + "__" + atp.target);
                // within the subcontext (subcontextPath) we make the prependment (?) of itself to the target field (m_generationContextPath + "__" + atp.target) of some upper context
            }
            
        } else {
            ret.emplace_back((NFAActionType)atp.type, m_generationContextPath, m_generationContextPath + "__" + atp.target);
            // within the context (m_generationContextPath) we modify the target (m_generationContextPath + "__" + atp.target) with the payload of the transition
        }
    }

    return ret;
}
