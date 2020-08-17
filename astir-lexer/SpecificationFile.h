#pragma once

#include <list>
#include <memory>
#include <map>
#include <string>

#include "FileLocation.h"
#include "Specification.h"

template <class ProductionType>
using UniqueList = std::list<std::unique_ptr<ProductionType>>;

/*
	As a general rule, avoid creating full insertive constructors for objects, since the container ownership of unique_ptrs then often gets quite tricky.
	It's usually much better to create a 'minimal' initialization in in the default constructor and have everything else done from outside by the relevant parsing procedure.
*/

struct ParsedStructure : public IFileLocalizable { };

struct SpecificationFileStatement;
struct SpecificationFile : public ISpecificationInitializable<Specification> {
	UniqueList<SpecificationFileStatement> statements;

	std::shared_ptr<Specification> makeSpecificationEntity() const override;
private:
	void initializeMachineWithDependencies(Machine* machine, const std::map<std::string, MachineDefinition*>& definitions, std::map<std::string, bool>& initializationMap) const;
};

struct SpecificationFileStatement : public ParsedStructure {
	virtual ~SpecificationFileStatement() = default;
};

struct UsingStatement : public SpecificationFileStatement {
	std::string filePath;
};

struct MachineStatement;
struct MachineDefinition : public SpecificationFileStatement, public ISpecificationInitializable<Machine> {
public:
	std::string name;
	UniqueList<MachineStatement> statements;
	std::string extends;
	std::string follows;
	
	void initializeSpecificationEntity(Machine* machine) const override;
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

	std::shared_ptr<Machine> makeSpecificationEntity() const override;
};

enum class GrammarStatementType {
	Regex,
	Token,
	Rule,
	Production
};

struct FieldDeclaration;
struct MachineStatement : public ParsedStructure, public ISpecificationInitializable<MachineComponent> {
	std::string name;
	std::list<std::string> categories;
	UniqueList<FieldDeclaration> fields;
	
	virtual ~MachineStatement() = default;

	void initializeSpecificationEntity(MachineComponent* machineComponent) const override;
	virtual void initializeSpecificationEntity(const Machine* context, MachineComponent* machineComponent) const;
};

struct CategoryStatement : public MachineStatement {
	std::shared_ptr<MachineComponent> makeSpecificationEntity() const override;
};

struct DisjunctiveRegex;
struct GrammarStatement : public MachineStatement {
	GrammarStatementType type;
	std::shared_ptr<DisjunctiveRegex> disjunction;

	std::shared_ptr<MachineComponent> makeSpecificationEntity() const override;
};

struct FieldDeclaration : public ParsedStructure, public ISpecificationInitializable<Field> {
	std::string name;
};

struct FlagFieldDeclaration : public FieldDeclaration {
	std::shared_ptr<Field> makeSpecificationEntity() const override;
};

struct RawFieldDeclaration : public FieldDeclaration {
	std::shared_ptr<Field> makeSpecificationEntity() const override;
};

struct VariablyTypedFieldDeclaration : public FieldDeclaration {
	std::string type;
};

struct ItemFieldDeclaration : public VariablyTypedFieldDeclaration {
	std::shared_ptr<Field> makeSpecificationEntity() const override;
};

struct ListFieldDeclaration : public VariablyTypedFieldDeclaration {
	std::shared_ptr<Field> makeSpecificationEntity() const override;
};

struct RootRegex : public ParsedStructure, public IProductionReferencable {
	virtual ~RootRegex() = default;
};

struct ActionAtomicRegex;
struct RepetitiveRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> actionAtomicRegex;
	unsigned long minRepetitions;
	unsigned long maxRepetitions;

	const unsigned long INFINITE_REPETITIONS = (unsigned long)((signed int)-1);

	RepetitiveRegex() : minRepetitions(0), maxRepetitions(0) { }

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
};

struct AtomicRegex;
struct LookaheadRegex : public RootRegex {
	std::unique_ptr<ActionAtomicRegex> match;
	std::unique_ptr<AtomicRegex> lookahead;

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
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

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
};

struct AtomicRegex : public ParsedStructure, public IProductionReferencable {
	virtual ~AtomicRegex() = default;
};

struct ConjunctiveRegex;
struct DisjunctiveRegex : public AtomicRegex {
	UniqueList<ConjunctiveRegex> disjunction;

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
};

struct RootRegex;
struct ConjunctiveRegex : public ParsedStructure, public IProductionReferencable {
	UniqueList<RootRegex> conjunction;

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
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

	bool componentRecursivelyReferenced(const Machine& machine, std::list<std::string>& namesEncountered) const override;
};

struct ArbitraryLiteralRegex : public PrimitiveRegex { };

struct LineBeginRegex : public PrimitiveRegex { };

struct LineEndRegex : public PrimitiveRegex { };