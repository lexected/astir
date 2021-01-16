#pragma once

#include <memory>

#include "RegexAction.h"

#include "ISyntacticEntity.h"
#include "IReferencing.h"
#include "INFABuildable.h"
#include "IActing.h"
#include "ILLkFirstable.h"
#include "ILLkBuilding.h"
#include "ILLkParserGenerable.h"
#include "CharType.h"
#include "ILRABuilding.h"

struct Regex
	: public IActing, public INFABuildable, public ISyntacticEntity,
		public IReferencing, public ILLkBuilding, public ILLkParserGenerable,
		public ILRABuilding { };

struct RootRegex : public Regex {
public:
	std::list<RegexAction> actions;

	virtual ~RootRegex() = default;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
	virtual std::string computeItemType(const MachineDefinition& machine, const MachineStatement* context) const;
	void accept(LLkBuilder* llkBuilder) const override;
};

struct AtomicRegex;
struct RepetitiveRegex : public RootRegex, public ILLkNonterminal {
	std::shared_ptr<AtomicRegex> regex;
	unsigned long minRepetitions;
	unsigned long maxRepetitions;
	static const unsigned long INFINITE_REPETITIONS;

	RepetitiveRegex() : RepetitiveRegex(nullptr, 0, 0) { }
	RepetitiveRegex(const std::shared_ptr<AtomicRegex>& regex, unsigned long minRepetitions,
	unsigned long maxRepetitions) : regex(regex), minRepetitions(minRepetitions), maxRepetitions(maxRepetitions), m_tailFlyweight(std::make_unique<std::shared_ptr<RepetitiveRegex>>()) { }

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkBuilder* llkBuilder) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;

	const std::shared_ptr<RepetitiveRegex>& kleeneTail() const;
private:
	std::unique_ptr<std::shared_ptr<RepetitiveRegex>> m_tailFlyweight;
};

struct AtomicRegex : public RootRegex { };

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex, public ILLkNonterminal {
	std::list<std::unique_ptr<ConjunctiveRegex>> disjunction;

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkBuilder* llkBuilder) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct ConjunctiveRegex : public Regex, public ILLkNonterminal {
	std::list<std::unique_ptr<RootRegex>> conjunction;

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkBuilder* llkBuilder) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct PrimitiveRegex : public AtomicRegex, public ILLkFirstable {
	
};

struct EmptyRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct RegexRange {
	CharType start;
	CharType end;
};

struct AnyRegex : public PrimitiveRegex {
	std::list<std::string> literals;
	std::list<RegexRange> ranges;

	SymbolGroupList makeSymbolGroups() const;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct ExceptAnyRegex : public AnyRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct LiteralRegex : public PrimitiveRegex {
	std::string literal;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct MachineStatement;
struct ReferenceRegex : public PrimitiveRegex {
	std::string referenceName;
	const MachineDefinition* referenceStatementMachine;
	const MachineStatement* referenceStatement;

	ReferenceRegex()
		: referenceStatementMachine(nullptr), referenceStatement(nullptr) { }

	std::string computeItemType(const MachineDefinition& machine, const MachineStatement* context) const override;

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkBuilder* llkBuilder) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};

struct ArbitrarySymbolRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
	SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const override;
	void accept(LLkParserGenerator* generator) const override;
	void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const override;
};
