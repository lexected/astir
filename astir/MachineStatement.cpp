#include "MachineStatement.h"

#include "MachineDefinition.h"
#include "SemanticAnalysisException.h"
#include "NFABuilder.h"
#include "LLkFirster.h"
#include "LLkBuilder.h"
#include "LLkParserGenerator.h"
#include "GenerationVisitor.h"
#include "LRABuilder.h"

#include <algorithm>

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

IFileLocalizableCPtr CategoryStatement::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
	for (const auto& referencePair : this->references) {
		const IFileLocalizable* ret;
		if ((ret = referencePair.second.statement->findRecursiveReference(referencingEntitiesEncountered)) != nullptr) {
			return ret;
		}
	}

	return nullptr;
}

std::set<const AttributedStatement*> CategoryStatement::unpickReferal(const AttributedStatement* statement) const {
	if (!categoricallyRefersTo(statement)) {
		return std::set<const AttributedStatement*>({ this });
	} else {
		std::set<const AttributedStatement*> ret;
		for (const auto& referencePair : this->references) {
			if (referencePair.second.statement != statement) {
				auto subUnpicked = referencePair.second.statement->unpickReferal(statement);
				ret.insert(subUnpicked.cbegin(), subUnpicked.cend());
			}
		}
		return ret;
	}
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

SymbolGroupList CategoryStatement::first(LLkFirster* firster, const SymbolGroupList& prefix) const {
	return firster->visit(this, prefix);
}

void CategoryStatement::accept(LLkBuilder* llkBuilder) const {
	llkBuilder->visit(this);
}

void CategoryStatement::accept(LLkParserGenerator* generator) const {
	generator->visit(this);
}

void CategoryStatement::accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const {
	lraBuilder->visit(this, lra, startingState, lookahead);
}

bool CategoryStatement::categoricallyRefersTo(const AttributedStatement* statement) const {
	for (const auto& catRefPair : references) {
		if (catRefPair.second.statement->categoricallyRefersTo(statement)) {
			return true;
		}
	}

	return false;
}

void ProductionStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, true);
}

bool ProductionStatement::categoricallyRefersTo(const AttributedStatement* statement) const {
	return this == statement;
}

std::set<const AttributedStatement*> ProductionStatement::unpickReferal(const AttributedStatement* statement) const {
	return std::set<const AttributedStatement*>({ this });
}

std::list<const ProductionStatement*> ProductionStatement::calculateInstandingProductions() const {
	return std::list<const ProductionStatement*>({ this });
}

NFA ProductionStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void ProductionStatement::accept(LLkParserGenerator* generator) const {
	generator->visit(this);
}

void ProductionStatement::accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const {
	lraBuilder->visit(this, lra, startingState, lookahead);
}

void PatternStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, true);
}

bool PatternStatement::categoricallyRefersTo(const AttributedStatement* statement) const {
	return this == statement;
}

std::set<const AttributedStatement*> PatternStatement::unpickReferal(const AttributedStatement* statement) const {
	return std::set<const AttributedStatement*>({ this });
}

std::list<const ProductionStatement*> PatternStatement::calculateInstandingProductions() const {
	return std::list<const ProductionStatement*>(); // exactly what I want to return here - nothing
}

NFA PatternStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void PatternStatement::accept(LLkParserGenerator* generator) const {
	generator->visit(this);
}

void PatternStatement::accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const {
	lraBuilder->visit(this, lra, startingState, lookahead);
}

void RegexStatement::verifyContextualValidity(const MachineDefinition& machine) const {
	regex->checkAndTypeformActionUsage(machine, this, false);
}

NFA RegexStatement::accept(const NFABuilder& nfaBuilder) const {
	return nfaBuilder.visit(this);
}

void RegexStatement::accept(LLkParserGenerator* generator) const {
	generator->visit(this);
}

void RegexStatement::accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const {
	lraBuilder->visit(this, lra, startingState, lookahead);
}

bool TypeFormingStatement::isTypeForming() const {
	return true;
}

void TypeFormingStatement::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

std::string MachineStatement::referenceName() const {
	return this->name;
}

bool MachineStatement::isTypeForming() const {
	return false;
}

void RuleStatement::completeReferences(MachineDefinition& machine) const {
	regex->completeReferences(machine);
}

IFileLocalizableCPtr RuleStatement::findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const {
	auto ret = regex->findRecursiveReference(referencingEntitiesEncountered);
	if (ret) {
		return ret;
	}
	return nullptr;
}

void RuleStatement::accept(LLkBuilder* llkBuilder) const {
	llkBuilder->visit(this);
}

SymbolGroupList RuleStatement::first(LLkFirster* firster, const SymbolGroupList& prefix) const {
	return firster->visit(this, prefix);
}
