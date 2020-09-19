#include "LLkFirster.h"

#include "LLkBuilder.h"
#include <queue>
#include "SemanticAnalysisException.h"

LLkFirster::LLkFirster(const MachineDefinition& machine)
	: m_machine(machine) { }

SymbolGroupList LLkFirster::visit(const CategoryStatement* cs, const SymbolGroupList& prefix) {
	SymbolGroupList ret;

	for (const auto& categoryReferencePair : cs->references) {
		ret += categoryReferencePair.second.statement->first(this, prefix);
	}

	return ret;
}

SymbolGroupList LLkFirster::visit(const RuleStatement* rs, const SymbolGroupList& prefix) {
	return rs->regex->first(this, prefix);
}

SymbolGroupList LLkFirster::visit(const RepetitiveRegex* rr, const SymbolGroupList& prefix) {
	const auto& atom = rr->regex;

	unsigned long counter = 0;

	SymbolGroupList ret;
	std::list<SymbolGroupList::const_iterator> currentQueueOfPrefixEnds, nextQueueOfPrefixEnds({ prefix.cbegin() });

	auto& currentPrefixEnd = currentQueueOfPrefixEnds.front();
	auto currentPrefix = SymbolGroupList(prefix.cbegin(), currentPrefixEnd);
	while (!nextQueueOfPrefixEnds.empty() && counter < rr->maxRepetitions) {
		currentQueueOfPrefixEnds = nextQueueOfPrefixEnds;
		while (!currentQueueOfPrefixEnds.empty()) {
			ILLkFirstableCPtr atomicRegexAsFirstable = dynamic_cast<ILLkFirstableCPtr>(atom.get());
			auto thisPartsFirst = atomicRegexAsFirstable->first(this, currentPrefix);
			if (thisPartsFirst.containsEmpty()) {
				nextQueueOfPrefixEnds.push_back(currentPrefixEnd);
				if (rr->maxRepetitions == rr->INFINITE_REPETITIONS) {
					throw SemanticAnalysisException("The repetitive regex used at " + rr->locationString() + " permits infinite repetition of a regex that may derive to empty production -- this may lead to infinite loop of the program, hence the error");
				}
			}
			thisPartsFirst.removeEmpty();

			if (!thisPartsFirst.empty()) {
				if (currentPrefixEnd != prefix.cend()) {
					if (thisPartsFirst.contains(*currentPrefixEnd)) {
						auto advancedCurrentPrefixEnd = currentPrefixEnd;
						++advancedCurrentPrefixEnd;
						currentQueueOfPrefixEnds.push_back(advancedCurrentPrefixEnd);
					}
				} else {
					ret += thisPartsFirst;
				}
			}
		}
		++counter;
	}

	return ret;
}

SymbolGroupList LLkFirster::visit(const DisjunctiveRegex* dr, const SymbolGroupList& prefix) {
	SymbolGroupList ret;

	for (const auto& aBitOfDisjunctions : dr->disjunction) {
		ret += aBitOfDisjunctions->first(this, prefix);
	}

	return ret;
}

SymbolGroupList LLkFirster::visit(const ConjunctiveRegex* cr, const SymbolGroupList& prefix) {
	auto conjunctionIt = cr->conjunction.cbegin();

	SymbolGroupList ret;
	std::list<std::pair<SymbolGroupList::const_iterator, SymbolGroupList::const_iterator>> currentQueueOfPrefixEnds, nextQueueOfPrefixEnds({ {prefix.cbegin(), prefix.cbegin() } });

	while(!nextQueueOfPrefixEnds.empty() && conjunctionIt != cr->conjunction.cend()) {
		currentQueueOfPrefixEnds = nextQueueOfPrefixEnds;
		nextQueueOfPrefixEnds.clear();
		while(!currentQueueOfPrefixEnds.empty()) {
			auto currentPrefixPair = currentQueueOfPrefixEnds.front();
			auto currentPrefix = SymbolGroupList(currentPrefixPair.first, currentPrefixPair.second);

			ILLkFirstableCPtr rootRegexAsFirstable = dynamic_cast<ILLkFirstableCPtr>(conjunctionIt->get());
			auto thisPartsFirst = rootRegexAsFirstable->first(this, currentPrefix);
			if (thisPartsFirst.containsEmpty()) {
				nextQueueOfPrefixEnds.emplace_back(currentPrefixPair.second, currentPrefixPair.second);
				thisPartsFirst.removeEmpty();
			}
			
			if (!thisPartsFirst.empty()) {
				if (currentPrefixPair.second != prefix.cend()) {
					if (thisPartsFirst.contains(*currentPrefixPair.second)) {
						auto advancedCurrentPrefixEnd = currentPrefixPair.second;
						++advancedCurrentPrefixEnd;
						currentQueueOfPrefixEnds.emplace_back(currentPrefixPair.first, advancedCurrentPrefixEnd);
					}
				} else {
					ret += thisPartsFirst;
				}
			}

			currentQueueOfPrefixEnds.pop_front();
		}
		++conjunctionIt;
	}

	return ret;
}

SymbolGroupList LLkFirster::visit(const EmptyRegex* er, const SymbolGroupList& prefix) {
	if (prefix.empty()) {
		return SymbolGroupList({ std::make_shared<EmptySymbolGroup>() });
	} else {
		return SymbolGroupList();
	}
}

SymbolGroupList LLkFirster::visit(const ReferenceRegex* rr, const SymbolGroupList& prefix) {
	if (rr->referenceStatementMachine == &this->m_machine) {
		return rr->referenceStatement->first(this, prefix);
	} else {
		auto statementAsTypeForming = dynamic_cast<const TypeFormingStatement*>(rr->referenceStatement); // if it does not originate from this machine then it must be type-forming as it is a root!
		return SymbolGroupList({ std::make_shared<StatementSymbolGroup>(statementAsTypeForming, rr->referenceStatementMachine) });
	}
}

SymbolGroupList LLkFirster::visit(const AnyRegex* ar, const SymbolGroupList& prefix) {
	if (prefix.empty()) {
		return ar->makeSymbolGroups();
	} else if (prefix.size() == 1) {
		return SymbolGroupList({ std::make_shared<EmptySymbolGroup>() });
	} else {
		return SymbolGroupList();
	}
}

SymbolGroupList LLkFirster::visit(const ExceptAnyRegex* ar, const SymbolGroupList& prefix) {
	if (prefix.empty()) {
		return ar->makeSymbolGroups();
	} else if (prefix.size() == 1) {
		return SymbolGroupList({ std::make_shared<EmptySymbolGroup>() });
	} else {
		return SymbolGroupList();
	}
}

SymbolGroupList LLkFirster::visit(const LiteralRegex* lr, const SymbolGroupList& prefix) {
	if (!m_machine.isOnTerminalInput()) {
		throw SemanticAnalysisException("Attemtping to capture the literal '" + lr->literal + "' at " + lr->locationString() + " within the context of machine '" + m_machine.name + "' that may see non-terminal productions on its input ('" + m_machine.name + "' is on '" + m_machine.on.second->name + "')");
	}

	if (prefix.empty()) {
		return SymbolGroupList({ std::make_shared<LiteralSymbolGroup>(lr->literal) });
	} else if (prefix.size() == 1) {
		return SymbolGroupList({ std::make_shared<EmptySymbolGroup>() });
	} else {
		return SymbolGroupList();
	}
}

SymbolGroupList LLkFirster::visit(const ArbitrarySymbolRegex* asr, const SymbolGroupList& prefix) {
	if (prefix.empty()) {
		return SymbolGroupList({ m_machine.computeArbitrarySymbolGroup() });
	} else if (prefix.size() == 1) {
		return SymbolGroupList({ std::make_shared<EmptySymbolGroup>()  });
	} else {
		return SymbolGroupList();
	}
}
