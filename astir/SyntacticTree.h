#pragma once

#include <list>
#include <memory>
#include <map>
#include <string>

#include "ISyntacticEntity.h"
#include "Regex.h"
#include "Field.h"
#include "NFA.h"

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure. Hence also the choice of struct over class.
*/

struct UsesStatement;
struct MachineDefinition;
struct SyntacticTree : public ISyntacticEntity, public ISemanticEntity, public IGenerationVisitable{
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

enum class MachineFlag {
	ProductionsTerminalByDefault,
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

using TerminalTypeIndex = size_t;

struct MachineStatement;
struct AttributedStatement;
struct ProductionStatement;
struct TypeFormingStatement;
struct AttributedStatement;
struct MachineDefinition : public ISyntacticEntity, public ISemanticEntity, public IGenerationVisitable {
public:
	std::string name;
	std::map<MachineFlag, MachineDefinitionAttribute> attributes;
	std::map<std::string, std::shared_ptr<MachineDefinition>> uses;
	std::pair<std::string, std::shared_ptr<MachineDefinition>> on;
	std::map<std::string, std::shared_ptr<MachineStatement>> statements;

	void initialize() override;

	std::shared_ptr<MachineStatement> findMachineStatement(const std::string& name, const MachineDefinition** sourceMachine = nullptr) const; // anyone calling this function shall not take up even a partial ownership of the component, normal pointer suffices
	std::list<std::shared_ptr<ProductionStatement>> getTerminalProductions() const;
	std::list<std::shared_ptr<TypeFormingStatement>> getTypeFormingStatements() const;
	bool hasPurelyTerminalRoots() const;
	std::list<std::shared_ptr<TypeFormingStatement>> getRoots() const;
	std::list<const ProductionStatement*> getUnderlyingProductionsOfRoots() const;
	std::list<std::shared_ptr<ProductionStatement>> getTerminalRoots() const;
	TerminalTypeIndex terminalProductionCount() const { return m_terminalCount; };

	void completeCategoryReferences(std::list<std::string> namesEncountered, const std::shared_ptr<AttributedStatement>& attributedStatement, bool mustBeACategory = false) const;
	const IFileLocalizable* findRecursiveReferenceThroughName(const std::string& referenceName, std::list<std::string>& namesEncountered, const std::string& targetName) const;

protected:
	MachineDefinition()
		: attributes({
			{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
			{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(false) },
			{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) }
		}), m_terminalCount((TerminalTypeIndex)0) { }
		
	MachineDefinition(const std::map<MachineFlag, MachineDefinitionAttribute>& attributes)
		: attributes(attributes), m_terminalCount((TerminalTypeIndex)0) { }
	// TODO: instead of setting the attribute map hard, combine the two maps with values overriden by the incoming when duplicate

private:
	TerminalTypeIndex m_terminalCount;
};

struct FiniteAutomatonDefinition : public MachineDefinition {
	FiniteAutomatonDefinition()
		:  MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
			}) { }

	void initialize() override;

	const NFA& getNFA() const { return m_nfa; }

	void accept(GenerationVisitor* visitor) const override;
private:
	std::shared_ptr<const FiniteAutomatonDefinition> m_finiteAutomatonDefinition;
	NFA m_nfa;
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

struct MachineStatement : public ISyntacticEntity, public ISemanticEntity, public IProductionReferencable, public INFABuildable {
	std::string name;
	virtual ~MachineStatement() = default;

protected:
	MachineStatement() = default;
	MachineStatement(const std::string& name)
		: name(name) { }
};

struct CategoryStatement;
struct AttributedStatement : public virtual MachineStatement {
	std::map<std::string, std::shared_ptr<CategoryStatement>> categories;
	std::list<std::shared_ptr<Field>> fields;

	std::shared_ptr<Field> findField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const;
	void completeFieldDeclarations(MachineDefinition& context) const;

	virtual std::list<const ProductionStatement*> calculateInstandingProductions() const = 0;

private:
	std::shared_ptr<Field> findCategoryField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const;
};

struct TypeFormingStatement : public AttributedStatement, public IGenerationVisitable {
	Rootness rootness;

	void accept(GenerationVisitor* visitor) const override;
protected:
	TypeFormingStatement()
		: rootness(Rootness::Unspecified) { }
	TypeFormingStatement(Rootness rootness)
		: rootness(rootness) { }
};

struct RuleStatement : public virtual MachineStatement {
	std::shared_ptr<DisjunctiveRegex> regex;

	const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const override;

	virtual void verifyContextualValidity(const MachineDefinition& machine) const = 0;
};

struct CategoryReference {
	const AttributedStatement* statement;
	bool isAReferenceFromUnderlyingMachine; // well, if you put it this way, it really does sounds stupid!

	CategoryReference()
		: statement(nullptr), isAReferenceFromUnderlyingMachine(false) { }
	CategoryReference(const MachineStatement* component, bool isAReferenceFromUnderlyingMachine)
		: statement(statement), isAReferenceFromUnderlyingMachine(isAReferenceFromUnderlyingMachine) { }
};

struct CategoryStatement : public TypeFormingStatement {
	std::map<std::string, CategoryReference> references; // references to 'me',  i.e. by other machine components. Non-owning pointers so ok.

	const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const override;

	std::list<const ProductionStatement*> calculateInstandingProductions() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct ProductionStatement : public TypeFormingStatement, public RuleStatement {
	Terminality terminality;
	TerminalTypeIndex terminalTypeIndex;

	ProductionStatement()
		: terminality(Terminality::Unspecified), terminalTypeIndex(0) { }

	void verifyContextualValidity(const MachineDefinition& machine) const override;

	std::list<const ProductionStatement*> calculateInstandingProductions() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct PatternStatement : public AttributedStatement, public RuleStatement {
	void verifyContextualValidity(const MachineDefinition& machine) const override;

	std::list<const ProductionStatement*> calculateInstandingProductions() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct RegexStatement : public RuleStatement {
	void verifyContextualValidity(const MachineDefinition& machine) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};