#pragma once

#include <map>
#include <string>
#include <memory>
#include <list>

#include "Exception.h"
#include "ISyntacticEntity.h"
#include "ISemanticEntity.h"
#include "IActing.h"

#include "NFA.h"
#include "INFABuildable.h"

#include "IGenerationVisitable.h"

/*
	A few forward declarations
*/

struct SyntacticTree;
struct MachineDefinition;
struct FiniteAutomatonDefinition;
struct MachineStatement;
struct CategoryStatement;
struct RuleStatement;
struct DisjunctiveRegex;
struct Field;

/*
	The main exception type
*/

class SemanticAnalysisException : public Exception {
public:
	SemanticAnalysisException(const std::string& message)
		: Exception(message) { }
	SemanticAnalysisException(const std::string& message, const IFileLocalizable& somethingLocalizableToPinpointLocationBy);
};

/*
	The core of the file
*/

class SemanticTree : public ISemanticEntity, public IGenerationVisitable {
public:
	std::map<std::string, std::shared_ptr<Machine>> machines; // maybe could be made into a unique_ptr <- for further review

	void checkForMachineHierarchyRecursion(std::list<std::string>& namesEncountered, const std::string& nameConsidered) const;

	SemanticTree(const std::shared_ptr<const SyntacticTree>& syntacticTree)
		: m_syntacticTree(syntacticTree) { }

	std::shared_ptr<const ISyntacticEntity> underlyingSyntacticEntity() const override;

	void initialize() override;

	void accept(GenerationVisitor* visitor) const override;
private:
	std::shared_ptr<const SyntacticTree> m_syntacticTree;
};

using TerminalTypeIndex = size_t;

class Machine;
class MachineComponent;
class Category;
class Rule;
struct ReferenceRegex;
class Machine : public ISemanticEntity, public IGenerationVisitable {
public:
	const std::string name;
	std::shared_ptr<Machine> on; // ownership is shared.. in theory just a normal pointer to Machine would be lastApplicationSuccessful as specifications own the pointers, but OK - can be resolved later
	std::list<std::shared_ptr<Machine>> uses; // ownership is shared
	std::map<std::string, std::shared_ptr<MachineComponent>> components;

	Machine(const std::string& name)
		: name(name), m_terminalCount((TerminalTypeIndex)0) { }

	void initialize() override;

	MachineComponent* findMachineComponent(const std::string& name, const Machine** sourceMachine = nullptr) const; // anyone calling this function shall not take up even a partial ownership of the component, normal pointer suffices
	std::list<const MachineComponent*> getTerminalComponents() const;
	std::list<const MachineComponent*> getTerminalTypeComponents() const;
	std::list<const MachineComponent*> getTypeComponents() const;
	bool hasPurelyTerminalRoots() const;
	std::list<const MachineComponent*> getRoots() const;
	std::list<const Production*> getProductionRoots() const;
	std::list<const Production*> getTerminalRoots() const;
	TerminalTypeIndex terminalProductionCount() const { return m_terminalCount; };

	void checkForDeclarationCategoryRecursion(std::list<std::string>& namesEncountered, const std::string& nameConsidered, const IFileLocalizable& occurence, bool mustBeACategory = false) const;
	const IFileLocalizable* findRecursiveReferenceThroughName(const std::string& referenceName, std::list<std::string>& namesEncountered, const std::string& targetName) const;
	virtual void checkForComponentRecursion() const = 0;

private:
	TerminalTypeIndex m_terminalCount;
};

class FiniteAutomatonMachine : public Machine {
public:
	FiniteAutomatonMachine(const std::shared_ptr<const FiniteAutomatonDefinition>& machineDefinition, const std::string& name)
		: Machine(name), m_finiteAutomatonDefinition(machineDefinition) { }

	void checkForComponentRecursion() const override;
	std::shared_ptr<const ISyntacticEntity> underlyingSyntacticEntity() const override;
	void initialize() override;

	const NFA& getNFA() const { return m_nfa; }

	void accept(GenerationVisitor* visitor) const override;
private:
	std::shared_ptr<const FiniteAutomatonDefinition> m_finiteAutomatonDefinition;
	NFA m_nfa;
};

class Production;
class MachineComponent : public ISemanticEntity, public IProductionReferencable, public INFABuildable, public IGenerationVisitable {
public:
	const std::string name;
	std::list<const Category*> categories; // non-owning pointers for the categories
	std::list<std::shared_ptr<Field>> fields; // owning pointers for the fields <- I tried with unique_ptr, but it does not fit the narrative of the ISpecificationInitializable methods that return shared_ptrs. So shared_ptr it is because narrative...

	void initialize() override;
	void checkFieldName(Machine& context, const Field* field) const;
	void checkAndTypeformFieldDeclarations(Machine& context) const;
	const Field* findField(const std::string& name) const;

	virtual bool entails(const std::string& name) const = 0;
	virtual bool entails(const std::string& name, std::list<const Category*>& path) const = 0;

	virtual bool isTypeForming() const = 0;
	virtual bool isTerminal() const = 0;
	virtual bool isRoot() const = 0;
	virtual bool isIgnored() const = 0;

	virtual void verifyContextualValidity(const Machine& machine) const;
	void accept(GenerationVisitor* visitor) const override;

	virtual std::list<const Production*> calculateInstandingProductions() const = 0;

protected:
	MachineComponent(const std::string& name)
		: name(name) { }
};

struct CategoryReference {
	const MachineComponent* component;
	bool isAReferenceFromUnderlyingMachine; // well, if you put it this way, it really does sounds stupid!

	CategoryReference()
		: component(nullptr), isAReferenceFromUnderlyingMachine(false) { }
	CategoryReference(const MachineComponent* component, bool isAReferenceFromUnderlyingMachine)
		: component(component), isAReferenceFromUnderlyingMachine(isAReferenceFromUnderlyingMachine) { }
};

class Category : public MachineComponent {
public:
	std::map<std::string, CategoryReference> references; // references to 'me',  i.e. by other machine components. Non-owning pointers so ok.

	Category(const std::shared_ptr<const CategoryStatement>& categoryStatement, const std::string& name)
		: MachineComponent(name), m_categoryStatement(categoryStatement) { }

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const override;
	std::shared_ptr<const ISyntacticEntity> underlyingSyntacticEntity() const override;
	bool entails(const std::string& name) const override;
	bool entails(const std::string& name, std::list<const Category*>& path) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	bool isTypeForming() const override;
	bool isTerminal() const override;

	std::list<const Production*> calculateInstandingProductions() const override;
private:
	std::shared_ptr<const CategoryStatement> m_categoryStatement;
};

class Rule : public MachineComponent {
public:
	std::shared_ptr<DisjunctiveRegex> regex;

	Rule(const std::shared_ptr<const RuleStatement>& ruleStatement, const std::string& name, const std::shared_ptr<DisjunctiveRegex>& regex)
		: m_ruleStatement(ruleStatement), MachineComponent(name), regex(regex) { }

	void initialize() override;
	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const override;
	std::shared_ptr<const ISyntacticEntity> underlyingSyntacticEntity() const override;
	
	bool entails(const std::string& name) const override;
	bool entails(const std::string& name, std::list<const Category*>& path) const override;

	void verifyContextualValidity(const Machine& machine) const override;

private:
	std::shared_ptr<const RuleStatement> m_ruleStatement;
};

class Pattern : public Rule {
public:
	Pattern(const std::shared_ptr<const RuleStatement>& ruleStatement, const std::string& name, const std::shared_ptr<DisjunctiveRegex>& regex)
		: Rule(ruleStatement, name, regex) { }

	bool isTypeForming() const override;
	bool isTerminal() const override;
	bool isRoot() const override;
	bool isIgnored() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	std::list<const Production*> calculateInstandingProductions() const override;
private:
	std::shared_ptr<const RuleStatement> m_ruleStatement;
};

class Production : public Rule {
public:
	TerminalTypeIndex typeIndex;

	Production(const std::shared_ptr<const RuleStatement>& ruleStatement, const std::string& name, std::shared_ptr<DisjunctiveRegex>& regex, TerminalTypeIndex typeIndex, bool isRoot)
		: Rule(ruleStatement, name, regex), typeIndex(typeIndex), m_isRoot(isRoot) { }

	bool isTypeForming() const override;
	bool isTerminal() const override;
	bool isRoot() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	std::list<const Production*> calculateInstandingProductions() const override;
private:
	bool m_isRoot;
	std::shared_ptr<const RuleStatement> m_ruleStatement;
};