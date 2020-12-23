#include "AFA.h"

bool EmptyAFACondition::equals(const std::shared_ptr<AFACondition>& anotherCondition) const {
    return (bool)std::dynamic_pointer_cast<EmptyAFACondition>(anotherCondition);
}
