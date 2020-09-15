#include "Regex.h"
#include "MachineStatement.h"
#include "MachineDefinition.h"
#include "NFA.h"
#include "NFABuilder.h"

#include "SemanticAnalysisException.h"

void RepetitiveRegex::completeReferences(const MachineDefinition& machine) {
	regex->completeReferences(machine);
}

IFileLocalizableCPtr RepetitiveRegex::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
	return regex->findRecursiveReference(referencingEntitiesEncountered);
}

NFA RepetitiveRegex::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void RepetitiveRegex::checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) {
	regex->checkAndTypeformActionUsage(machine, context, areActionsAllowed);
}

void RootRegex::checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) {
	if (!areActionsAllowed && !actions.empty()) {
		throw SemanticAnalysisException("Regex actions appearing in the regex located at " + this->locationString() + " while all actions are prohibited within the context of '" + context->name + "' (presumably a regex statement) declared at " + context->locationString());
	}

	auto attributedStatement = dynamic_cast<const AttributedStatement*>(context);
	if (!attributedStatement) {
		throw SemanticAnalysisException("Regex actions attempting to modify '" + context->name + "' in the regex at " + this->locationString() + " but '" + context->name + "', declared at " + context->locationString() + ", is not an attributed statement");
	}

	for (auto& action : actions) {
		std::shared_ptr<CategoryStatement> __tmp;
		auto fieldPtr = attributedStatement->findField(action.target, __tmp);
		action.targetField = fieldPtr;
		if (!fieldPtr) {
			throw SemanticAnalysisException("The action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not recognized as a field in the context of the rule '" + context->name + "' with definition at '" + context->locationString());
		}

		switch (action.type) {
			case RegexActionType::Flag:
			case RegexActionType::Unflag: {
				auto ffPtr = std::dynamic_pointer_cast<FlagField>(fieldPtr);
				if (ffPtr == nullptr) {
					throw SemanticAnalysisException("The action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not a 'flag' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}

			case RegexActionType::Capture:
			case RegexActionType::Empty:
			case RegexActionType::Append:
			case RegexActionType::Prepend: {
				auto rfPtr = std::dynamic_pointer_cast<RawField>(fieldPtr);
				if (rfPtr == nullptr) {
					throw SemanticAnalysisException("The action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not an 'raw' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}

			case RegexActionType::Set: {
				auto svtf = std::dynamic_pointer_cast<VariablyTypedField>(fieldPtr);
				if (svtf == nullptr) {
					throw SemanticAnalysisException("The typed action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not a typed field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}

				const std::string sitemType = computeItemType(machine, context);
				if (sitemType != svtf->type) {
					throw SemanticAnalysisException("The typed action at '" + action.locationString() + "' refers to target '" + action.target + "' of type '" + svtf->type + "' with payload of type '" + sitemType + "' -- in '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
			}
			case RegexActionType::Unset: {
				auto ifPtr = std::dynamic_pointer_cast<ItemField>(fieldPtr);
				if (ifPtr == nullptr) {
					throw SemanticAnalysisException("The action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not an 'item' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}

			case RegexActionType::Push: {
				auto pvtf = std::dynamic_pointer_cast<VariablyTypedField>(fieldPtr);
				if (pvtf == nullptr) {
					throw SemanticAnalysisException("The typed action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not a typed field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				const std::string pitemType = computeItemType(machine, context);
				if (pitemType != pvtf->type) {
					throw SemanticAnalysisException("The typed action at '" + action.locationString() + "' refers to target '" + action.target + "' of type '" + pvtf->type + "' with payload of type '" + pitemType + "' -- in '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
			}
			case RegexActionType::Pop:
			case RegexActionType::Clear: {
				auto lf = std::dynamic_pointer_cast<ListField>(fieldPtr);
				if (lf == nullptr) {
					throw SemanticAnalysisException("The action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not a 'list' field of '" + context->name + "' (see its definition at " + context->locationString() + ")");
				}
				break;
			}
			case RegexActionType::None:
				// ehh, should never happen ... 
				throw SemanticAnalysisException("The action at '" + action.locationString() + "' refers to target '" + action.target + "' that is not 'none-able' '" + context->name + "' (see its definition at '" + context->locationString() + ")");
				break;
		}
	}
}

std::string RootRegex::computeItemType(const MachineDefinition& machine, const MachineStatement* context) const {
	// TODO: figure out wtf is this for...
	return "raw";
}

void DisjunctiveRegex::completeReferences(const MachineDefinition& machine) {
	for (const auto& conjunction : disjunction) {
		conjunction->completeReferences(machine);
	}
}

IFileLocalizableCPtr DisjunctiveRegex::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
	for (const auto& conjunction : disjunction) {
		auto ret = conjunction->findRecursiveReference(referencingEntitiesEncountered);
		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

NFA DisjunctiveRegex::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void DisjunctiveRegex::checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) {
	for (const auto& conjunction : disjunction) {
		conjunction->checkAndTypeformActionUsage(machine, context, areActionsAllowed);
	}
}

void ConjunctiveRegex::completeReferences(const MachineDefinition& machine) {
	for (const auto& r : conjunction) {
		r->completeReferences(machine);
	}
}

IFileLocalizableCPtr ConjunctiveRegex::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
	for (const auto& r : conjunction) {
		auto ret = r->findRecursiveReference(referencingEntitiesEncountered);
		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

NFA ConjunctiveRegex::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void ConjunctiveRegex::checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) {
	for (const auto& rootRegex : conjunction) {
		rootRegex->checkAndTypeformActionUsage(machine, context, areActionsAllowed);
	}
}

std::string ReferenceRegex::computeItemType(const MachineDefinition& machine, const MachineStatement* context) const {
	auto capturedStatement = machine.findMachineStatement(referenceName);
	const std::string& typeName = capturedStatement->name;
	return typeName;
}

void ReferenceRegex::completeReferences(const MachineDefinition& machine) {
	auto ms = machine.findMachineStatement(referenceName);
	if (!ms) {
		throw SemanticAnalysisException("Name '"+referenceName+"' referenced in regex at " + this->locationString() + " could not be found in the context of the machine '" + machine.name + "' declared at " + machine.locationString());
	}

	referenceStatement = ms.get();
}

IFileLocalizableCPtr ReferenceRegex::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
	auto fit = std::find(referencingEntitiesEncountered.cbegin(), referencingEntitiesEncountered.cend(), this->referenceStatement);
	if (fit != referencingEntitiesEncountered.cend()) {
		return this->referenceStatement;
	}

	referencingEntitiesEncountered.push_back(this->referenceStatement);
	auto ret = referenceStatement->findRecursiveReference(referencingEntitiesEncountered);
	if (ret) {
		return ret;
	}
	referencingEntitiesEncountered.pop_back();

	return nullptr;
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

NFA EmptyRegex::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}
