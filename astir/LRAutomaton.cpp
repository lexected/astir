#include "LRAutomaton.h"

const LRAStateObject& LRAStateObject::operator+=(const LRAStateObject& rhs) {
    this->AFAStateObject<LRATransition>::operator+=(rhs);

    copyPayloadIn(rhs);

    return *this;
}

void LRAStateObject::copyPayloadIn(const LRAStateObject& rhs) {
    actions.insert(actions.cend(), rhs.actions.cbegin(), rhs.actions.cend());
}

void LRAStateObject::copyPayloadIn(const LRATransition& rhs) {
    // no-op
}

void LRAStateObject::copyPayloadOut(LRATransition& rhs) const {
    // no-op
}
