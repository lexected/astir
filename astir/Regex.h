#pragma once

#include <memory>

#include "RegexAction.h"

#include "ISyntacticEntity.h"
#include "IReferencing.h"
#include "INFABuildable.h"
#include "IActing.h"
#include "ILLkNonterminal.h"
#include "ILLkBuildable.h"
#include "CharType.h"

struct Regex : public IActing, public INFABuildable, public ISyntacticEntity, public IReferencing, public ILLkBuildable, public ILLkNonterminal { };

struct RootRegex : public Regex {
public:
	std::list<RegexAction> actions;

	virtual ~RootRegex() = default;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
	virtual std::string computeItemType(const MachineDefinition& machine, const MachineStatement* context) const;

protected:
};

struct AtomicRegex;
struct RepetitiveRegex : public RootRegex {
	std::unique_ptr<AtomicRegex> regex;
	unsigned long minRepetitions;
	unsigned long maxRepetitions;
	static const unsigned long INFINITE_REPETITIONS = (unsigned long)((signed int)-1);

	RepetitiveRegex() : minRepetitions(0), maxRepetitions(0) { }

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct AtomicRegex : public RootRegex { };

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex {
	std::list<std::unique_ptr<ConjunctiveRegex>> disjunction;

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct ConjunctiveRegex : public Regex {
	std::list<std::unique_ptr<RootRegex>> conjunction;

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct PrimitiveRegex : public AtomicRegex {
	
};

struct EmptyRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct RegexRange {
	CharType start;
	CharType end;
};

struct AnyRegex : public PrimitiveRegex {
	std::list<std::string> literals;
	std::list<RegexRange> ranges;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct ExceptAnyRegex : public AnyRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct LiteralRegex : public PrimitiveRegex {
	std::string literal;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

class MachineStatement;
struct ReferenceRegex : public PrimitiveRegex {
	std::string referenceName;
	const MachineStatement* referenceStatement;

	std::string computeItemType(const MachineDefinition& machine, const MachineStatement* context) const override;

	void completeReferences(const MachineDefinition& machine) override;
	IFileLocalizableCPtr findRecursiveReference(std::list<IReferencingCPtr>& referencingEntitiesEncountered) const override;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct ArbitrarySymbolRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};
