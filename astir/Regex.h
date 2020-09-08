#pragma once

#include <memory>

#include "RegexAction.h"

#include "ISyntacticEntity.h"
#include "IProductionReferencable.h"
#include "INFABuildable.h"
#include "IActing.h"

using CharType = unsigned char;
using ComputationCharType = signed short int;

struct Regex : public IActing, public INFABuildable, public ISyntacticEntity, public IProductionReferencable { };

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

	const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct AtomicRegex : public RootRegex { };

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex {
	std::list<std::unique_ptr<ConjunctiveRegex>> disjunction;

	const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkAndTypeformActionUsage(const MachineDefinition& machine, const MachineStatement* context, bool areActionsAllowed) override;
};

struct RootRegex;
struct ConjunctiveRegex : public Regex {
	std::list<std::unique_ptr<RootRegex>> conjunction;

	const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

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

struct ReferenceRegex : public PrimitiveRegex {
	std::string referenceName;

	std::string computeItemType(const MachineDefinition& machine, const MachineStatement* context) const override;

	const IFileLocalizable* findRecursiveReference(const MachineDefinition& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct ArbitrarySymbolRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};
