#include "LRA.h"

LRATransition& LRA::addTransition(AFAState sourceState, AFAState targetState, const std::shared_ptr<SymbolGroup>& condition) {
    return this->AFA<LRAStateObject, LRTag>::addTransition(sourceState, LRATransition(targetState, condition));
}

LRTag::LRTag(const TypeFormingStatement* statement, const SymbolGroupPtrVector& lookahead)
    : statement(statement), lookahead(lookahead) { }

LRTag::LRTag(const TypeFormingStatement* statement, const SymbolGroupPtrVector&& lookahead)
    : statement(statement), lookahead(lookahead) { }
