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

struct SpecificationFileStatement;
struct SpecificationFile {
	StandardList<SpecificationFileStatement> statements;
};

struct SpecificationFileStatement {
	virtual ~SpecificationFileStatement() = default;
};

struct UsingStatement : public SpecificationFileStatement {
	std::string filePath;
};

struct MachineStatement;
struct MachineDefinition : public SpecificationFileStatement {
	std::string machineName;
	StandardList<MachineStatement> statements;
	std::string extends;
	std::string follows;

	MachineDefinition() = default;
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

struct MemberDeclaration;
struct MachineStatement {
	std::string name;
	std::list<std::string> categories;
	StandardList<MemberDeclaration> members;

	virtual ~MachineStatement() = default;
};

struct CategoryStatement : public MachineStatement {

};

struct Alternative;
struct GrammarStatement : public MachineStatement {
	GrammarStatementType type;
	StandardList<Alternative> alternatives;
};

struct MemberDeclaration {
	std::string name;

	virtual ~MemberDeclaration() = default;
};

struct FlagDeclaration : public MemberDeclaration {
	
};

struct RawDeclaration : public MemberDeclaration {

};

struct VariablyTypedDeclaration : public MemberDeclaration {
	std::string type;
};

struct ItemDeclaration : public VariablyTypedDeclaration {
	
};

struct ListDeclaration : public VariablyTypedDeclaration {

};

struct RootRegex;
struct Alternative {
	StandardList<RootRegex> rootRegexes;
};

struct RootRegex {
	virtual ~RootRegex() = default;
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
	virtual ~AtomicRegex() = default;
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
	std::string referenceName;
};

struct ArbitraryLiteralRegex : public AtomicRegex {
	
};

struct LineBeginRegex : public AtomicRegex {

};

struct LineEndRegex : public AtomicRegex {

};