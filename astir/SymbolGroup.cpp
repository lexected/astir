#include "SymbolGroup.h"
#include "CharType.h"

#include <algorithm>
#include "SyntacticTree.h"
#include "MachineStatement.h"

#include <sstream>
#include <stdexcept>

bool SymbolGroup::equals(const std::shared_ptr<AFACondition>& anotherCondition) const {
	const SymbolGroup* convertedRhs = dynamic_cast<const SymbolGroup*>(anotherCondition.get());
	if (convertedRhs != nullptr) {
		return this->equals(convertedRhs);
	} else {
		return this->AFACondition::equals(anotherCondition);
	}
}

bool SymbolGroup::equals(const SymbolGroup* rhs) const {
	return isEmpty() && rhs->isEmpty();
}

bool SymbolGroup::disjoint(const SymbolGroup* rhs) const {
	return !rhs->isEmpty();
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> SymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) {
	if (rhs->equals(this)) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	}

	if (!rhs->isEmpty()) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >>({ { rhs, true } });
	} else {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	}
}

std::string SymbolGroup::toString() const {
	return std::string("empty");
}

std::shared_ptr<std::list<SymbolIndex>> SymbolGroup::retrieveSymbolIndices() const {
	return std::shared_ptr<std::list<SymbolIndex>>();
}

bool ByteSymbolGroup::isEmpty() const {
	return false;
}

bool ByteSymbolGroup::equals(const SymbolGroup* rhs) const {
	const ByteSymbolGroup* rhsCast = dynamic_cast<const ByteSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		const LiteralSymbolGroup* rhsCastAgain = dynamic_cast<const LiteralSymbolGroup*>(rhs);
		if (rhsCastAgain == nullptr) {
			return false;
		} else {
			return rhsCastAgain->literal.length() == 1
				&& rhsCastAgain->literal[0] == this->rangeStart
				&& rhsCastAgain->literal[0] == this->rangeEnd;
		}
	} else {
		return this->rangeStart == rhsCast->rangeStart && this->rangeEnd == rhsCast->rangeEnd;
	}
}

bool ByteSymbolGroup::disjoint(const SymbolGroup* rhs) const {
	const ByteSymbolGroup* rhsCast = dynamic_cast<const ByteSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		const LiteralSymbolGroup* rhsCastAgain = dynamic_cast<const LiteralSymbolGroup*>(rhs);
		if (rhsCastAgain == nullptr) {
			return true;
		} else {
			return rhsCastAgain->literal.length() != 1
				|| rhsCastAgain->literal[0] < this->rangeStart
				|| rhsCastAgain->literal[0] > this->rangeEnd;
		}
	} else {
		return this->rangeStart > rhsCast->rangeEnd || rhsCast->rangeStart > this->rangeEnd;
	}
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >> ByteSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhsUncast) {
	if (rhsUncast->equals(this)) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	} else if (this->disjoint(rhsUncast.get())) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >>({ { rhsUncast, true } });
	}

	std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >> ret;
	const ByteSymbolGroup* rhs;
	const LiteralSymbolGroup* rhsCastAgain = dynamic_cast<const LiteralSymbolGroup*>(rhsUncast.get());
	if (rhsCastAgain != nullptr) {
		// to avoid duplicating code just pretend that you are a ByteSymbolGroup and then don't forget to deallocate before return
		rhs = new ByteSymbolGroup((CharType)rhsCastAgain->literal[0], (CharType)rhsCastAgain->literal[0]);
	} else {
		rhs = dynamic_cast<ByteSymbolGroup*>(rhsUncast.get());
	}

	if (this->rangeEnd >= rhs->rangeStart) {
		ComputationCharType mid_beg = std::max(this->rangeStart, rhs->rangeStart);
		ComputationCharType mid_end = std::min(this->rangeEnd, rhs->rangeEnd);
		// thanks to the first condition we already have mid_beg <= mid_end

		ComputationCharType bottom_beg = std::min(this->rangeStart, rhs->rangeStart);
		ComputationCharType bottom_end = (short)mid_beg - 1;

		ComputationCharType top_beg = std::min(this->rangeEnd, rhs->rangeEnd) + 1;
		ComputationCharType top_end = std::max(this->rangeEnd, rhs->rangeEnd);

		if (bottom_beg <= bottom_end) {
			ret.emplace_back(std::make_shared<ByteSymbolGroup>((CharType)bottom_beg, (CharType)bottom_end), bottom_beg != this->rangeStart);
		}

		this->rangeStart = (CharType)mid_beg;
		this->rangeEnd = (CharType)mid_end;
		ret.emplace_back(std::make_shared<ByteSymbolGroup>((CharType)mid_beg, (CharType)mid_end), true);

		if (top_beg <= top_end) {
			ret.emplace_back(std::make_shared<ByteSymbolGroup>((CharType)top_beg, (CharType)top_end), top_beg == this->rangeEnd);
		}
	}

	if (rhsCastAgain != nullptr) {
		delete rhs;
	}
	return ret;
}

std::string ByteSymbolGroup::toString() const {
	return std::string("'") + std::string(rangeStart, 1) + "' - " + std::string(rangeEnd, 1) + "'";
}

std::shared_ptr<std::list<SymbolIndex>> ByteSymbolGroup::retrieveSymbolIndices() const {
	if (m_symbolIndicesFlyweight->empty()) {
		for (ComputationCharType it = rangeStart; it <= (ComputationCharType)rangeEnd; ++it) {
			m_symbolIndicesFlyweight->push_back(it);
		}
	}

	return m_symbolIndicesFlyweight;
}

bool SymbolGroupList::contains(const std::shared_ptr<SymbolGroup>& symbolGroupPtr) const {
	for (const auto& elementPtr : *this) {
		if (elementPtr == symbolGroupPtr) {
			return true;
		}

		if (!elementPtr->disjoint(symbolGroupPtr.get())) {
			return true;
		}
	}

	return false;
}

bool SymbolGroupList::containsEmpty() const {
	for (const auto& sgPtr : *this) {
		if (sgPtr->isEmpty()) {
			return true;
		}
	}

	return false;
}

SymbolGroupList SymbolGroupList::allButEmpty() const {
	SymbolGroupList ret;
	for (const auto& sgPtr : *this) {
		if (!sgPtr->isEmpty()) {
			ret.push_back(sgPtr);
		}
	}

	return ret;
}

void SymbolGroupList::removeEmpty() {
	/*auto newEndIt = 
	I ONLY INTRODUCED THIS BECAUSE OF A NO-DISCARD. TURNS OUT IT IS NOT C++17 compliant, only C++20*/
	
	this->remove_if([](const auto& ptr) {
		return ptr->isEmpty();
	});
}

std::string SymbolGroupList::asSequenceString() const {
	if (empty()) {
		return "";
	}

	std::stringstream ss;
	ss << front()->toString();
	for (auto it = ++cbegin(); it != cend(); ++it) {
		ss << ' ';
		ss << (*it)->toString();
	}
	return ss.str();
}

SymbolGroupList& SymbolGroupList::operator+=(const SymbolGroupList& rhs) {
	for (const auto& rhsItem : rhs) {
		auto fit = std::find_if(cbegin(), cend(), [&rhsItem](const auto& sgPtr) {
			return sgPtr->equals(rhsItem.get());
		});
		if (fit == cend()) {
			push_back(rhsItem);
		}
	}

	return *this;
}

bool LiteralSymbolGroup::isEmpty() const {
	return false;
}

bool LiteralSymbolGroup::equals(const SymbolGroup* rhs) const {
	const LiteralSymbolGroup* rhsCast = dynamic_cast<const LiteralSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		const ByteSymbolGroup* rhsCastAsBSG = dynamic_cast<const ByteSymbolGroup*>(rhs);
		if(rhsCastAsBSG == nullptr) {
			return false;
		} else {
			return rhsCastAsBSG->equals(this);
		}
	} else {
		return this->literal == rhsCast->literal;
	}
}

bool LiteralSymbolGroup::disjoint(const SymbolGroup* rhs) const {
	return !equals(rhs);
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> LiteralSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) {
	if(equals(rhs.get())) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	} else {
		ByteSymbolGroup* rhsCastAsBSG = dynamic_cast<ByteSymbolGroup*>(rhs.get());
		if (rhsCastAsBSG == nullptr || rhsCastAsBSG->disjoint(this)) {
			return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhs, true } });
		} else {
			std::shared_ptr<ByteSymbolGroup> bsg = std::make_shared<ByteSymbolGroup>((CharType)literal[0], (CharType)literal[0]);
			auto disjoinmentResult = rhsCastAsBSG->disjoinFrom(bsg);
			for (auto& pair : disjoinmentResult) {
				pair.second = !pair.second; // flipping sides
			}
			this->literal[0] = rhsCastAsBSG->rangeStart; // == bsg->rangeEnd (remember?)
			rhsCastAsBSG->rangeStart = bsg->rangeStart;
			rhsCastAsBSG->rangeEnd = bsg->rangeEnd;
			return disjoinmentResult;
		}
	}
}

std::string LiteralSymbolGroup::toString() const {
	return "'" + this->literal + "'";
}

std::shared_ptr<std::list<SymbolIndex>> LiteralSymbolGroup::retrieveSymbolIndices() const {
	throw std::logic_error("retrieveSymbolIndices() called on LiteralSymbolGroup -- an invalid call");
	// make it into a(n internal) warning and just return an empty list, continuing with execution
	return std::shared_ptr<std::list<SymbolIndex>>();
}

bool StatementSymbolGroup::isEmpty() const {
	return false;
}

bool StatementSymbolGroup::equals(const SymbolGroup* rhs) const {
	const StatementSymbolGroup* ssgPtr = dynamic_cast<const StatementSymbolGroup*>(rhs);
	if (ssgPtr == nullptr) {
		return false;
	}

	return ssgPtr->statement == statement;
}

bool StatementSymbolGroup::disjoint(const SymbolGroup* rhs) const {
	if (rhs == this) {
		return false;
	}

	const StatementSymbolGroup* rhsAsStatementSymbolGroup = dynamic_cast<const StatementSymbolGroup*>(rhs);
	if (rhsAsStatementSymbolGroup == nullptr) {
		return true;
	}

	if (this->statement == rhsAsStatementSymbolGroup->statement) {
		return false;
	}

	const CategoryStatement* rhsStatementAsCategoryStatement = dynamic_cast<const CategoryStatement*>(rhsAsStatementSymbolGroup->statement);
	if (rhsStatementAsCategoryStatement != nullptr) {
		if (rhsStatementAsCategoryStatement->categoricallyRefersTo(this->statement)) {
			return false;
		}
	}

	const CategoryStatement* thisStatementAsCategoryStatement = dynamic_cast<const CategoryStatement*>(statement);
	if (thisStatementAsCategoryStatement == nullptr) {
		return true;
	}

	if (thisStatementAsCategoryStatement->categoricallyRefersTo(rhsAsStatementSymbolGroup->statement)) {
		return false;
	}

	return true;
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> StatementSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) {
	if (rhs.get() == this || equals(rhs.get())) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	}

	const StatementSymbolGroup* rhsAsStatementSymbolGroup = dynamic_cast<const StatementSymbolGroup*>(rhs.get());
	if (rhsAsStatementSymbolGroup == nullptr) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhs, true } });
	}

	const CategoryStatement* rhsStatementAsCategoryStatement = dynamic_cast<const CategoryStatement*>(rhsAsStatementSymbolGroup->statement);
	if (rhsStatementAsCategoryStatement != nullptr) {
		std::set<const AttributedStatement*> disjoined = rhsStatementAsCategoryStatement->unpickReferal(statement);
		std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> ret;
		for (const AttributedStatement* as : disjoined) {
			auto asAsTypeForming = dynamic_cast<const TypeFormingStatement*>(as);
			if (asAsTypeForming != nullptr) {
				ret.emplace_back(std::make_shared<StatementSymbolGroup>(asAsTypeForming, rhsAsStatementSymbolGroup->statementMachine), true); // comes from rhs, hence true
			}
		}

		return ret;
	}

	const CategoryStatement* thisStatementAsCategoryStatement = dynamic_cast<const CategoryStatement*>(statement);
	if (thisStatementAsCategoryStatement == nullptr) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhs, true } });
	}

	if (thisStatementAsCategoryStatement->categoricallyRefersTo(rhsAsStatementSymbolGroup->statement)) {
		std::set<const AttributedStatement*> disjoinment = thisStatementAsCategoryStatement->unpickReferal(rhsAsStatementSymbolGroup->statement);
		this->statement = rhsAsStatementSymbolGroup->statement;
		this->statementMachine = rhsAsStatementSymbolGroup->statementMachine;

		std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> ret;
		for (const AttributedStatement* as : disjoinment) {
			auto asAsTypeForming = dynamic_cast<const TypeFormingStatement*>(as);
			if (asAsTypeForming != nullptr) {
				ret.emplace_back(std::make_shared<StatementSymbolGroup>(asAsTypeForming, rhsAsStatementSymbolGroup->statementMachine), false); // comes from lhs, hence false
			}
		}
		return ret;
	}

	return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhs, true } });
}

std::string StatementSymbolGroup::toString() const {
	return statement->name;
}

std::shared_ptr<std::list<SymbolIndex>> StatementSymbolGroup::retrieveSymbolIndices() const {
	if (m_symbolIndicesFlyweight->empty()) {
		auto referencedProductions = statement->calculateInstandingProductions();
		for (const ProductionStatement* referencedComponentPtr : referencedProductions) {
			if (referencedComponentPtr->terminality != Terminality::Terminal) {
				throw std::logic_error("Calling retrieveSymbolIndices on at least partially non-terminal statement");
			}
			m_symbolIndicesFlyweight->push_back(referencedComponentPtr->terminalTypeIndex);
		}
	}

	return m_symbolIndicesFlyweight;
}
