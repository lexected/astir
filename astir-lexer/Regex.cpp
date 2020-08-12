#include "Regex.h"

#include "SemanticTree.h"

const IFileLocalizable* RepetitiveRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    return actionAtomicRegex->findRecursiveReference(machine, namesEncountered, targetName);
}

const IFileLocalizable* LookaheadRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    const IFileLocalizable* ret = match->findRecursiveReference(machine, namesEncountered, targetName);
    if (ret) {
        return ret;
    }

    return lookahead->findRecursiveReference(machine, namesEncountered, targetName);
}

const IFileLocalizable* ActionAtomicRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    return regex->findRecursiveReference(machine, namesEncountered, targetName);
}

const IFileLocalizable* DisjunctiveRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    for (const auto& conjunctiveRegexPtr : disjunction) {
        auto ret = conjunctiveRegexPtr->findRecursiveReference(machine, namesEncountered, targetName);
        if (ret) {
            return ret;
        }
    }

    return nullptr;
}

const IFileLocalizable* ConjunctiveRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    for (const auto& rootRegexPtr : conjunction) {
        auto ret = rootRegexPtr->findRecursiveReference(machine, namesEncountered, targetName);
        if (ret) {
            return ret;
        }
    }

    return nullptr;
}

const IFileLocalizable* ReferenceRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    if (targetName == referenceName) {
        return this;
    }

    return machine.findRecursiveReferenceThroughName(referenceName, namesEncountered, targetName);
}
