#include "Regex.h"
#include "SyntacticTree.h"
#include "SemanticTree.h"
#include "NFA.h"
#include "NFABuilder.h"

const IFileLocalizable* RepetitiveRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    return actionAtomicRegex->findRecursiveReference(machine, namesEncountered, targetName);
}

NFA RepetitiveRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

void RepetitiveRegex::checkActionUsage(const MachineComponent* context) const {
    actionAtomicRegex->checkActionUsage(context);
}

const IFileLocalizable* LookaheadRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    const IFileLocalizable* ret = match->findRecursiveReference(machine, namesEncountered, targetName);
    if (ret) {
        return ret;
    }

    return lookahead->findRecursiveReference(machine, namesEncountered, targetName);
}

NFA LookaheadRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

void LookaheadRegex::checkActionUsage(const MachineComponent* context) const {
    match->checkActionUsage(context);
    // lookahead->checkActionUsage(context); NO NEED!!!
}

const IFileLocalizable* ActionAtomicRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    return regex->findRecursiveReference(machine, namesEncountered, targetName);
}

NFA ActionAtomicRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

void ActionAtomicRegex::checkActionUsage(const MachineComponent* context) const {
    for (const auto& actionTargetPair : actionTargetPairs) {
        const Field* fieldPtr = context->findField(actionTargetPair.target);
        if (fieldPtr == nullptr) {
            throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not recognized as a field in the context of the rule '" + context->name + "' with definition at '" + context->locationString());
        }

        switch (actionTargetPair.action) {
            case RegexAction::Flag:
            case RegexAction::Unflag:
                if (!fieldPtr->flaggable()) {
                    throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'flaggable' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
                }
                break;
            case RegexAction::Set:
            case RegexAction::Unset:
                if (!fieldPtr->settable()) {
                    throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'settable' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
                }
                break;
            case RegexAction::Append:
            case RegexAction::Prepend:
            case RegexAction::Clear:
            case RegexAction::LeftTrim:
            case RegexAction::RightTrim:
                if (!fieldPtr->settable()) {
                    throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'settable' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
                }
                break;
            case RegexAction::None:
                // ehh, should never happen ... 
                if (!fieldPtr->settable()) {
                    throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'none-able' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
                }
                break;
        }
    }

    this->regex->checkActionUsage(context);
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

NFA DisjunctiveRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

void DisjunctiveRegex::checkActionUsage(const MachineComponent* context) const {
    for (const auto& conjunction : disjunction) {
        conjunction->checkActionUsage(context);
    }
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

NFA ConjunctiveRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

void ConjunctiveRegex::checkActionUsage(const MachineComponent* context) const {
    for (const auto& rootRegex : conjunction) {
        rootRegex->checkActionUsage(context);
    }
}

const IFileLocalizable* ReferenceRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    if (targetName == referenceName) {
        return this;
    }

    return machine.findRecursiveReferenceThroughName(referenceName, namesEncountered, targetName);
}

NFA ReferenceRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

NFA AnyRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

NFA ExceptAnyRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

NFA LiteralRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

NFA ArbitraryLiteralRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

NFA LineEndRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}
