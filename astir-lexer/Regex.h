#pragma once

#include <memory>

#include "ISyntacticEntity.h"
#include "IProductionReferencable.h"

struct RootRegex : public ISyntacticEntity, public IProductionReferencable {
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
};

struct AtomicRegex;
struct LookaheadRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> match;
	std::unique_ptr<AtomicRegex> lookahead;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
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
struct ActionTargetPair {
	RegexAction action = RegexAction::None;
	std::string target;
};
struct ActionAtomicRegex : public RootRegex {
	std::list<ActionTargetPair> actionTargetPairs;
	std::unique_ptr<AtomicRegex> regex;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
};

struct AtomicRegex : public ISyntacticEntity, public IProductionReferencable {
	virtual ~AtomicRegex() = default;
};

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex {
	std::list<std::unique_ptr<ConjunctiveRegex>> disjunction;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
};

struct RootRegex;
struct ConjunctiveRegex : public ISyntacticEntity, public IProductionReferencable {
	std::list<std::unique_ptr<RootRegex>> conjunction;

	const IFileLocalizable* findRecursiveReference(const Machine& machine, std::list<std::string>& namesEncountered, const std::string& targetName) const;
};

struct PrimitiveRegex : public AtomicRegex { };

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