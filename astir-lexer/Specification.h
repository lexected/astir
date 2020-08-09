#pragma once

#include <map>
#include <string>
#include <memory>
#include <list>

#include "Exception.h"

/*
	A few forward declarations
*/

struct MachineStatement;
struct MachineDefinition;
struct ParsedStructure;

class SemanticAnalysisException : public Exception {
public:
	SemanticAnalysisException(const std::string& message)
		: Exception(message) { }
	SemanticAnalysisException(const std::string& message, const ParsedStructure& parsedStructureToInferLocationFrom);
};

template <class CorrespondingSpecificationType>
class ISpecificationInitializable {
public:
	virtual ~ISpecificationInitializable() = default;

	virtual std::shared_ptr<CorrespondingSpecificationType> makeSpecificationComponent() const = 0;
	virtual void initializeSpecificationComponent(CorrespondingSpecificationType* specificationComponent) const { };

protected:
	ISpecificationInitializable() = default;
};

class Machine;
class Specification {
public:
	std::map<std::string, std::shared_ptr<Machine>> machines;

	bool containsDeclarationCategoryRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const;
};

class MachineEntity;
class Category;
class Production;
class Machine {
public:
	const std::string name;
	std::shared_ptr<Machine> follows;
	std::shared_ptr<Machine> extends;
	std::map<std::string, std::shared_ptr<MachineEntity>> entities;

	Machine(const std::string& name)
		: name(name) { }

	std::shared_ptr<MachineEntity> contextFindMachineEntity(const std::string& name) const;
	bool containsDeclarationCategoryRecursion(const std::map<std::string, MachineStatement*>& statements, std::list<std::string>& namesEncountered, std::string nameConsidered, bool mustBeACategory = false) const;
};

enum class FAType {
	Deterministic,
	Nondeterministic
};

class FAMachine : public Machine {
public:
	const FAType type;

	FAMachine(const std::string& name, FAType type)
		: Machine(name), type(type) { }
};

class Field;
class MachineEntity {
public:
	const std::string name;
	std::shared_ptr<Category> categories;
	std::list<std::shared_ptr<Field>> fields;

	MachineEntity(const std::string& name)
		: name(name) { }
};

class Category : public MachineEntity {
public:
	Category(const std::string& name)
		: MachineEntity(name) { }
};

class Production : public MachineEntity {
public:
	const bool recursionAllowed;
	const bool typeForming;

	Production(const std::string& name, bool recursionAllowed, bool typeForming)
		: MachineEntity(name), recursionAllowed(recursionAllowed), typeForming(typeForming) { }
};

class Field {
public:
	const std::string name;

	Field(const std::string& name)
		: name(name) { }

	virtual ~Field() = default;
};

class FlagField : public Field {
public:
	FlagField(const std::string& name)
		: Field(name) { }
};

class RawField : public Field {
public:
	RawField(const std::string& name)
		: Field(name) { }
};

class VariablyTypedField : public Field {
public:
	const std::string type;

	VariablyTypedField(const std::string& name, const std::string& type)
		: Field(name), type(type) { }
};

class ItemField : public VariablyTypedField {
public:
	ItemField(const std::string& name, const std::string& type)
		: VariablyTypedField(name, type) { }
};

class ListField : public VariablyTypedField {
public:
	ListField(const std::string& name, const std::string& type)
		: VariablyTypedField(name, type) { }
};

