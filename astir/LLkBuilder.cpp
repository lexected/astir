#include "LLkBuilder.h"

#include "SemanticAnalysisException.h"

LLkBuilder::LLkBuilder(unsigned long k, const MachineDefinition& context)
	: m_k(k), m_contextMachine(context), m_firster(context) {}

void LLkBuilder::visit(const CategoryStatement* categoryStatement) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[categoryStatement];

	std::list<ILLkFirstableCPtr> alternatives;
	for (const auto& categoryReferencePair : categoryStatement->references) {
		ILLkFirstableCPtr alternative = categoryReferencePair.second.statement;
		alternatives.push_back(alternative);
		registerContextAppearance(alternative, categoryStatement, std::list<ILLkFirstableCPtr>());
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

	std::list<ILLkFirstableCPtr> alternatives;
	for (const auto& conjunction : regex->disjunction) {
		ILLkFirstableCPtr conjunctionCast = conjunction.get();
		alternatives.push_back(conjunctionCast);
		registerContextAppearance(conjunctionCast, regex, std::list<ILLkFirstableCPtr>());

		ILLkBuildableCPtr conjunctionCastAsBuildable = conjunction.get();
		conjunctionCastAsBuildable->accept(this);
	}
	disambiguate(alternatives);
	fillDisambiguationParent(regex, alternatives);
}

void LLkBuilder::visit(const ConjunctiveRegex* regex) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[regex];

	std::list<ILLkFirstableCPtr> conjunctionBits;
	for (auto it = regex->conjunction.rbegin(); it != regex->conjunction.rend();++it) {
		const auto& rootRegex = *it;
		ILLkFirstableCPtr ptr = dynamic_cast<ILLkFirstableCPtr>(rootRegex.get());
		if(ptr != nullptr) {
			conjunctionBits.push_front(ptr);
			registerContextAppearance(ptr, regex, conjunctionBits);
		}

		/*ILLkBuildableCPtr rootRegexCastAsBuildable = rootRegex.get();
		rootRegexCastAsBuildable->accept(this);*/
	}
	
	if (!conjunctionBits.empty()) {
		const auto& startFlyweight = m_flyweights[conjunctionBits.front()];
		correspondingFlyweight.decisions += startFlyweight.decisions;
	} else {
		correspondingFlyweight.decisions.transitions.push_back(std::make_shared<LLkTransition>(std::make_shared<EmptySymbolGroup>()));
	}
}

void LLkBuilder::visit(const RepetitiveRegex* regex) {
	auto& repetitiveFlyweight = m_flyweights[regex];
	/*ILLkBuildableCPtr atomBuildable = regex->regex.get();
	atomBuildable->accept(this);*/

	ILLkFirstableCPtr atomAsNonterminal = dynamic_cast<ILLkFirstableCPtr>(regex->regex.get());
	if(atomAsNonterminal != nullptr) {
		auto& atomFlyweight = m_flyweights[atomAsNonterminal];

		auto repetitionCap = regex->maxRepetitions == regex->INFINITE_REPETITIONS ? regex->minRepetitions : regex->maxRepetitions;
		std::list<ILLkFirstableCPtr> certainContinuation(repetitionCap, atomAsNonterminal);
		certainContinuation.pop_front();
		for (size_t counter = 0; counter < repetitionCap; ++counter) {
			atomFlyweight.contexts.emplace_back(regex, certainContinuation);
		}

		if (regex->maxRepetitions == regex->INFINITE_REPETITIONS) {
			atomFlyweight.contexts.emplace_back(regex, std::list<ILLkFirstableCPtr>({ regex->kleeneTail().get() }));
		}
	}
}

void LLkBuilder::disambiguate(const std::list<ILLkFirstableCPtr>& alternatives) {
	auto firstIterator = alternatives.begin();
	for (; firstIterator != alternatives.end(); ++firstIterator) {
		auto secondIterator = firstIterator;
		++secondIterator;
		for (; secondIterator != alternatives.end(); ++secondIterator) {
			disambiguatePair(*firstIterator, *secondIterator);
		}
	}
}

void LLkBuilder::disambiguatePair(ILLkFirstableCPtr first, ILLkFirstableCPtr second) {
	std::list<std::shared_ptr<SymbolGroup>> currentPrefix;
	LLkFlyweight &firstFlyweight = m_flyweights[first], &secondFlyweight = m_flyweights[second];
	LLkDecisionPoint &currentFirstPoint = firstFlyweight.decisions, &currentSecondPoint = secondFlyweight.decisions;
	
	SymbolGroupList prefix;
	disambiguateDecisionPoints(first, second, currentFirstPoint, currentSecondPoint, prefix);
}

void LLkBuilder::disambiguateDecisionPoints(ILLkFirstableCPtr first, ILLkFirstableCPtr second, LLkDecisionPoint& firstPoint, LLkDecisionPoint& secondPoint, SymbolGroupList& prefix) {
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
					if (!djPair.second) {
						firstPoint.transitions.push_back(std::make_shared<LLkTransition>(djPair.first, firstDecisionPoint));
					} else {
						secondPoint.transitions.push_back(std::make_shared<LLkTransition>(djPair.first, secondDecisionPoint));
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

void LLkBuilder::fillDisambiguationParent(ILLkFirstableCPtr parent, const std::list<ILLkFirstableCPtr>& alternatives) {
	auto& parentFlyweight = m_flyweights[parent];
	for (auto alternative : alternatives) {
		const auto& alternativeFlyweight = m_flyweights[alternative];
		parentFlyweight.decisions += alternativeFlyweight.decisions;
	}
}

SymbolGroupList LLkBuilder::lookahead(ILLkFirstableCPtr nonterminal, const SymbolGroupList& prefix) {
	if (prefix.size() >= m_k) {
		throw SemanticAnalysisException("Lookahead of " + std::to_string(m_k) + " is insufficient, need to look further ahead");
	}

	auto& flyweight = m_flyweights[nonterminal];
	LLkDecisionPoint* currentDecisionPoint = &flyweight.decisions;

	auto prefixIt = prefix.begin();
	while (prefixIt != prefix.end()) {
		const auto& currentLookahead = prefix.front();
		auto fit = std::find_if(currentDecisionPoint->transitions.begin(), currentDecisionPoint->transitions.end(), [&currentLookahead](const auto& transitionPtr) {
			return currentLookahead->equals(transitionPtr->condition.get());
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

	auto lookaheadSymbols = nonterminal->first(&m_firster, prefix);
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
		currentDecisionPoint->transitions.push_back(std::make_shared<LLkTransition>(lookaheadSymbol));
	}

	return lookaheadSymbols;
}

SymbolGroupList LLkBuilder::sequentialLookahead(std::list<ILLkFirstableCPtr>::const_iterator& sequenceIt, const std::list<ILLkFirstableCPtr>::const_iterator& sequenceEnd, const SymbolGroupList& prefix) {
	SymbolGroupList ret;
	if (sequenceIt != sequenceEnd) {
		auto currentFirst = (*sequenceIt)->first(&m_firster, prefix);
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

	return ret;
}

void LLkBuilder::registerContextAppearance(ILLkFirstableCPtr target, ILLkFirstableCPtr parent, const std::list<ILLkFirstableCPtr>& followedBy) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[target];
	auto fit = std::find_if(correspondingFlyweight.contexts.begin(), correspondingFlyweight.contexts.end(), [&parent, &followedBy](const LLkNonterminalContext& context) {
		return context.parent == parent && context.followedBy == followedBy;
	});

	if (fit == correspondingFlyweight.contexts.end()) {
		correspondingFlyweight.contexts.emplace_back(parent, followedBy);
	}
}

void LLkBuilder::registerContextAppearance(ILLkFirstableCPtr target, ILLkFirstableCPtr parent, std::list<ILLkFirstableCPtr>::const_iterator followedByIt, std::list<ILLkFirstableCPtr>::const_iterator followedByEnd) {
	registerContextAppearance(target, parent, std::list<ILLkFirstableCPtr>(followedByIt, followedByEnd));
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
		auto fit = std::find_if(transitions.begin(), transitions.end(), [&incomingTransition](const auto& tPtr) {
			return tPtr->condition->equals(incomingTransition->condition.get());
			});

		if (fit == transitions.end()) {
			transitions.insert(fit, incomingTransition);
		} else {
			(*fit)->point += incomingTransition->point;
		}
	}
	this->transitions.insert(this->transitions.end(), rhs.transitions.cbegin(), rhs.transitions.cend());

	return *this;
}
