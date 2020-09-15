#include "LLkBuilder.h"

#include "SemanticAnalysisException.h"

LLkBuilder::LLkBuilder(unsigned long k, const MachineDefinition& context)
	: m_k(k), m_contextMachine(context) {}

void LLkBuilder::visit(const CategoryStatement* categoryStatement) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[categoryStatement];

	std::list<ILLkNonterminalCPtr> alternatives;
	for (const auto& categoryReferencePair : categoryStatement->references) {
		ILLkNonterminalCPtr alternative = categoryReferencePair.second.statement;
		alternatives.push_back(alternative);
		registerContextAppearance(alternative, categoryStatement, std::list<ILLkNonterminalCPtr>());
	}
	disambiguate(alternatives);
	fillDisambiguationParent(categoryStatement, alternatives);
}

void LLkBuilder::visit(const RuleStatement* productionStatement) {
	LLkFlyweight& ruleFlyweight = m_flyweights[productionStatement];

	auto regexPtr = productionStatement->regex;
	auto regexPtrCast = std::dynamic_pointer_cast<ILLkBuildable>(regexPtr);
	regexPtrCast->accept(this);

	LLkFlyweight& regexFlyweight = m_flyweights[regexPtr.get()];
	ruleFlyweight.decisions = regexFlyweight.decisions;
}

void LLkBuilder::visit(const DisjunctiveRegex* regex) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[regex];

	std::list<ILLkNonterminalCPtr> alternatives;
	for (const auto& conjunction : regex->disjunction) {
		ILLkNonterminalCPtr conjunctionCast = conjunction.get();
		alternatives.push_back(conjunctionCast);
		registerContextAppearance(conjunctionCast, regex, std::list<ILLkNonterminalCPtr>());

		ILLkBuildableCPtr conjunctionCastAsBuildable = conjunction.get();
		conjunctionCastAsBuildable->accept(this);
	}
	disambiguate(alternatives);
	fillDisambiguationParent(regex, alternatives);
}

void LLkBuilder::visit(const ConjunctiveRegex* regex) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[regex];

	std::list<ILLkNonterminalCPtr> conjunctionBits;
	for (auto it = regex->conjunction.rbegin(); it != regex->conjunction.rend();++it) {
		const auto& rootRegex = *it;
		ILLkNonterminalCPtr ptr = dynamic_cast<ILLkNonterminalCPtr>(rootRegex.get());
		if(ptr != nullptr) {
			conjunctionBits.push_front(ptr);
			registerContextAppearance(ptr, regex, conjunctionBits);
		}

		ILLkBuildableCPtr rootRegexCastAsBuildable = rootRegex.get();
		rootRegexCastAsBuildable->accept(this);
	}
	
	if (!conjunctionBits.empty()) {
		const auto& startFlyweight = m_flyweights[conjunctionBits.front()];
		correspondingFlyweight.decisions += startFlyweight.decisions;
	} else {
		correspondingFlyweight.decisions.transitions.emplace_back(std::make_shared<EmptySymbolGroup>());
	}
}

void LLkBuilder::visit(const RepetitiveRegex* regex) {
	auto& repetitiveFlyweight = m_flyweights[regex];
	ILLkBuildableCPtr atomBuildable = regex->regex.get();
	atomBuildable->accept(this);

	ILLkNonterminalCPtr atomAsNonterminal = dynamic_cast<ILLkNonterminalCPtr>(regex->regex.get());
	if(atomAsNonterminal != nullptr) {
		auto& atomFlyweight = m_flyweights[atomAsNonterminal];

		auto repetitionCap = regex->maxRepetitions == regex->INFINITE_REPETITIONS ? regex->minRepetitions : regex->maxRepetitions;
		std::list<ILLkNonterminalCPtr> certainContinuation(repetitionCap, atomAsNonterminal);
		certainContinuation.pop_front();
		for (size_t counter = 0; counter < repetitionCap; ++counter) {
			atomFlyweight.contexts.emplace_back(regex, certainContinuation);
		}

		if (regex->maxRepetitions == regex->INFINITE_REPETITIONS) {
			atomFlyweight.contexts.emplace_back(regex, std::list<ILLkNonterminalCPtr>({ regex->kleeneTail().get() }));
		}
	}
}

void LLkBuilder::disambiguate(const std::list<ILLkNonterminalCPtr>& alternatives) {
	auto firstIterator = alternatives.begin();
	for (; firstIterator != alternatives.end(); ++firstIterator) {
		auto secondIterator = firstIterator;
		++secondIterator;
		for (; secondIterator != alternatives.end(); ++secondIterator) {
			disambiguatePair(*firstIterator, *secondIterator);
		}
	}
}

void LLkBuilder::disambiguatePair(ILLkNonterminalCPtr first, ILLkNonterminalCPtr second) {
	std::list<std::shared_ptr<SymbolGroup>> currentPrefix;
	LLkFlyweight &firstFlyweight = m_flyweights[first], &secondFlyweight = m_flyweights[second];
	LLkDecisionPoint &currentFirstPoint = firstFlyweight.decisions, &currentSecondPoint = secondFlyweight.decisions;
	
	SymbolGroupList prefix;
	disambiguateDecisionPoints(first, second, currentFirstPoint, currentSecondPoint, prefix);
}

void LLkBuilder::disambiguateDecisionPoints(ILLkNonterminalCPtr first, ILLkNonterminalCPtr second, LLkDecisionPoint& firstPoint, LLkDecisionPoint& secondPoint, SymbolGroupList& prefix) {
	if (firstPoint.transitions.empty()) {
		lookahead(first, prefix);
	}
	if (secondPoint.transitions.empty()) {
		lookahead(second, prefix);
	}

	// TODO: if both first.transitions and second.transitions contain EndOfGrammarSymbolGroup, we are seeing a fully ambiguous pair of rules
	
	auto firstIterator = firstPoint.transitions.begin();
	for (; firstIterator != firstPoint.transitions.end(); ++firstIterator) {
		auto secondIterator = secondPoint.transitions.begin();
		for (; secondIterator != secondPoint.transitions.end(); ++secondIterator) {
			if (!(*firstIterator)->condition->disjoint((*secondIterator)->condition.get())) {
				auto disjoinmentOutcome = (*firstIterator)->condition->disjoinFrom((*secondIterator)->condition);
				auto& firstDecisionPoint = (*firstIterator)->point;
				auto& secondDecisionPoint = (*secondIterator)->point;
				for (const auto& djPair : disjoinmentOutcome) {
					if (!djPair.first) {
						firstPoint.transitions.emplace_back(djPair.second, firstDecisionPoint);
					} else {
						secondPoint.transitions.emplace_back(djPair.second, secondDecisionPoint);
					}
				}

				// now, at this point, the challenge is to handle the deeper-level difference
				prefix.push_back((*firstIterator)->condition); // firstIterator->condition is guaranteed to be modified by the above to be the overlap symbol group
				disambiguateDecisionPoints(first, second, firstDecisionPoint, secondDecisionPoint, prefix);
				prefix.pop_back();
			}
		}
	}
}

void LLkBuilder::fillDisambiguationParent(ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& alternatives) {
	auto& parentFlyweight = m_flyweights[parent];
	for (auto alternative : alternatives) {
		const auto& alternativeFlyweight = m_flyweights[alternative];
		parentFlyweight.decisions += alternativeFlyweight.decisions;
	}
}

SymbolGroupList LLkBuilder::lookahead(ILLkNonterminalCPtr nonterminal, const SymbolGroupList& prefix) {
	if (prefix.size() >= m_k) {
		throw SemanticAnalysisException("Lookahead of " + std::to_string(m_k) + " is insufficient, need to look further ahead");
	}

	auto& flyweight = m_flyweights[nonterminal];
	LLkDecisionPoint* currentDecisionPoint = &flyweight.decisions;

	auto prefixIt = prefix.begin();
	while (prefixIt != prefix.end()) {
		const auto& currentLookahead = prefix.front();
		auto fit = std::find(currentDecisionPoint->transitions.begin(), currentDecisionPoint->transitions.end(), [&currentLookahead](const LLkTransition& transition) {
			return currentLookahead->equals(transition.condition.get());
		});
		if (fit == currentDecisionPoint->transitions.end()) {
			break;
		}

		currentDecisionPoint = &(*fit)->point;
		++prefixIt;
	}

	if (!currentDecisionPoint->transitions.empty()) {
		return currentDecisionPoint->computeConditionSymbols();
	}

	auto lookaheadSymbols = nonterminal->first(prefix);
	if (lookaheadSymbols.containsEmpty()) {
		SymbolGroupList emptyPrefix;
		lookaheadSymbols.removeEmpty();
		for(const auto& context : flyweight.contexts) {
			const auto& followedBy = context.followedBy;
			auto followedByIt = followedBy.begin();
			auto furtherLookaheadDueToFollows = sequentialLookahead(followedByIt, followedBy.end(), emptyPrefix);
			lookaheadSymbols.insert(lookaheadSymbols.end(), furtherLookaheadDueToFollows.cbegin(), furtherLookaheadDueToFollows.cend());
			
			if (followedByIt == followedBy.end()) {
				auto furtherLookaheadDueToParent = this->lookahead(context.parent, emptyPrefix);
				lookaheadSymbols.insert(lookaheadSymbols.end(), furtherLookaheadDueToParent.cbegin(), furtherLookaheadDueToParent.cend());
			}
		}
	}

	for (const auto& lookaheadSymbol : lookaheadSymbols) {
		currentDecisionPoint->transitions.emplace_back(lookaheadSymbol);
	}

	return lookaheadSymbols;
}

SymbolGroupList LLkBuilder::sequentialLookahead(std::list<ILLkNonterminalCPtr>::const_iterator& sequenceIt, const std::list<ILLkNonterminalCPtr>::const_iterator& sequenceEnd, const SymbolGroupList& prefix) {
	SymbolGroupList ret;
	if (sequenceIt != sequenceEnd) {
		auto currentFirst = (*sequenceIt)->first(prefix);
		if (ret.containsEmpty()) {
			++sequenceIt;
			currentFirst.removeEmpty();
			ret.insert(ret.end(), currentFirst.cbegin(), currentFirst.cend());

			auto furtherLookahead = sequentialLookahead(sequenceIt, sequenceEnd, prefix);
			ret.insert(ret.end(), furtherLookahead.cbegin(), furtherLookahead.cend());
		} else {
			ret.insert(ret.end(), currentFirst.cbegin(), currentFirst.cend());
		}
	}
}

void LLkBuilder::registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& followedBy) {
	auto retPair = m_flyweights.emplace(target, LLkFlyweight());
	const auto& iterator = retPair.first;
	LLkFlyweight& correspondingFlyweight = iterator->second;

	LLkFlyweight& correspondingFlyweight = m_flyweights[target];
	auto fit = std::find_if(correspondingFlyweight.contexts.begin(), correspondingFlyweight.contexts.end(), [&parent, &followedBy](const LLkNonterminalContext& context) {
		return context.parent == parent && context.followedBy == followedBy;
	});

	if (fit == correspondingFlyweight.contexts.end()) {
		correspondingFlyweight.contexts.emplace_back(parent, followedBy);
	}
}

void LLkBuilder::registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, std::list<ILLkNonterminalCPtr>::const_iterator followedByIt, std::list<ILLkNonterminalCPtr>::const_iterator followedByEnd) {
	registerContextAppearance(target, parent, std::list<ILLkNonterminalCPtr>(followedByIt, followedByEnd));
}

SymbolGroupList LLkDecisionPoint::computeConditionSymbols() const {
	SymbolGroupList ret;

	for (const auto& transition : transitions) {
		ret.push_back(transition->condition);
	}

	return ret;
}

LLkDecisionPoint& LLkDecisionPoint::operator+=(const LLkDecisionPoint& rhs) {
	for (const auto& incomingTransition : rhs.transitions) {
		auto fit = std::find_if(transitions.begin(), transitions.end(), [&incomingTransition](const LLkTransition& t) {
			return t.condition->equals(incomingTransition->condition.get());
			});

		if (fit == transitions.end()) {
			transitions.insert(fit, incomingTransition);
		} else {
			(*fit)->point += incomingTransition->point;
		}
	}
	this->transitions.insert(this->transitions.end(), rhs.transitions.cbegin(), rhs.transitions.cend());
}
