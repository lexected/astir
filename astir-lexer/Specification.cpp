#include "Specification.h"


void Specification::initializeFromFile(const SpecificationFile& specificationFile) {
	// build up the name database
	std::map<std::string, MachineDefinition*> machineDefinitions;
	for (const auto & statementPtr: specificationFile.statements) {
		MachineDefinition* md;
		if ((md = dynamic_cast<MachineDefinition*>(statementPtr.get())) != nullptr) {
			auto prevDefIt = machineDefinitions.find(md->machineName);
			if (prevDefIt != machineDefinitions.cend()) {
				throw SemanticAnalysisException("A machine with name '" + md->machineName + "' already defined", *prevDefIt->second);
			}
			machineDefinitions[md->machineName] = md;

			auto machinePtr = std::make_shared<Machine>(md->machineName);
			machines[md->machineName] = move(machinePtr);
		} else {
			throw SemanticAnalysisException("Unsupported statement type encountered", *statementPtr);
		}
	}

	// check for recursion
	std::list<std::string> namesEncountered;
	for (const auto& pair : machineDefinitions) {
		if (containsDeclarationCategoryRecursion(machineDefinitions, namesEncountered, pair.first)) {
			std::string hierarchyPath = namesEncountered.front();
			namesEncountered.pop_front();
			for (const auto& nameEncountered : namesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Definition recursion found in the mixed follow/extends hierarchy path " + hierarchyPath);
		}
	}

	// now add links between machines
	for (const auto & pair : machineDefinitions) {
		MachineDefinition* md = pair.second;
		
		machines[pair.first]->extends = machines[md->extends];
		machines[pair.first]->follows = machines[md->follows];
	}

	// and, finally, initialize all machines from the file, internally
	for (const auto& pair : machineDefinitions) {
		MachineDefinition* md = pair.second;

		machines[pair.first]->initializeFromDefinition(md);
	}
}

bool Specification::containsDeclarationCategoryRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const {

	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	const MachineDefinition* definitionConsidered = definitions.find(nameConsidered)->second;
	if (containsDeclarationCategoryRecursion(definitions, namesEncountered, definitionConsidered->follows)) {
		return true;
	}

	if (containsDeclarationCategoryRecursion(definitions, namesEncountered, definitionConsidered->extends)) {
		return true;
	}

	namesEncountered.pop_back();
	return false;
}

void Specification::initializeMachineWithDependencies(Machine& machine, const std::map<std::string, MachineDefinition*>& definitions, std::map<std::string, bool> & initializationMap) const {
	// here we simply make sure that the dependencies are initialized before the machine itself. Notice that since there are no recursions the process is guaranteed to terminate.

	if(machine.extends) {
		auto extendsInitIt = initializationMap.find(machine.extends->name);
		if (!extendsInitIt->second) {
			initializeMachineWithDependencies(*machine.extends, definitions, initializationMap);
		}
	}

	if(machine.follows) {
		auto followsInitIt = initializationMap.find(machine.follows->name);
		if (!followsInitIt->second) {
			initializeMachineWithDependencies(*machine.follows, definitions, initializationMap);
		}
	}

	auto defIt = definitions.find(machine.name);
	machine.initializeFromDefinition(defIt->second);
	initializationMap[machine.name] = true;
}

void Machine::initializeFromDefinition(const MachineDefinition* definition) {
	// build up the name database
	std::map<std::string, MachineStatement*> statements;
	for (const auto& statementUPtr : definition->statements) {
		if (contextFindMachineEntity(statementUPtr->name)) {
			throw SemanticAnalysisException("Name '" + statementUPtr->name + "' already defined"); // TODO: could use an improvement; already defined where?
		}

		MachineStatement* statement = statementUPtr.get();
		statements[statement->name] = statement;
		GrammarStatement* gs;
		std::shared_ptr<MachineEntity> entity;
		if (dynamic_cast<CategoryStatement*>(statement) != nullptr) {
			entity = std::make_shared<Category>();
		} else if ((gs = dynamic_cast<GrammarStatement*>(statement)) != nullptr) {
			auto production = std::make_shared<Production>();
			if (gs->type == GrammarStatementType::Production) {
				production->recursionAllowed = true;
				production->typeForming = true;
			} else if (gs->type == GrammarStatementType::Rule) {
				production->recursionAllowed = true;
				production->typeForming = false;
			} else if (gs->type == GrammarStatementType::Token) {
				production->recursionAllowed = false;
				production->typeForming = true;
			} else if (gs->type == GrammarStatementType::Regex) {
				production->recursionAllowed = false;
				production->typeForming = false;
			} else {
				throw SemanticAnalysisException("Unrecognized GrammarStatement type. This should never happen in practice.");
			}
			entity = production;
		} else {
			throw SemanticAnalysisException("Unrecognized machine statement. This should never happen in practice.");
		}

		entity->name = statement->name;

		entities[entity->name] = entity;
	}

	// check for possible category reference recursion (and obviously whether the referenced category is indeed a category)
	std::list<std::string> namesEncountered;
	for (const auto& statementUPtr : definition->statements) {
		if (containsDeclarationCategoryRecursion(statements, namesEncountered, statementUPtr->name, false)) {
			std::string hierarchyPath = namesEncountered.front();
			namesEncountered.pop_front();
			for (const auto& nameEncountered : namesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Declaration recursion found in the category use declaration hierarchy path " + hierarchyPath);
		}
	}

	// at this point we need to build the initialize the MachineEntities, in particular their fields

	// the last bit is to check that there is no disallowed recursion within the productions themselves. Good news is that we no longer have to check when a category is referenced, as a category can not, on its own, reference a production
}

std::shared_ptr<MachineEntity> Machine::contextFindMachineEntity(const std::string& name) const {
	std::shared_ptr<MachineEntity> ret;
	if (extends && (ret = extends->contextFindMachineEntity(name))) {
		return ret;
	}
	
	if (follows && (ret = follows->contextFindMachineEntity(name))) {
		return ret;
	}

	auto it = entities.find(name);
	if (it != entities.cend()) {
		return it->second;
	}

	return nullptr;
}

bool Machine::containsDeclarationCategoryRecursion(const std::map<std::string, MachineStatement*>& statements, std::list<std::string>& namesEncountered, std::string nameConsidered, bool mustBeACategory) const {
	bool retNow = std::find(namesEncountered.cbegin(), namesEncountered.cend(), nameConsidered) == namesEncountered.cend();
	namesEncountered.push_back(nameConsidered);
	if (retNow) {
		return true;
	}

	const MachineStatement* statementConsidered = statements.find(nameConsidered)->second;
	if (mustBeACategory && dynamic_cast<const CategoryStatement*>(statementConsidered) == nullptr) {
		throw SemanticAnalysisException(nameConsidered + "' is not a category but was referenced as a category in the declaration of " + namesEncountered.back());
	}

	for(const std::string& categoryReferenced : statementConsidered->categories) {
		if (containsDeclarationCategoryRecursion(statements, namesEncountered, categoryReferenced, true)) {
			return true;
		}
	}

	namesEncountered.pop_back();
	return false;
}
