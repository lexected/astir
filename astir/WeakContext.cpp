#include "WeakContext.h"

WeakContext::WeakContext(std::shared_ptr<WeakContext> parent)
    : parent(parent) { }

WeakContext::WeakContext(std::shared_ptr<WeakContext> parent, std::list<ILLkFirstableCPtr> followedBy)
    : parent(parent), followedBy(followedBy) { }

bool WeakContext::equals(const WeakContext& rhs) const {
    if (followedBy != rhs.followedBy) {
        return false;
    }

    if (!rhs.parent && !parent) {
        return true;
    } else if (!rhs.parent || !parent) {
        return false;
    } else {
        return parent->equals(*rhs.parent);
    }
}

bool WeakContext::operator==(const WeakContext& rhs) const {
    return equals(rhs);
}
