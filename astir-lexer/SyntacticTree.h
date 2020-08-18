#pragma once

#include <list>
#include <memory>
#include <map>
#include <string>

#include "ISyntacticEntity.h"
#include "Regex.h"
#include "SemanticTree.h"

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure. Hence also the choice of struct over class.
*/

template <class CorrespondingSpecificationType>
class ISemanticallyProcessable {
public:
	virtual ~ISemanticallyProcessable() = default;

	virtual std::shared_ptr<CorrespondingSpecificationType> makeSemanticEntity() const = 0; // why shared_ptr you ask? well, it may be the case that the AST will retain some ownership of the entity 'made' here as well, in which case unique_ptr would be unfeasible. at the same time, survival of the AST is not guaranteed, so a normal pointer would not work

protected:
	ISemanticallyProcessable() = default;
};

struct UsesStatement;
struct MachineDefinition;
struct SyntacticTree : public ISyntacticEntity, public ISemanticallyProcessable<SemanticTree> {
	std::list<std::unique_ptr<UsesStatement>> usesStatements;
	std::list<std::shared_ptr<MachineDefinition>> machineDefinitions;

	std::shared_ptr<SemanticTree> makeSemanticEntity() const override;
};

struct UsesStatement : public ISyntacticEntity {
	std::string filePath;
};

enum class MachineFlag {
	GroupedStringLiterals
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
	std::string follows;
	std::list<std::shared_ptr<CategoryStatement>> categoryStatements;
	std::list<std::shared_ptr<RuleStatement>> ruleStatements;

	MachineDefinition()
		: attributes({
				{ MachineFlag::GroupedStringLiterals, MachineDefinitionAttribute(false) }
			}) { }
};

struct FiniteAutomatonDefinition : public MachineDefinition {
	FiniteAutomatonType type;
	
	FiniteAutomatonDefinition()
		:  MachineDefinition(), type(FiniteAutomatonType::Nondeterministic) { }

	std::shared_ptr<Machine> makeSemanticEntity() const override;
};

enum class RuleStatementType {
	Pattern,
	Production
};

struct Field;
struct MachineStatement : public ISyntacticEntity {
	std::string name;
	std::list<std::string> categories;
	std::list<std::shared_ptr<Field>> fields;
	
	virtual ~MachineStatement() = default;
};

struct CategoryStatement : public MachineStatement, public ISemanticallyProcessable<Category> {
	std::shared_ptr<Category> makeSemanticEntity() const override;
};

struct RuleStatement : public MachineStatement, public ISemanticallyProcessable<Rule> {
	bool terminalitySpecified;
	bool terminality;
	bool typeSpecified;
	RuleStatementType type;
	std::shared_ptr<DisjunctiveRegex> disjunction;

	RuleStatement()
		: terminalitySpecified(false), terminality(false), typeSpecified(false), type(RuleStatementType::Production) { }

	std::shared_ptr<Rule> makeSemanticEntity() const override;
};

struct Field : public ISyntacticEntity {
	std::string name;

	virtual bool flaggable() const = 0;
	virtual bool settable() const = 0;
	virtual bool listable() const = 0;
};

struct FlagField : public Field {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;
};

struct RawField : public Field {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;
};

struct VariablyTypedField : public Field {
	std::string type;
};

struct ItemField : public VariablyTypedField {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;
};

struct ListField : public VariablyTypedField {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;
};