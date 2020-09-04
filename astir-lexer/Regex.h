#pragma once

#include <memory>

#include "ISyntacticEntity.h"
#include "IProductionReferencable.h"
#include "INFABuildable.h"
#include "IActing.h"

using CharType = unsigned char;
using ComputationCharType = signed short int;

struct Regex : public IActing, public INFABuildable, public ISyntacticEntity, public IProductionReferencable { };

struct RootRegex : public Regex {
	virtual ~RootRegex() = default;
};

struct AtomicRegex;
struct RepetitiveRegex : public RootRegex {
	std::unique_ptr<AtomicRegex> regex;
	unsigned long minRepetitions;
	unsigned long maxRepetitions;
	static const unsigned long INFINITE_REPETITIONS = (unsigned long)((signed int)-1);

	RepetitiveRegex() : minRepetitions(0), maxRepetitions(0) { }

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const Machine& machine, const MachineComponent* context) const override;
};

struct PrimitiveRegex;
struct LookaheadRegex : public RootRegex {
	std::unique_ptr<AtomicRegex> match;
	std::unique_ptr<PrimitiveRegex> lookahead;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const Machine& machine, const MachineComponent* context) const override;
};

enum class RegexActionType : unsigned char {
	Flag = 1,
	Unflag = 2,

	Capture = 3,
	Empty = 4,
	Append = 5,
	Prepend = 6,

	Set = 7,
	Unset = 8,
	Push = 9,
	Pop = 10,
	Clear = 11,
	
	None = 255
};
struct RegexAction : public ISyntacticEntity {
	RegexActionType type = RegexActionType::None;
	std::string target;

	RegexAction() = default;
	RegexAction(RegexActionType type, const std::string target)
		: type(type), target(target) { }
};
struct AtomicRegex : public RootRegex { };

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex {
	std::list<std::unique_ptr<ConjunctiveRegex>> disjunction;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const Machine& machine, const MachineComponent* context) const override;
};

struct RootRegex;
struct ConjunctiveRegex : public Regex {
	std::list<std::unique_ptr<RootRegex>> conjunction;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const Machine& machine, const MachineComponent* context) const override;
};

struct PrimitiveRegex : public AtomicRegex {
	std::list<RegexAction> actions;

	void checkActionUsage(const Machine& machine, const MachineComponent* context) const override;
	virtual std::string computeItemType(const Machine& machine, const MachineComponent* context) const;
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

	std::string computeItemType(const Machine& machine, const MachineComponent* context) const override;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct ArbitraryLiteralRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct LineEndRegex : public PrimitiveRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};

/* struct LineBeginRegex : public PrimitiveRegex {
	
}; */