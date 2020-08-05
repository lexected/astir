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

struct MachineDefinition;
struct Specification {
	StandardList<MachineDefinition> machineDefinitions;
};

struct Statement;
struct MachineDefinition {
	std::string machineName;
	StandardList<Statement> statements;
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

struct Statement {
	std::string name;
	std::list<std::string> categories;
};

struct Alternative;
struct GrammarStatement : public Statement {
	GrammarStatementType type;
	StandardList<Alternative> alternatives;
};

struct QualifiedName;
struct SpecifiedName;
struct CategoryStatement : public Statement {
	StandardList<QualifiedName> qualifiedNames;
};

struct SpecifiedName {
	std::list<std::string> queriedCategories;
};

struct QualifiedName : public SpecifiedName {
	std::string instanceName;
};

struct QualifiedRegex;
struct Alternative {
	StandardList<QualifiedRegex> qualifiedRegexes;
};

struct NamableRegex;
struct QualifiedRegex {
	bool hasInstanceName;
	std::string instanceName;
	std::unique_ptr<NamableRegex> regex;
};

/* namable regexes differ from the regexes with alternatives in that they can not on their own have alternatives on their top level */
struct NamableRegex {
	
};

struct RepetitiveRegex : public NamableRegex {
	unsigned int minRepetitions;
	unsigned int maxRepetitions;
};

struct AtomicRegex;
struct LookaheadRegex : public NamableRegex {
	std::unique_ptr<AtomicRegex> match;
	std::unique_ptr<NamableRegex> lookahead;
};

struct AtomicRegex : public NamableRegex { };

struct RegexRange {
	std::string start;
	std::string end;
};

struct AnyLiteralRegex : public AtomicRegex {
	std::list<std::string> literals;
};

struct AnyRangeRegex : public AtomicRegex {
	std::list<RegexRange> ranges;
};

struct ExceptAnyLiteralRegex : public AtomicRegex {
	std::list<std::string> literals;
};

struct ExceptAnyRangeRegex : public AtomicRegex {
	std::list<RegexRange> ranges;
};

struct ConjunctiveRegex {
	StandardList<NamableRegex> conjunction;
};

struct DisjunctiveRegex : public AtomicRegex {
	StandardList<ConjunctiveRegex> disjunction;
};

struct LiteralRegex : public AtomicRegex {
	
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