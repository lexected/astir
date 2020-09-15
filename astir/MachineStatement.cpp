#include "MachineStatement.h"

#include "MachineDefinition.h"
#include "SemanticAnalysisException.h"
#include "NFABuilder.h"
#include "GenerationVisitor.h"

std::shared_ptr<Field> AttributedStatement::findField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const {
	auto fit = std::find_if(fields.cbegin(), fields.cend(), [&name](const auto& fieldPtr) {
		return fieldPtr->name == name;
		});

	if (fit != fields.cend()) {
		categoryFoundIn = nullptr;
		return *fit;
	} else {
		return findCategoryField(name, categoryFoundIn);
	}
}

std::shared_ptr<Field> AttributedStatement::findCategoryField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const {
	for (const auto& categoryPair : categories) {
		auto foundPtr = categoryPair.second->findField(name, categoryFoundIn);
		if (foundPtr) {
			if (!categoryFoundIn) {
				categoryFoundIn = categoryPair.second;
			}
			return foundPtr;
		}
	}

	return nullptr;
}

void AttributedStatement::completeFieldDeclarations(MachineDefinition& context) const {
	for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
		const auto& currentField = *it;

		auto previousOccurenceOfTheName = std::find_if(fields.cbegin(), it, [&currentField](const auto& fieldPtr) {
			return fieldPtr->name == currentField->name;
			});

		if (previousOccurenceOfTheName != it) {
			throw SemanticAnalysisException("The field '" + currentField->name + "' declared at " + currentField->locationString() + " uses the name already taken by the field declared at " + (*it)->locationString());
		}

		std::shared_ptr<CategoryStatement> categoryFoundIn;
		auto foundFieldPtr = findCategoryField(currentField->name, categoryFoundIn);
		if (foundFieldPtr) {
			throw SemanticAnalysisException("The field '" + currentField->name + "' declared at " + currentField->locationString() + " uses the name already taken by the field declared at " + foundFieldPtr->locationString() + "' in category '" + categoryFoundIn->name);
		}

		VariablyTypedField* vtf = dynamic_cast<VariablyTypedField*>(currentField.get());
		if (vtf) {
			const MachineDefinition* mcm;
			std::shared_ptr<MachineStatement> mc = context.findMachineStatement(vtf->type, &mcm);
			if (!mc) {
				throw SemanticAnalysisException("The variably typed currentField '" + vtf->name + "' references type '" + vtf->type + "' at " + currentField->locationString() + ", but no such type (i.e. a category or production rule) could be found in the present context of the machine '" + context.name + "'");
			}

			vtf->machineOfTheType = mcm;
		}
	}
}

const IFileLocalizable* RuleStatement::findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto nEit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), targetName);
	if (nEit != namesEncountered.cend()) {
		return nullptr;
	}

	namesEncountered.push_back(this->name);

	auto ret = regex->findRecursiveReference(machine, namesEncountered, targetName);
	if (ret) {
		return ret;
	}

	namesEncountered.pop_back();
	return nullptr;
}

const IFileLocalizable* CategoryStatement::findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const {
	auto nEit = std::find(namesEncountered.cbegin(), namesEncountered.cend(), targetName);
	if (nEit != namesEncountered.cend()) {
		return nullptr;
	}

	namesEncountered.push_back(this->name);

	for (const auto& referencePair : this->references) {
		const IFileLocalizable* ret;
		if ((ret = referencePair.second.statement->findRecursiveReference(machine, namesEncountered, targetName)) != nullptr) {
			return ret;
		}
	}

	namesEncountered.pop_back();
	return nullptr;
}

std::list<const ProductionStatement*> CategoryStatement::calculateInstandingProductions() const {
	std::list<const ProductionStatement*> ret;

	for (const auto& referencePair : references) {
		auto instandingProductions = referencePair.second.statement->calculateInstandingProductions();
		ret.insert(ret.end(), instandingProductions.cbegin(), instandingProductions.cend());
	}

	return ret;
}

NFA CategoryStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void ProductionStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, true);
}

std::list<const ProductionStatement*> ProductionStatement::calculateInstandingProductions() const {
	return std::list<const ProductionStatement*>({ this });
}

NFA ProductionStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void PatternStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, true);
}

std::list<const ProductionStatement*> PatternStatement::calculateInstandingProductions() const {
	return std::list<const ProductionStatement*>(); // exactly what I want to return here - nothing
}

NFA PatternStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void RegexStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, false);
}

NFA RegexStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void TypeFormingStatement::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}