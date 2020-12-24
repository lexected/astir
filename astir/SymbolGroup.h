#pragma once

#include <string>
#include <list>

#include "AFACondition.h"
#include "CharType.h"

using SymbolIndex = size_t;

struct SymbolGroup : public virtual AFACondition {
public:
	virtual ~SymbolGroup() = default;

	bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const override;
	virtual bool equals(const SymbolGroup* rhs) const = 0;
	virtual bool disjoint(const SymbolGroup* rhs) const = 0;
	virtual std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) = 0;
	virtual std::string toString() const = 0;

	virtual std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const = 0;
protected:
	SymbolGroup() = default;
};

class SymbolGroupList : public std::list<std::shared_ptr<SymbolGroup>> {
public:
	SymbolGroupList() = default;
	SymbolGroupList(std::initializer_list<std::shared_ptr<SymbolGroup>> il)
		: std::list<std::shared_ptr<SymbolGroup>>(il) { }
	SymbolGroupList(const SymbolGroupList::const_iterator& begin, const SymbolGroupList::const_iterator& end)
		: std::list<std::shared_ptr<SymbolGroup>>(begin, end) { }

	bool contains(const std::shared_ptr<SymbolGroup>& symbolGroupPtr) const;
	bool containsEmpty() const;
	SymbolGroupList allButEmpty() const;
	void removeEmpty();

	std::string asSequenceString() const;

	SymbolGroupList& operator+=(const SymbolGroupList& rhs);
};

struct EmptySymbolGroup : public SymbolGroup, public EmptyAFACondition {
public:
	EmptySymbolGroup() = default;
	
	bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const override;
	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;
	std::string toString() const override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
protected:
};

struct ByteSymbolGroup : public SymbolGroup {
	ByteSymbolGroup()
		: ByteSymbolGroup(0, 0) { }
	ByteSymbolGroup(CharType rangeStart, CharType rangeEnd)
		: SymbolGroup(), rangeStart(rangeStart), rangeEnd(rangeEnd), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }
	ByteSymbolGroup(const ByteSymbolGroup& lsg)
		: ByteSymbolGroup(lsg.rangeStart, lsg.rangeEnd) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;
	std::string toString() const override;

	CharType rangeStart;
	CharType rangeEnd;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup() = default;
	LiteralSymbolGroup(const std::string& literal)
		: SymbolGroup(), literal(literal) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;
	std::string toString() const override;

	std::string literal;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
};

struct TypeFormingStatement;
struct MachineDefinition;
struct StatementSymbolGroup : public SymbolGroup {
	StatementSymbolGroup() = default;
	StatementSymbolGroup(const TypeFormingStatement* statement, const MachineDefinition* statementMachine)
		: SymbolGroup(), statement(statement), statementMachine(statementMachine), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;
	std::string toString() const override;

	const TypeFormingStatement* statement;
	const MachineDefinition* statementMachine;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;

private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};
