#pragma once

#include <list>
#include <memory>
#include <map>
#include <string>

#include "ISyntacticEntity.h"
#include "Regex.h"
#include "SemanticTree.h"
#include "Field.h"

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure. Hence also the choice of struct over class.
*/

template <class CorrespondingSpecificationType>
class ISemanticallyProcessable {
public:
	virtual ~ISemanticallyProcessable() = default;

	virtual std::shared_ptr<CorrespondingSpecificationType> makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<CorrespondingSpecificationType>>& ownershipPtr) const = 0; // why shared_ptr you ask? well, it may be the case that the AST will retain some ownership of the entity 'made' here as well, in which case unique_ptr would be unfeasible. at the same time, survival of the AST is not guaranteed, so a normal pointer would not work

protected:
	ISemanticallyProcessable() = default;
};

struct UsesStatement;
struct MachineDefinition;
struct SyntacticTree : public ISyntacticEntity, public ISemanticallyProcessable<SemanticTree> {
	std::list<std::unique_ptr<UsesStatement>> usesStatements;
	std::list<std::shared_ptr<MachineDefinition>> machineDefinitions;

	std::shared_ptr<SemanticTree> makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<SemanticTree>>& ownershipPtr) const override;
};

struct UsesStatement : public ISyntacticEntity {
	std::string filePath;
};

enum class MachineFlag {
	ProductionsTerminalByDefault,
	RulesProductionsByDefault,
	ProductionsRootByDefault,
	CategoriesRootByDefault
};

struct MachineDefinitionAttribute {
	bool set;
	bool value;

	MachineDefinitionAttribute()
		: set(false), value(false) { }

	MachineDefinitionAttribute(bool value)
		: set(false), value(value) { }
};

struct MachineStatement;
struct MachineDefinition : public ISyntacticEntity, public ISemanticallyProcessable<Machine> {
public:
	std::string name;
	std::map<MachineFlag, MachineDefinitionAttribute> attributes;
	std::list<std::string> uses;
	std::string on;
	std::list<std::shared_ptr<MachineStatement>> statements;

	MachineDefinition()
		: attributes({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::RulesProductionsByDefault, MachineDefinitionAttribute(true) }
			}) { }
	MachineDefinition(const std::map<MachineFlag, MachineDefinitionAttribute>& attributes)
		: attributes(attributes) { }
	// TODO: instead of setting the attribute map hard, combine the two maps with values overriden by the incoming when duplicate
};


struct FiniteAutomatonDefinition : public MachineDefinition {
	FiniteAutomatonDefinition()
		:  MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::RulesProductionsByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
			}) { }

	std::shared_ptr<Machine> makeSemanticEntity(const std::shared_ptr<ISemanticallyProcessable<Machine>>& ownershipPtr) const override;
};

enum class Rootness {
	AcceptRoot,
	IgnoreRoot,
	Unspecified
};

enum class Terminality {
	Terminal,
	Nonterminal,
	Unspecified
};

struct MachineStatement : public ISyntacticEntity {
	std::string name;
	virtual ~MachineStatement() = default;

protected:
	MachineStatement() = default;
	MachineStatement(const std::string& name)
		: name(name) { }
};

struct AttributedStatement : public virtual MachineStatement {
	std::list<std::string> categories;
	std::list<std::shared_ptr<Field>> fields;
};

struct TypeFormingStatement : public AttributedStatement {
	Rootness rootness;

protected:
	TypeFormingStatement()
		: rootness(Rootness::Unspecified) { }
	TypeFormingStatement(Rootness rootness)
		: rootness(rootness) { }
};

struct RuleStatement : public virtual MachineStatement {
	std::shared_ptr<DisjunctiveRegex> ruleRegex;
};

struct CategoryStatement : public TypeFormingStatement { };

struct ProductionStatement : public TypeFormingStatement, public RuleStatement {
	Terminality terminality;

	ProductionStatement()
		: terminality(Terminality::Unspecified) { }
};

struct PatternStatement : public AttributedStatement, public RuleStatement { };

struct RegexStatement : public RuleStatement { };