#pragma once

#include <list>
#include <memory>
#include <map>

template <class ProductionType>
using StandardList = std::list<std::unique_ptr<ProductionType>>;

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure.
*/

struct SpecificationStatement;
struct Specification {
	StandardList<SpecificationStatement> statements;
};

struct SpecificationStatement {

};

struct UsingStatement : public SpecificationStatement {
	std::string filePath;
};

struct MachineStatement;
struct MachineDefinition : public SpecificationStatement {
	std::string machineName;
	StandardList<MachineStatement> statements;
	std::string extends;
	std::string follows;

	MachineDefinition() = default;

	virtual ~MachineDefinition() = default;
};

enum class FAType {
	Deterministic,
	Nondeterministic
};

enum class FAFlag {
	GroupedStringLiterals,
	TableLookup
};

struct FADefinition : public MachineDefinition {
	FAType type;
	std::map<FAFlag, bool> attributes;

	FADefinition()
		:  MachineDefinition(), type(FAType::Nondeterministic), attributes({
			{ FAFlag::GroupedStringLiterals, false },
			{ FAFlag::TableLookup, false }
		}) { }

	virtual ~FADefinition() = default;
};

enum class GrammarStatementType {
	Regex,
	Token,
	Rule,
	Production
};

struct MachineStatement {
	std::string name;
	std::list<std::string> categories;
};

struct Alternative;
struct GrammarStatement : public MachineStatement {
	GrammarStatementType type;
	StandardList<Alternative> alternatives;
};

struct QualifiedName;
struct SpecifiedName;
struct CategoryStatement : public MachineStatement {
	StandardList<QualifiedName> qualifiedNames;
};

struct SpecifiedName {
	std::string name;
};

struct QualifiedName : public SpecifiedName {
	std::string instanceName;
};

struct RootRegex;
struct Alternative {
	StandardList<RootRegex> rootRegexes;
};

struct RootRegex {
	
};

struct ActionAtomicRegex;
struct RepetitiveRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> actionAtomicRegex;
	unsigned long minRepetitions;
	unsigned long maxRepetitions;

	const unsigned long INFINITE_REPETITIONS = (unsigned long)((signed int)-1);

	RepetitiveRegex() : minRepetitions(0), maxRepetitions(0) { }
};

struct AtomicRegex;
struct LookaheadRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> match;
	std::unique_ptr<AtomicRegex> lookahead;
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
};

struct AtomicRegex {

};

struct RegexRange {
	char start;
	char end;
};

struct AnyRegex : public AtomicRegex {
	std::list<std::string> literals;
	std::list<RegexRange> ranges;
};

struct ExceptAnyRegex : public AnyRegex {
	
};

struct ConjunctiveRegex {
	StandardList<RootRegex> conjunction;
};

struct DisjunctiveRegex : public AtomicRegex {
	StandardList<ConjunctiveRegex> disjunction;
};

struct LiteralRegex : public AtomicRegex {
	std::string literal;
};

struct ReferenceRegex : public AtomicRegex {
	SpecifiedName referenceName;
};

struct ArbitraryLiteralRegex : public AtomicRegex {
	
};

struct LineBeginRegex : public AtomicRegex {

};

struct LineEndRegex : public AtomicRegex {

};