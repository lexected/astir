#pragma once

#include <memory>

#include "ISyntacticEntity.h"
#include "IProductionReferencable.h"
#include "INFABuildable.h"
#include "IActing.h"

struct Regex : public IActing, public INFABuildable, public ISyntacticEntity, public IProductionReferencable { };

struct RootRegex : public Regex {
	virtual ~RootRegex() = default;
};

struct ActionAtomicRegex;
struct RepetitiveRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> actionAtomicRegex;
	unsigned long minRepetitions;
	unsigned long maxRepetitions;
	static const unsigned long INFINITE_REPETITIONS = (unsigned long)((signed int)-1);

	RepetitiveRegex() : minRepetitions(0), maxRepetitions(0) { }

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const MachineComponent* context) const override;
};

struct AtomicRegex;
struct LookaheadRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> match;
	std::unique_ptr<AtomicRegex> lookahead;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const MachineComponent* context) const override;
};

enum class RegexAction {
	Set,
	Unset,
	Flag,
	Unflag,
	Append,
	Prepend,
	Clear,
	LeftTrim,
	RightTrim,

	None
};
struct ActionTargetPair : public ISyntacticEntity {
	RegexAction action = RegexAction::None;
	std::string target;

	ActionTargetPair() = default;
	ActionTargetPair(RegexAction action, const std::string target)
		: action(action), target(target) { }
};
struct ActionAtomicRegex : public RootRegex {
	std::list<ActionTargetPair> actionTargetPairs;
	std::unique_ptr<AtomicRegex> regex;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const MachineComponent* context) const override;
};

struct AtomicRegex : public Regex { };

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex {
	std::list<std::unique_ptr<ConjunctiveRegex>> disjunction;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const MachineComponent* context) const override;
};

struct RootRegex;
struct ConjunctiveRegex : public Regex {
	std::list<std::unique_ptr<RootRegex>> conjunction;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;

	NFA accept(const NFABuilder& nfaBuilder) const override;

	void checkActionUsage(const MachineComponent* context) const override;
};

struct PrimitiveRegex : public AtomicRegex {
	NFA accept(const NFABuilder& nfaBuilder) const override;
};

struct RegexRange {
	char start;
	char end;
};

struct AnyRegex : PrimitiveRegex {
	std::list<std::string> literals;
	std::list<RegexRange> ranges;
};

struct ExceptAnyRegex : public AnyRegex { };

struct LiteralRegex : public PrimitiveRegex {
	std::string literal;
};

struct ReferenceRegex : public PrimitiveRegex {
	std::string referenceName;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
};

struct ArbitraryLiteralRegex : public PrimitiveRegex { };

struct LineBeginRegex : public PrimitiveRegex { };

struct LineEndRegex : public PrimitiveRegex { };