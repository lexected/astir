#include "LRA.h"

LRATransition& LRA::addTransition(AFAState sourceState, AFAState targetState, const std::shared_ptr<SymbolGroup>& condition) {
    return this->AFA<LRAStateObject, LRTag>::addTransition(sourceState, LRATransition(targetState, condition));
}

LRTag::LRTag(const TypeFormingStatement* statement, const SymbolGroupPtrVector& lookahead)
    : statement(statement), lookahead(lookahead) { }

LRTag::LRTag(const TypeFormingStatement* statement, const SymbolGroupPtrVector&& lookahead)
    : statement(statement), lookahead(lookahead) { }

bool LRTag::operator<(const LRTag& rhs) const {
    return
        ((unsigned long long int)statement) < ((unsigned long long int)rhs.statement)
        || lookahead < rhs.lookahead
        ;
}

const LRAStateObject& LRAStateObject::operator+=(const LRAStateObject& rhs) {
    actions.insert(actions.end(), rhs.actions.cbegin(), rhs.actions.cend());
    return *this;
}

void LRAStateObject::copyPayloadIn(const LRAStateObject& rhs) {
    actions.insert(actions.end(), rhs.actions.cbegin(), rhs.actions.cend());
}

void LRAStateObject::copyPayloadIn(const LRATransition& rhs) {
    // no-op
}

void LRAStateObject::copyPayloadOut(LRATransition& rhs) const {
    // no-op
}
