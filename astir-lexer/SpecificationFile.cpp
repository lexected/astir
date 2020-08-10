#include "SpecificationFile.h"

#include "Specification.h"

std::shared_ptr<MachineComponent> CategoryStatement::makeSpecificationEntity() const {
	return std::make_shared<Category>(this->name);
}

std::shared_ptr<MachineComponent> GrammarStatement::makeSpecificationEntity() const {
	bool recursionAllowed;
	bool typeForming;

	if (type == GrammarStatementType::Production) {
		recursionAllowed = true;
		typeForming = true;
	} else if (type == GrammarStatementType::Rule) {
		recursionAllowed = true;
		typeForming = false;
	} else if (type == GrammarStatementType::Token) {
		recursionAllowed = false;
		typeForming = true;
	} else if (type == GrammarStatementType::Regex) {
		recursionAllowed = false;
		typeForming = false;
	} else {
		throw SemanticAnalysisException("Unrecognized GrammarStatement type. This should never happen in practice.");
	}

	return std::make_shared<Rule>(this->name, recursionAllowed, typeForming, this->disjunction);
}

std::shared_ptr<Field> FlagFieldDeclaration::makeSpecificationEntity() const {
	return std::make_shared<FlagField>(name);
}

std::shared_ptr<Field> RawFieldDeclaration::makeSpecificationEntity() const {
	return std::make_shared<RawField>(name);
}

std::shared_ptr<Field> ItemFieldDeclaration::makeSpecificationEntity() const {
	return std::make_shared<ItemField>(name, type);
}

std::shared_ptr<Field> ListFieldDeclaration::makeSpecificationEntity() const {
	return std::make_shared<ListField>(name, type);
}

std::shared_ptr<Specification> SpecificationFile::makeSpecificationEntity() const {
	std::shared_ptr<Specification> specification = std::make_shared<Specification>();

	// build up the name database
	std::map<std::string, MachineDefinition*> machineDefinitions;
	for (const auto& statementPtr : statements) {
		MachineDefinition* md;
		if ((md = dynamic_cast<MachineDefinition*>(statementPtr.get())) != nullptr) {
			auto prevDefIt = machineDefinitions.find(md->name);
			if (prevDefIt != machineDefinitions.cend()) {
				throw SemanticAnalysisException("A machine with name '" + md->name + "' already defined", *prevDefIt->second);
			}
			machineDefinitions[md->name] = md;

			specification->machines[md->name] = md->makeSpecificationEntity();
		} else {
			throw SemanticAnalysisException("Unsupported statement type encountered", *statementPtr);
		}
	}

	// check for recursion
	std::list<std::string> namesEncountered;
	for (const auto& pair : machineDefinitions) {
		if (specification->containsMachineHierarchyRecursion(machineDefinitions, namesEncountered, pair.first)) {
			std::string hierarchyPath = namesEncountered.front();
			namesEncountered.pop_front();
			for (const auto& nameEncountered : namesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Definition recursion found in the mixed follow/extends hierarchy path " + hierarchyPath);
		}
	}

	// now add links between machines
	for (const auto& pair : machineDefinitions) {
		MachineDefinition* md = pair.second;

		specification->machines[pair.first]->extends = specification->machines[md->extends];
		specification->machines[pair.first]->follows = specification->machines[md->follows];
	}

	// and, finally, initialize all machines from the file, internally
	std::map<std::string, bool> initializationMap;
	for (const auto& pair : specification->machines) {
		initializeMachineWithDependencies(pair.second.get(), machineDefinitions, initializationMap);
	}

	return specification;
}

void SpecificationFile::initializeMachineWithDependencies(Machine* machine, const std::map<std::string, MachineDefinition*>& definitions, std::map<std::string, bool>& initializationMap) const {
	// here we simply make sure that the dependencies are initialized before the machine itself. Notice that since there are no recursions assumed the process is guaranteed to terminate.

	if (initializationMap[machine->name]) {
		return;
	}

	if (machine->extends) {
		auto extendsInitIt = initializationMap.find(machine->extends->name);
		if (!extendsInitIt->second) {
			initializeMachineWithDependencies(machine->extends.get(), definitions, initializationMap);
		}
	}

	if (machine->follows) {
		auto followsInitIt = initializationMap.find(machine->follows->name);
		if (!followsInitIt->second) {
			initializeMachineWithDependencies(machine->follows.get(), definitions, initializationMap);
		}
	}

	auto defIt = definitions.find(machine->name);
	defIt->second->initializeSpecificationEntity(machine);
	initializationMap[machine->name] = true;
}

void MachineDefinition::initializeSpecificationEntity(Machine* machine) const {
	// build up the name database
	std::map<std::string, MachineStatement*> statements;
	for (const auto& statementUPtr : this->statements) {
		if (machine->contextFindMachineComponent(statementUPtr->name)) {
			throw SemanticAnalysisException("Name '" + statementUPtr->name + "' encountered in machine '" + machine->name + "' already defined"); // TODO: could use an improvement; already defined where?
		}

		MachineStatement* statement = statementUPtr.get();
		statements[statement->name] = statement;

		machine->components[statement->name] = statement->makeSpecificationEntity();
	}

	// check for possible category reference recursion (and obviously whether the referenced category is indeed a category)
	std::list<std::string> namesEncountered;
	for (const auto& statementUPtr : this->statements) {
		if (machine->containsDeclarationCategoryRecursion(statements, namesEncountered, statementUPtr->name, false)) {
			std::string hierarchyPath = namesEncountered.front();
			namesEncountered.pop_front();
			for (const auto& nameEncountered : namesEncountered) {
				hierarchyPath += "-" + nameEncountered;
			}
			throw SemanticAnalysisException("Declaration recursion found in the category use declaration hierarchy path " + hierarchyPath);
		}
	}

	// at this point we need to initialize the MachineComponents, in particular their fields
	for (const auto& statementPair : statements) {
		statementPair.second->initializeSpecificationEntity(machine->components[statementPair.second->name].get());
	}

	// the last bit is to check that there is no disallowed recursion within the productions themselves
	std::list<std::string> namesEncounteredInComponentRecursion;
	if (machine->containsDisallowedComponentRecursion(namesEncounteredInComponentRecursion)) {
		std::string hierarchyPath = namesEncounteredInComponentRecursion.front();
		namesEncountered.pop_front();
		for (const auto& nameEncountered : namesEncounteredInComponentRecursion) {
			hierarchyPath += "-" + nameEncountered;
		}
		throw SemanticAnalysisException("Rule/category reference recursion found in the path " + hierarchyPath);
	}
}

std::shared_ptr<Machine> FADefinition::makeSpecificationEntity() const {
	return std::make_shared<FAMachine>(name, type);
}

void MachineStatement::initializeSpecificationEntity(MachineComponent* machineComponent) const {
	for (const auto& fieldPtr : fields) {
		machineComponent->fields.push_back(fieldPtr->makeSpecificationEntity());
	}
}

void MachineStatement::initializeSpecificationEntity(const Machine* context, MachineComponent* machineComponent) const {
	initializeSpecificationEntity(machineComponent);

	for (const auto& categoryName : this->categories) {
		auto categoryComponent = dynamic_cast<Category*>(context->contextFindMachineComponent(categoryName));
		machineComponent->categories.push_back(categoryComponent);
		categoryComponent->references[machineComponent->name] = machineComponent;
	}
}

bool RepetitiveRegex::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	return actionAtomicRegex->componentRecursivelyReferenced(machine, namesEncountered);
}

bool LookaheadRegex::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	return match->componentRecursivelyReferenced(machine, namesEncountered) || lookahead->componentRecursivelyReferenced(machine, namesEncountered);
}

bool ActionAtomicRegex::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	return regex->componentRecursivelyReferenced(machine, namesEncountered);
}

bool DisjunctiveRegex::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	for (const auto& conjunctiveRegexPtr : disjunction) {
		if (conjunctiveRegexPtr->componentRecursivelyReferenced(machine, namesEncountered)) {
			return true;
		}
	}

	return false;
}

bool ConjunctiveRegex::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	for (const auto& rootRegexPtr : conjunction) {
		if (rootRegexPtr->componentRecursivelyReferenced(machine, namesEncountered)) {
			return true;
		}
	}

	return false;
}

bool ReferenceRegex::componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
	return machine.componentRecursivelyReferenced(namesEncountered, *this);
}
