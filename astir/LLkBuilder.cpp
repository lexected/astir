#include "LLkBuilder.h"

#include "SemanticAnalysisException.h"

#include <algorithm>
#include "LLkParserDefinition.h"

LLkBuilder::LLkBuilder(const LLkParserDefinition* context)
	: m_contextMachine(context), m_firster(context) {}

void LLkBuilder::visitRootDisjunction(const std::list<std::shared_ptr<TypeFormingStatement>>& rootDisjunction) {
	std::list<ILLkNonterminalCPtr> disjunction;
	std::transform(rootDisjunction.cbegin(), rootDisjunction.cend(), std::back_inserter(disjunction), [](const auto& sptr) {
		return sptr.get();
	});
	disambiguate(disjunction);
}

void LLkBuilder::visit(const CategoryStatement* categoryStatement) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[categoryStatement];

	std::list<ILLkNonterminalCPtr> alternatives;
	for (const auto& categoryReferencePair : categoryStatement->references) {
		ILLkNonterminalCPtr alternative = categoryReferencePair.second.statement;
		alternatives.push_back(alternative);
		registerContextAppearance(alternative, categoryStatement, std::list<ILLkFirstableCPtr>());
	}
	disambiguate(alternatives);
	fillDisambiguationParent(categoryStatement, alternatives);
}

void LLkBuilder::visit(const RuleStatement* productionStatement) {
	LLkFlyweight& ruleFlyweight = m_flyweights[productionStatement];

	auto disjunctionPtr = productionStatement->regex;
	auto disjunctionPtrCast = std::dynamic_pointer_cast<ILLkBuilding>(disjunctionPtr);
	disjunctionPtrCast->accept(this);

	LLkFlyweight& disjunctionFlyweight = m_flyweights[disjunctionPtr.get()];
	ruleFlyweight.decisions = disjunctionFlyweight.decisions;
}

void LLkBuilder::visit(const DisjunctiveRegex* regex) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[regex];

	std::list<ILLkNonterminalCPtr> alternatives;
	for (const auto& conjunction : regex->disjunction) {
		ILLkNonterminalCPtr conjunctionCast = conjunction.get();
		alternatives.push_back(conjunctionCast);
		registerContextAppearance(conjunctionCast, regex, std::list<ILLkFirstableCPtr>());

		ILLkBuildingCPtr conjunctionCastAsBuildable = conjunction.get();
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
		ILLkFirstableCPtr firstablePtr = dynamic_cast<ILLkFirstableCPtr>(rootRegex.get());
		conjunctionBits.push_front(firstablePtr);

		const ReferenceRegex* referenceRegexPtr = dynamic_cast<const ReferenceRegex*>(rootRegex.get());
		if(referenceRegexPtr != nullptr && referenceRegexPtr->referenceStatementMachine == m_contextMachine) {
			ILLkNonterminalCPtr nonterminalPtr = referenceRegexPtr->referenceStatement;
			registerContextAppearance(nonterminalPtr, regex, conjunctionBits);
		}

		ILLkBuildingCPtr rootRegexCastAsBuildable = rootRegex.get();
		rootRegexCastAsBuildable->accept(this);
	}
	
	if (conjunctionBits.empty()) {
		correspondingFlyweight.decisions.transitions.push_back(std::make_shared<LLkTransition>(std::make_shared<EmptySymbolGroup>()));
	}
}

void LLkBuilder::visit(const RepetitiveRegex* regex) {
	auto& repetitiveFlyweight = m_flyweights[regex];
	ILLkBuildingCPtr atomBuildable = regex->regex.get();
	atomBuildable->accept(this);

	ILLkNonterminalCPtr atomAsNonterminal = dynamic_cast<ILLkNonterminalCPtr>(regex->regex.get());
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

void LLkBuilder::visit(const ReferenceRegex* regex) {
	/*if (regex->referenceStatementMachine != &m_contextMachine) {
		return;
	}

	ILLkBuildingCPtr regexStatementAsBuilding = regex->referenceStatement;
	regexStatementAsBuilding->accept(this);*/
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

	if(firstPoint.transitions.empty() || secondPoint.transitions.empty()) {
		if (m_contextMachine->attributes.find(MachineFlag::AmbiguityResolvedByPrecedence)->second.value) {
			// do nothing? will just doing that work? I think it should. But it should be tested
		} else {
			throw SemanticAnalysisException("Ambiguity of grammar identified on the lookahead sequence \"" + prefix.asSequenceString() + "\". Alternatives include using the `ambiguity_resolved_by_precedence` machine flag (i.e. in a `with` clause)");
		}
	}

	// TODO: if both first.transitions and second.transitions contain EndOfGrammarSymbolGroup, we are seeing a fully ambiguous pair of rules
	// another option is to check at the very end of this function whether both of the above are still empty -- if yes, we have have a fully ambiguous pair of rules as above
	// maybe don't make it an error but a warning...
	// furthermore, here, (!firstPoint.transitions.empty() && secondPoint.transitions.empty()) is a potentially permissible ambiguity (longer is parsed first) but (firstPoint.transitions.empty() && !secondPoint.transitions.empty()) is likely to be undesired

	auto firstIterator = firstPoint.transitions.begin();
	for (; firstIterator != firstPoint.transitions.end(); ++firstIterator) {
		auto secondIterator = secondPoint.transitions.begin();
		for (; secondIterator != secondPoint.transitions.end(); ++secondIterator) {
			if (!(*firstIterator)->condition->disjoint((*secondIterator)->condition.get())) {
				auto disjoinmentOutcome = (*firstIterator)->condition->disjoinFrom((*secondIterator)->condition);
				for (const auto& djPair : disjoinmentOutcome) {
					if (!djPair.second) {
						firstPoint.transitions.push_back(std::make_shared<LLkTransition>(djPair.first, (*firstIterator)->point));
					} else {
						secondPoint.transitions.push_back(std::make_shared<LLkTransition>(djPair.first, (*secondIterator)->point));
					}
				}

				// the reason for the following line is that the disjoinFrom procedures are written with a NFA building in mind
				// lhs of disjoinFrom is modified to contain the ovelap of the two non-disjoint conditions (lhs, rhs)
				// in case of NFAs, we simply need just one copy of that, but here where we are working on two separate decision trees,
				// the overlap transition needs to be present on the rhs as well as it is a completely separate decision tree
				auto secondDecisionPoint = (*secondIterator)->point;
				secondPoint.transitions.push_back(std::make_shared<LLkTransition>((*firstIterator)->condition));

				secondIterator = secondPoint.transitions.erase(secondIterator);
				if (secondIterator == secondPoint.transitions.end()) {
					continue;
					// should never, ever happen.
				}

				// now, at this point, the challenge is to handle the deeper-level difference
				prefix.push_back((*firstIterator)->condition); // firstIterator->condition is guaranteed to be modified by the above to be the overlap symbol group
				disambiguateDecisionPoints(first, second, (*firstIterator)->point, secondDecisionPoint, prefix);
				prefix.pop_back();
			}
		}
	}
}

void LLkBuilder::fillDisambiguationParent(ILLkNonterminalCPtr parent, const std::list<ILLkNonterminalCPtr>& alternatives) {
	// actually, most of the time, we don't want to do the following, so I am just commenting it out in case I later on realize that I need it
	/*auto& parentFlyweight = m_flyweights[parent];
	for (auto alternative : alternatives) {
		const auto& alternativeFlyweight = m_flyweights[alternative];
		parentFlyweight.decisions += alternativeFlyweight.decisions;
	}*/
}

SymbolGroupList LLkBuilder::lookahead(ILLkFirstableCPtr firstable, const SymbolGroupList& prefix) {
	if (prefix.size() >= m_contextMachine->k()) {
		throw SemanticAnalysisException("Lookahead of " + std::to_string(m_contextMachine->k()) + " is insufficient, need to look further ahead");
	}

	auto nonterminal = dynamic_cast<ILLkNonterminalCPtr>(firstable);
	if (nonterminal == nullptr) {
		return firstable->first(&m_firster, prefix);
	}

	auto& flyweight = m_flyweights[nonterminal];
	LLkDecisionPoint* currentDecisionPoint = &flyweight.decisions;

	auto prefixIt = prefix.begin();
	while (prefixIt != prefix.end()) {
		const auto& currentLookahead = *prefixIt;
		auto fit = std::find_if(currentDecisionPoint->transitions.begin(), currentDecisionPoint->transitions.end(), [&currentLookahead](const auto& transitionPtr) {
			return !currentLookahead->disjoint(transitionPtr->condition.get());
		});
		if (fit == currentDecisionPoint->transitions.end()) {
			/*
			The reason you don't want to throw this exception is that it breaks if some decision trees haven't yet been fully contextually filled in
			if(!currentDecisionPoint->transitions.empty()) {
				throw SemanticAnalysisException("Unrecognized prefix element sought in the decision point tree");
			}*/
			break;
		}

		currentDecisionPoint = &(*fit)->point;
		++prefixIt;
	}

	if (!currentDecisionPoint->transitions.empty()) {
		return currentDecisionPoint->computeConditionSymbols();
	}

	auto lookaheadSymbols = firstable->first(&m_firster, prefix);
	if (lookaheadSymbols.containsEmpty()) {
		SymbolGroupList emptyPrefix;
		lookaheadSymbols.removeEmpty();
		for(const auto& context : flyweight.contexts) {
			const auto& followedBy = context.followedBy;
			auto followedByIt = followedBy.begin();
			auto furtherLookaheadDueToFollows = sequentialLookahead(followedByIt, followedBy.end(), emptyPrefix);
			lookaheadSymbols += furtherLookaheadDueToFollows;
			
			if (followedByIt == followedBy.end()) {
				auto furtherLookaheadDueToParent = this->lookahead(context.parent, emptyPrefix);
				lookaheadSymbols += furtherLookaheadDueToParent;
			}
		}
	}

	for (const auto& lookaheadSymbol : lookaheadSymbols) {
		currentDecisionPoint->transitions.push_back(std::make_shared<LLkTransition>(lookaheadSymbol));
	}

	return lookaheadSymbols;
}

LLkDecisionPoint LLkBuilder::getDecisionTree(ILLkFirstableCPtr firstable) {
	auto nonterminal = dynamic_cast<ILLkNonterminalCPtr>(firstable);
	if (nonterminal == nullptr) {
		auto symbols = firstable->first(&m_firster, SymbolGroupList());

		LLkDecisionPoint ret;
		for (const auto& lookaheadSymbol : symbols) {
			ret.transitions.push_back(std::make_shared<LLkTransition>(lookaheadSymbol));
		}
		return ret;
	} else {
		lookahead(firstable, SymbolGroupList());
		return m_flyweights[nonterminal].decisions;
	}
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

void LLkBuilder::registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, const std::list<ILLkFirstableCPtr>& followedBy) {
	LLkFlyweight& correspondingFlyweight = m_flyweights[target];
	auto fit = std::find_if(correspondingFlyweight.contexts.begin(), correspondingFlyweight.contexts.end(), [&parent, &followedBy](const LLkNonterminalContext& context) {
		return context.parent == parent && context.followedBy == followedBy;
	});

	if (fit == correspondingFlyweight.contexts.end()) {
		correspondingFlyweight.contexts.emplace_back(parent, followedBy);
	}
}

void LLkBuilder::registerContextAppearance(ILLkNonterminalCPtr target, ILLkNonterminalCPtr parent, std::list<ILLkFirstableCPtr>::const_iterator followedByIt, std::list<ILLkFirstableCPtr>::const_iterator followedByEnd) {
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

size_t LLkDecisionPoint::maxDepth() const {
	size_t tmpMax = 0;
	for (const auto& transition : transitions) {
		tmpMax = std::max(tmpMax, transition->point.maxDepth() + 1);
	}
	return tmpMax;
}
