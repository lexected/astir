#include "NFAAction.h"

#include "GenerationVisitor.h"

bool NFAAction::operator==(const NFAAction& rhs) const {
    return
        this->type == rhs.type && this->contextPath == rhs.contextPath
        && this->targetName == rhs.targetName && this->payload == rhs.payload
        ;
}

NFAActionRegister NFAActionRegister::operator+(const NFAActionRegister& rhs) const {
    NFAActionRegister ret(*this);

    for (const auto& are : rhs) {
        auto it = std::find_if(begin(), end(), [are](const NFAAction& entry) {
            return entry.type == are.type && are.contextPath == entry.contextPath && are.targetName == entry.targetName;
            });
        if (it == end()) {
            ret.push_back(are);
        }
    }

    return ret;
}

const NFAActionRegister& NFAActionRegister::operator+=(const NFAActionRegister& rhs) {
    for (const auto& are : rhs) {
        auto it = std::find_if(begin(), end(), [&are](const NFAAction& entry) {
            return entry.type == are.type && are.contextPath == entry.contextPath && are.targetName == entry.targetName;
            });
        if (it == end()) {
            this->push_back(are);
        }
    }

    return *this;
}

bool NFAActionRegister::operator==(const NFAActionRegister& rhs) const {
    // TODO: might need an improvement for the future to consider different orders... or maybe not?
    return *static_cast<const std::list<NFAAction>*>(this) == rhs;
}