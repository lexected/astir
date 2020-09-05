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
	for (const auto& actionTargetPair : actions) {
		const Field* fieldPtr = context->findField(actionTargetPair.target);
		if (fieldPtr == nullptr) {
			throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not recognized as a field in the context of the rule '" + context->name + "' with definition at '" + context->locationString());
		}

		switch (actionTargetPair.type) {
			case RegexActionType::Flag:
			case RegexActionType::Unflag: {
				const FlagField* ffPtr = dynamic_cast<const FlagField*>(fieldPtr);
				if (ffPtr == nullptr) {
					throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not a 'flag' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}

			case RegexActionType::Capture:
			case RegexActionType::Empty:
			case RegexActionType::Append:
			case RegexActionType::Prepend: {
				const RawField* rfPtr = dynamic_cast<const RawField*>(fieldPtr);
				if (rfPtr == nullptr) {
					throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not an 'raw' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}

			case RegexActionType::Set: {
				const VariablyTypedField* svtf = dynamic_cast<const VariablyTypedField*>(fieldPtr);
				if (svtf == nullptr) {
					throw SemanticAnalysisException("The typed action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not a typed field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}

				const std::string sitemType = computeItemType(machine, context);
				if (sitemType != svtf->type) {
					throw SemanticAnalysisException("The typed action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' of type '" + svtf->type + "' with payload of type '" + sitemType + "' -- in '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
			}
			case RegexActionType::Unset: {
				const ItemField* ifPtr = dynamic_cast<const ItemField*>(fieldPtr);
				if (ifPtr == nullptr) {
					throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not an 'item' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}

			case RegexActionType::Push: {
				const VariablyTypedField* pvtf = dynamic_cast<const VariablyTypedField*>(fieldPtr);
				if (pvtf == nullptr) {
					throw SemanticAnalysisException("The typed action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not a typed field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				const std::string pitemType = computeItemType(machine, context);
				if (pitemType != pvtf->type) {
					throw SemanticAnalysisException("The typed action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' of type '" + pvtf->type + "' with payload of type '" + pitemType + "' -- in '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
			}
			case RegexActionType::Pop:
			case RegexActionType::Clear: {
				const ListField* lf = dynamic_cast<const ListField*>(fieldPtr);
				if (lf == nullptr) {
					throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not a 'list' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}
			case RegexActionType::None:
				// ehh, should never happen ... 
				throw SemanticAnalysisException("The action at '" + actionTargetPair.locationString() + "' refers to target '" + actionTargetPair.target + "' that is not 'none-able' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
				break;
		}
	}
}

std::string PrimitiveRegex::computeItemType(const Machine& machine, const MachineComponent* context) const {
	return "raw";
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

std::string ReferenceRegex::computeItemType(const Machine& machine, const MachineComponent* context) const {
	const MachineComponent* capturedComponent = machine.findMachineComponent(referenceName);
	const std::string& typeName = capturedComponent->name;
	return typeName;
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

NFA ArbitrarySymbolRegex::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

NFA LineEndRegex::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}
