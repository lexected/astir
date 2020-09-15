#pragma once

#include "ISyntacticEntity.h"
#include "ISemanticEntity.h"
#include "IProductionReferencable.h"
#include "INFABuildable.h"
#include "ILLkNonterminal.h"
#include "IGenerationVisitable.h"

#include "Field.h"
#include "Regex.h"

#include <map>

struct MachineDefinition;
using TerminalTypeIndex = size_t;

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

struct MachineStatement : public ISyntacticEntity, public ISemanticEntity, public IReferencing, public INFABuildable, public ILLkNonterminal {
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

	void completeReferences(MachineDefinition& machine) const;

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