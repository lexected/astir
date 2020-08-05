#pragma once

#include <list>
#include <memory>
#include <map>

template <class ProductionType>
using StandardList = std::list<std::unique_ptr<ProductionType>>;

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure.
*/

struct MachineDefinition;
struct Specification {
	StandardList<MachineDefinition> machineDefinitions;
};

struct Statement;
struct MachineDefinition {
	std::string machineName;
	StandardList<Statement> statements;
	std::string extends;
	std::string follows;

	MachineDefinition() = default;

	virtual ~MachineDefinition() = default;
};

enum class FAType {
	Deterministic,
	Nondeterministic
};

enum class FAFlag {
	GroupedStringLiterals,
	TableLookup
};

struct FADefinition : public MachineDefinition {
	FAType type;
	std::map<FAFlag, bool> attributes;

	FADefinition()
		:  MachineDefinition(), attributes({
			{ FAFlag::GroupedStringLiterals, false },
			{ FAFlag::TableLookup, false }
		}) { }

	virtual ~FADefinition() = default;
};

struct Statement {

};