#pragma once

#include <string>
#include <list>
#include <memory>

#include "CharType.h"

using SymbolIndex = size_t;

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	virtual bool equals(const SymbolGroup* rhs) const = 0;
	virtual bool disjoint(const SymbolGroup* rhs) const = 0;
	virtual std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) = 0;

protected:
	SymbolGroup() = default;
};

struct SimpleSymbolGroup : public SymbolGroup {
	virtual std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const = 0;
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

	SymbolGroupList& operator+=(const SymbolGroupList& rhs);
};

struct EmptySymbolGroup : public SimpleSymbolGroup {
public:
	EmptySymbolGroup() = default;

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
protected:
};

struct ByteSymbolGroup : public SimpleSymbolGroup {
	ByteSymbolGroup()
		: ByteSymbolGroup(0, 0) { }
	ByteSymbolGroup(CharType rangeStart, CharType rangeEnd)
		: SimpleSymbolGroup(), rangeStart(rangeStart), rangeEnd(rangeEnd), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }
	ByteSymbolGroup(const ByteSymbolGroup& lsg)
		: ByteSymbolGroup(lsg.rangeStart, lsg.rangeEnd) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	CharType rangeStart;
	CharType rangeEnd;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};

struct ProductionStatement;
struct TerminalSymbolGroup : public SimpleSymbolGroup {
	std::list<const ProductionStatement*> referencedProductions;
	TerminalSymbolGroup(const std::list<const ProductionStatement*>& referencedProductions)
		: SimpleSymbolGroup(), referencedProductions(referencedProductions), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

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

	std::string literal;
};

struct StatementSymbolGroup : public SymbolGroup {
	StatementSymbolGroup() = default;
	StatementSymbolGroup(const TypeFormingStatement* statement, const MachineDefinition* statementMachine)
		: SymbolGroup(), statement(statement), statementMachine(statementMachine) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	const TypeFormingStatement* statement;
	const MachineDefinition* statementMachine;
};
