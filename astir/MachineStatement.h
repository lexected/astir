#pragma once

#include "ISyntacticEntity.h"
#include "ISemanticEntity.h"
#include "IReferencing.h"
#include "INFABuildable.h"
#include "ILLkFirstable.h"
#include "ILLkBuilding.h"
#include "IGenerationVisitable.h"
#include "ILLkParserGenerable.h"
#include "ILRABuilding.h"

#include "Field.h"
#include "Regex.h"

#include <map>
#include <set>

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

/* The following locally supresses the "'X' inherits 'Y' via dominance" warnings */
#pragma warning( push )
#pragma warning( disable : 4250 )


struct MachineStatement
	: public ISyntacticEntity, public ISemanticEntity, public IReferencing,
		public INFABuildable, public ILLkNonterminal, public ILLkBuilding,
		public ILLkParserGenerable, public ILRABuilding {
	std::string name;
	virtual ~MachineStatement() = default;

	std::string referenceName() const override;

	virtual bool isTypeForming() const;

protected:
	MachineStatement() = default;
	MachineStatement(const std::string& name)
		: name(name) { }
};

struct CategoryStatement;
struct ProductionStatement;
struct AttributedStatement : public virtual MachineStatement {
	std::map<std::string, std::shared_ptr<CategoryStatement>> categories;
	std::list<std::shared_ptr<Field>> fields;

	std::shared_ptr<Field> findField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const;
	void completeFieldDeclarations(MachineDefinition& context) const;

	virtual std::list<const ProductionStatement*> calculateInstandingProductions() const = 0;

	virtual bool categoricallyRefersTo(const AttributedStatement* statement) const = 0;
	virtual std::set<const AttributedStatement*> unpickReferal(const AttributedStatement* statement) const = 0;

private:
	std::shared_ptr<Field> findCategoryField(const std::string& name, std::shared_ptr<CategoryStatement>& categoryFoundIn) const;
};

struct TypeFormingStatement : public AttributedStatement, public IGenerationVisitable {
	Rootness rootness;

	bool isTypeForming() const override;

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
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	virtual void verifyContextualValidity(const MachineDefinition& machine) const = 0;

	void accept(LLkBuilder* llkBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
};

struct CategoryReference {
	const AttributedStatement* statement;
	bool isAReferenceFromUnderlyingMachine; // well, if you put it this way, it really does sounds stupid!

	CategoryReference()
		: statement(nullptr), isAReferenceFromUnderlyingMachine(false) { }
	CategoryReference(const AttributedStatement* statement, bool isAReferenceFromUnderlyingMachine)
		: statement(statement), isAReferenceFromUnderlyingMachine(isAReferenceFromUnderlyingMachine) { }
};

struct CategoryStatement : public TypeFormingStatement {
	std::map<std::string, CategoryReference> references; // references to 'me',  i.e. by other machine components. Non-owning pointers so ok.

	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	bool categoricallyRefersTo(const AttributedStatement* statement) const override;
	std::set<const AttributedStatement*> unpickReferal(const AttributedStatement* statement) const override;
	std::list<const ProductionStatement*> calculateInstandingProductions() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkBuilder* llkBuilder) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct ProductionStatement : public TypeFormingStatement, public RuleStatement {
	Terminality terminality;
	TerminalTypeIndex terminalTypeIndex;

	ProductionStatement()
		: terminality(Terminality::Unspecified), terminalTypeIndex(0) { }

	void verifyContextualValidity(const MachineDefinition& machine) const override;

	bool categoricallyRefersTo(const AttributedStatement* statement) const override;
	std::set<const AttributedStatement*> unpickReferal(const AttributedStatement* statement) const override;
	std::list<const ProductionStatement*> calculateInstandingProductions() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	using RuleStatement::first;
	using RuleStatement::findRecursiveReference;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct PatternStatement : public AttributedStatement, public RuleStatement {
	void verifyContextualValidity(const MachineDefinition& machine) const override;

	bool categoricallyRefersTo(const AttributedStatement* statement) const override;
	std::set<const AttributedStatement*> unpickReferal(const AttributedStatement* statement) const override;
	std::list<const ProductionStatement*> calculateInstandingProductions() const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	using RuleStatement::first;
	using RuleStatement::findRecursiveReference;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct RegexStatement : public RuleStatement {
	void verifyContextualValidity(const MachineDefinition& machine) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	using RuleStatement::first;
	using RuleStatement::findRecursiveReference;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

#pragma warning( pop )