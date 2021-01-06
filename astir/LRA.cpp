#include "LRA.h"

LRATransition& LRA::addTransition(AFAState sourceState, AFAState targetState, const std::shared_ptr<SymbolGroup>& condition) {
    return this->AFA<LRAStateObject, LRTag>::addTransition(sourceState, LRATransition(targetState, condition));
}
