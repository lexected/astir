#pragma once

#include <list>
#include <memory>
#include <map>
#include <string>

#include "ISyntacticEntity.h"
#include "Regex.h"
#include "Field.h"
#include "NFA.h"
#include "ILLkFirstable.h"

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure. Hence also the choice of struct over class.
*/

struct UsesStatement;
struct MachineDefinition;
struct SyntacticTree : public ISyntacticEntity, public ISemanticEntity, public IGenerationVisitable {
	std::list<std::unique_ptr<UsesStatement>> usesStatements;
	std::map<std::string, std::shared_ptr<MachineDefinition>> machineDefinitions;

	// semantic bit
	void initialize() override;
	void completeMachineHierarchy(std::list<std::string>& namesEncountered, const std::shared_ptr<MachineDefinition>& machineDefinitionToComplete) const;

	// generation bit
	void accept(GenerationVisitor* visitor) const override;
};

struct UsesStatement : public ISyntacticEntity {
	std::string filePath;
};
