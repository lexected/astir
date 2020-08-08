#pragma once

#include <map>
#include <string>
#include <memory>
#include <list>

#include "Exception.h"
#include "SpecificationFile.h"

class SemanticAnalysisException : public Exception {
public:
	SemanticAnalysisException(const std::string& message)
		: Exception(message) { }
	SemanticAnalysisException(const std::string& message, const ParsedStructure& parsedStructureToInferLocationFrom)
		: Exception(message + parsedStructureToInferLocationFrom.locationString()) { }
};

class Machine;
class Specification {
public:
	std::map<std::string, std::shared_ptr<Machine>> machines;

	void initializeFromFile(const SpecificationFile& specificationFile);
private:
	bool containsDeclarationCategoryRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const;

	void initializeMachineWithDependencies(Machine& machine, const std::map<std::string, MachineDefinition*>& definitions, std::map<std::string, bool> & initializationMap) const;
};

class MachineEntity;
class Machine {
public:
	const std::string name;

	std::shared_ptr<Machine> follows;
	std::shared_ptr<Machine> extends;

	std::map<std::string, std::shared_ptr<MachineEntity>> entities;

	Machine(const std::string& name)
		: name(name) { }

	virtual void initializeFromDefinition(const MachineDefinition* definition);
private:
	std::shared_ptr<MachineEntity> contextFindMachineEntity(const std::string& name) const;

	bool containsDeclarationCategoryRecursion(const std::map<std::string, MachineStatement*>& statements, std::list<std::string>& namesEncountered, std::string nameConsidered, bool mustBeACategory = false) const;
};

class Field;
class Category;
class MachineEntity {
public:
	std::string name;
	std::shared_ptr<Category> categories;
	std::list<std::shared_ptr<Field>> fields;

	virtual void initializeFromDefinition(const MachineStatement* statement);
};

class Category : public MachineEntity { };

class Production : public MachineEntity {
public:
	bool recursionAllowed;
	bool typeForming;

	void initializeFromDefinition(const MachineStatement* statement) override;
};

class Field {
public:
	std::string name;

	virtual ~Field() = default;
};

class FlagField : public Field {

};

class RawField : public Field {

};

class VariablyTypedField : public Field {
	std::string type;
};

class ItemField : public VariablyTypedField {

};

class ListField : public VariablyTypedField {


};

