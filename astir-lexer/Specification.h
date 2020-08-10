#pragma once

#include <map>
#include <string>
#include <memory>
#include <list>

#include "Exception.h"

/*
	A few forward declarations
*/
struct ParsedStructure;
struct MachineStatement;
struct MachineDefinition;
struct DisjunctiveRegex;


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

	virtual std::shared_ptr<CorrespondingSpecificationType> makeSpecificationEntity() const = 0; // why shared_ptr you ask? well, it may be the case that the AST will retain some ownership of the entity 'made' here as well, in which case unique_ptr would be unfeasible. at the same time, survival of the AST is not guaranteed, so a normal pointer would not work
	virtual void initializeSpecificationEntity(CorrespondingSpecificationType* specificationEntity) const { };

protected:
	ISpecificationInitializable() = default;
};

class Machine;
class IProductionReferencable {
public:
	virtual ~IProductionReferencable() = default;

	virtual bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const {
		return false;
	}
protected:
	IProductionReferencable() = default;
};

class Specification {
public:
	std::map<std::string, std::shared_ptr<Machine>> machines; // maybe could be made into a unique_ptr <- for further review

	bool containsMachineHierarchyRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const;
};

class MachineComponent;
class Category;
class Rule;
struct ReferenceRegex;
class Machine {
public:
	const std::string name;
	std::shared_ptr<Machine> follows; // ownership is shared.. in theory just a normal pointer to Machine would be good as specifications own the pointers, but OK - can be resolved later
	std::shared_ptr<Machine> extends; // ownership is shared
	std::map<std::string, std::shared_ptr<MachineComponent>> components;

	Machine(const std::string& name)
		: name(name) { }

	MachineComponent* contextFindMachineComponent(const std::string& name) const; // anyone calling this function shall not take up even a partial ownership of the component! normal pointer suffices
	bool containsDeclarationCategoryRecursion(const std::map<std::string, MachineStatement*>& statements, std::list<std::string>& namesEncountered, std::string nameConsidered, bool mustBeACategory = false) const;
	bool componentRecursivelyReferenced(std::list<std::string>& namesEncountered, const ReferenceRegex& componentReference) const;
	virtual bool containsDisallowedComponentRecursion(std::list<std::string>& namesEncountered) const = 0;
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

	bool containsDisallowedComponentRecursion(std::list<std::string>& namesEncountered) const override;
};

class Field;
class MachineComponent : public IProductionReferencable {
public:
	const std::string name;
	std::list<const Category*> categories; // non-owning pointers for the categories
	std::list<std::shared_ptr<Field>> fields; // owning pointers for the fields <- I tried with unique_ptr, but it does not fit the narrative of the ISpecificationInitializable methods that return shared_ptrs. So shared_ptr it is because narrative...

	MachineComponent(const std::string& name)
		: name(name) { }

	virtual bool containsDisallowedComponentRecursion(const Machine& machine, std::list<std::string>& namesEncountered, bool isAllRecursionDisallowed = false) const = 0;
};

class Category : public MachineComponent {
public:
	Category(const std::string& name)
		: MachineComponent(name) { }

	std::map<std::string, const MachineComponent*> references; // references to 'me',  i.e. by other machine components. Non-owning pointers so ok.

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
	bool containsDisallowedComponentRecursion(const Machine& machine, std::list<std::string>& namesEncountered, bool isAllRecursionDisallowed = false) const override;
};

class Rule : public MachineComponent {
public:
	const bool terminal;
	const bool typeForming;

	std::shared_ptr<DisjunctiveRegex> regex; // shared ptr really really appropriate here

	Rule(const std::string& name, bool terminal, bool typeForming, const std::shared_ptr<DisjunctiveRegex>& regex)
		: MachineComponent(name), terminal(terminal), typeForming(typeForming), regex(regex) { }

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
	bool containsDisallowedComponentRecursion(const Machine& machine, std::list<std::string>& namesEncountered, bool isAllRecursionDisallowed = false) const override;
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
