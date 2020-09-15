#include "SymbolGroup.h"
#include "CharType.h"

#include <algorithm>
#include "SyntacticTree.h"
#include "MachineStatement.h"

bool ByteSymbolGroup::equals(const SymbolGroup* rhs) const {
	const ByteSymbolGroup* rhsCast = dynamic_cast<const ByteSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		return false;
	} else {
		return this->rangeStart == rhsCast->rangeStart && this->rangeEnd == rhsCast->rangeEnd;
	}
}

bool ByteSymbolGroup::disjoint(const SymbolGroup* rhs) const {
	const ByteSymbolGroup* rhsCast = dynamic_cast<const ByteSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		return true;
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
	const ByteSymbolGroup* rhs = dynamic_cast<ByteSymbolGroup*>(rhsUncast.get());
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

	return ret;
}

std::shared_ptr<std::list<SymbolIndex>> ByteSymbolGroup::retrieveSymbolIndices() const {
	if (m_symbolIndicesFlyweight->empty()) {
		for (ComputationCharType it = rangeStart; it <= (ComputationCharType)rangeEnd; ++it) {
			m_symbolIndicesFlyweight->push_back(it);
		}
	}

	return m_symbolIndicesFlyweight;
}

bool EmptySymbolGroup::equals(const SymbolGroup* rhs) const {
	return dynamic_cast<const EmptySymbolGroup*>(rhs) != nullptr;
}

bool EmptySymbolGroup::disjoint(const SymbolGroup* rhs) const {
	return dynamic_cast<const EmptySymbolGroup*>(rhs) == nullptr;
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> EmptySymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) {
	if (rhs->equals(this)) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	}

	if (dynamic_cast<EmptySymbolGroup*>(rhs.get()) == nullptr) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool >>({ { rhs, true } });
	} else {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	}
}

std::shared_ptr<std::list<SymbolIndex>> EmptySymbolGroup::retrieveSymbolIndices() const {
	return std::shared_ptr<std::list<SymbolIndex>>();
}

bool TerminalSymbolGroup::equals(const SymbolGroup* rhs) const {
	const TerminalSymbolGroup* rhsCast = dynamic_cast<const TerminalSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		return false;
	} else {
		return this->referencedProductions == rhsCast->referencedProductions;
	}
}

bool TerminalSymbolGroup::disjoint(const SymbolGroup* rhs) const {
	const TerminalSymbolGroup* rhsCast = dynamic_cast<const TerminalSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		return true;
	} else {
		for (const auto referencedComponentPtr : referencedProductions) {
			auto fit = std::find_if(rhsCast->referencedProductions.cbegin(), rhsCast->referencedProductions.cend(), [referencedComponentPtr](const MachineStatement* rhsComponentPtr) {
				return referencedComponentPtr->name == rhsComponentPtr->name;
				});
			if (fit != rhsCast->referencedProductions.cend()) {
				return false;
			}
		}

		return true;
	}
}

std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> TerminalSymbolGroup::disjoinFrom(const std::shared_ptr<SymbolGroup>& rhsUncast) {
	if (rhsUncast->equals(this)) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>();
	}

	const TerminalSymbolGroup* rhs = dynamic_cast<const TerminalSymbolGroup*>(rhsUncast.get());
	if (rhs == nullptr) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhsUncast, true } });
	}

	std::list<const ProductionStatement*> sharedComponents;
	std::list<const ProductionStatement*> excludedComponents;
	for (auto it = referencedProductions.begin(); it != referencedProductions.end(); ) {
		auto fit = std::find_if(rhs->referencedProductions.cbegin(), rhs->referencedProductions.cend(), [it](const ProductionStatement* rhsComponentPtr) {
			return (*it)->name == rhsComponentPtr->name;
			});
		if (fit != rhs->referencedProductions.cend()) {
			sharedComponents.push_back(*fit);
			it = referencedProductions.erase(it);
		} else {
			excludedComponents.push_back(*fit);
			++it;
		}
	}

	if (sharedComponents.empty()) {
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhsUncast, true } });
	} else {
		std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>> ret;
		ret.emplace_back(std::make_shared<TerminalSymbolGroup>(sharedComponents), false);
		ret.emplace_back(std::make_shared<TerminalSymbolGroup>(sharedComponents), true);
		ret.emplace_back(std::make_shared<TerminalSymbolGroup>(excludedComponents), true);

		return ret;
	}
}

std::shared_ptr<std::list<SymbolIndex>> TerminalSymbolGroup::retrieveSymbolIndices() const {
	if (m_symbolIndicesFlyweight->empty()) {
		for (const ProductionStatement* referencedComponentPtr : referencedProductions) {
			m_symbolIndicesFlyweight->push_back(referencedComponentPtr->terminalTypeIndex);
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
		if (dynamic_cast<const EmptySymbolGroup*>(sgPtr.get()) != nullptr) {
			return true;
		}
	}

	return false;
}

SymbolGroupList SymbolGroupList::allButEmpty() const {
	SymbolGroupList ret;
	for (const auto& sgPtr : *this) {
		if (dynamic_cast<const EmptySymbolGroup*>(sgPtr.get()) != nullptr) {
			ret.push_back(sgPtr);
		}
	}

	return ret;
}

void SymbolGroupList::removeEmpty() {
	std::remove_if(this->begin(), this->end(), [](const auto& ptr) {
		return dynamic_cast<const EmptySymbolGroup*>(ptr.get()) != nullptr;
	});
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
}

bool LiteralSymbolGroup::equals(const SymbolGroup* rhs) const {
	const LiteralSymbolGroup* rhsCast = dynamic_cast<const LiteralSymbolGroup*>(rhs);
	if (rhsCast == nullptr) {
		return false;
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
		return std::list<std::pair<std::shared_ptr<SymbolGroup>, bool>>({ { rhs, true } });
	}
}
