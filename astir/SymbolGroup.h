#pragma once

#include <list>
#include <memory>

using SymbolIndex = size_t;

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	virtual bool equals(const SymbolGroup* rhs) const = 0;
	virtual bool disjoint(const SymbolGroup* rhs) const = 0;
	virtual std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) = 0;

	virtual std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const = 0;

protected:
	SymbolGroup() = default;
};

class SymbolGroupList : public std::list<std::shared_ptr<SymbolGroup>> {
public:
	bool containsEmpty() const;
	SymbolGroupList allButEmpty() const;
	void removeEmpty();
};

struct EmptySymbolGroup : public SymbolGroup {
public:
	EmptySymbolGroup()
		: SymbolGroup() { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
protected:
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: LiteralSymbolGroup(0, 0) { }
	LiteralSymbolGroup(CharType rangeStart, CharType rangeEnd)
		: SymbolGroup(), rangeStart(rangeStart), rangeEnd(rangeEnd), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }
	LiteralSymbolGroup(const LiteralSymbolGroup& lsg)
		: LiteralSymbolGroup(lsg.rangeStart, lsg.rangeEnd) { }

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
struct TerminalSymbolGroup : public SymbolGroup {
	std::list<const ProductionStatement*> referencedProductions;
	TerminalSymbolGroup(const std::list<const ProductionStatement*>& referencedProductions)
		: SymbolGroup(), referencedProductions(referencedProductions), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }

	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};