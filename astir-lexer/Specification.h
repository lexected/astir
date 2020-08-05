#pragma once

#include <list>
#include <memory>
#include <map>

template <class ProductionType>
using StandardList = std::list<std::unique_ptr<ProductionType>>;

struct MachineDefinition;
struct Specification {
	StandardList<MachineDefinition> machineDefinitions;

	Specification(const StandardList<MachineDefinition>& defs)
		: machineDefinitions(defs) { }
};

struct Statement;
struct MachineDefinition {
	std::string machineName;
	StandardList<Statement> statements;
	std::string extends;
	std::string follows;

	MachineDefinition(const std::string& name, const StandardList<Statement>& statements, const std::string& extends, const std::string& follows)
		: machineName(name), statements(statements), extends(extends), follows(follows) { }
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

	FADefinition(FAType type, const std::string& name, const StandardList<Statement>& statements, const std::string& extends, const std::string& follows)
		: type(type), MachineDefinition(name, statements, extends, follows), attributes({ 
			{ FAFlag::GroupedStringLiterals, false },
			{ FAFlag::TableLookup, false } 
		}) { }
};

struct Statement {

};