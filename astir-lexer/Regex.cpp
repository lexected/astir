#include "Regex.h"
#include "SyntacticTree.h"
#include "SemanticTree.h"
#include "NFA.h"
#include "NFABuilder.h"

const IFileLocalizable* RepetitiveRegex::findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
    return regex->findRecursiveReference(machine, namesEncountered, targetName);
}

NFA RepetitiveRegex::accept(const NFABuilder& nfaBuilder) const {
    return nfaBuilder.visit(this);
}

void RepetitiveRegex::checkActionUsage(const Machine& machine, const MachineComponent* context) const {
    regex->checkActionUsage(machine, context);
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

void LookaheadRegex::checkActionUsage(const Machine& machine, const MachineComponent* context) const {
    match->checkActionUsage(machine, context);
    // lookahead->checkActionUsage(context); NO NEED!!!
}

void PrimitiveRegex::checkActionUsage(const Machine& machine, const MachineComponent* context) const {
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
                if (!fieldPtr->listable()) {
                    throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'list-operation-able' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
                }
                break;
            case RegexAction::None:
                // ehh, should never happen ... 
                if (!fieldPtr->settable()) {
                    throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'none-able' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
                }
                break;
        }

        checkActionUsageFieldType(machine, context, actionTargetPair.action, fieldPtr);
    }
}

void PrimitiveRegex::checkActionUsageFieldType(const Machine& machine, const MachineComponent* context, RegexAction action, const Field* targetField) const {
    const VariablyTypedField* vtf = dynamic_cast<const VariablyTypedField*>(targetField);
    if (vtf) {
        throw SemanticAnalysisException("Action type mismatch detected at " + context->locationString() + ", cannot associate a raw regex capture result to a field of particular type '" + vtf->type + "'");
    }
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

void DisjunctiveRegex::checkActionUsage(const Machine& machine, const MachineComponent* context) const {
    for (const auto& conjunction : disjunction) {
        conjunction->checkActionUsage(machine, context);
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

void ConjunctiveRegex::checkActionUsage(const Machine& machine, const MachineComponent* context) const {
    for (const auto& rootRegex : conjunction) {
        rootRegex->checkActionUsage(machine, context);
    }
}

void ReferenceRegex::checkActionUsageFieldType(const Machine& machine, const MachineComponent* context, RegexAction action, const Field* targetField) const {
    const MachineComponent* capturedComponent = machine.findMachineComponent(referenceName);
    const std::string& typeName = capturedComponent->name;

    const VariablyTypedField* vtf = dynamic_cast<const VariablyTypedField*>(targetField);
    if (vtf) {
        if (action == RegexAction::Append || action == RegexAction::Prepend || action == RegexAction::Set) {
            if (vtf->type != typeName) {
                throw SemanticAnalysisException("Action type mismatch detected at " + context->locationString() + ", cannot associate a production reference of type '" + typeName + "' to a field of type '" + vtf->type + "'");
            }
        }
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
