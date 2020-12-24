#include "AFACondition.h"

bool AFACondition::equals(const std::shared_ptr<AFACondition>& anotherCondition) const {
    return this->isEmpty() && anotherCondition->isEmpty();
}

bool AFACondition::isEmpty() const {
    return true;
}
