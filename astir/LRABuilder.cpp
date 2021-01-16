#include "LRABuilder.h"
#include "LLkFirster.h"

void LRABuilder::visit(const CategoryStatement* category, LRA* lra, AFAState attachmentState, SymbolGroupPtrVector lookahead) const {
	LRTag categoryTag(category, lookahead);

	auto tagIt = lra->tags.find(categoryTag);
	if (tagIt != lra->tags.end()) {
		// throw Exception("Tag for the category " + category->name + " already exists in the current LRA");
		lra->addEmptyTransition(attachmentState, tagIt->second);
	} else {
		AFAState categoryRootState = lra->addState();
		lra->addEmptyTransition(attachmentState, categoryRootState);

		lra->tagState(categoryRootState, categoryTag);

		for (const auto& categoryReferencePair : category->references) {
			AFAState referenceStatementStateAfterShift = lra->addState();
			lra->addTransition(categoryRootState, referenceStatementStateAfterShift, std::make_shared<StatementSymbolGroup>(categoryReferencePair.second.statement, &m_contextMachine));

			categoryReferencePair.second.statement->visit(lra, categoryRootState, lookahead);
		}
	}
}

void LRABuilder::visit(const PatternStatement* rule, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	rule->regex->accept(lra, startingState, lookahead);
}

void LRABuilder::visit(const ProductionStatement* rule, LRA* lra, AFAState attachmentState, SymbolGroupPtrVector lookahead) const {
	LRTag productionTag(rule, lookahead);

	auto tagIt = lra->tags.find(productionTag);
	if (tagIt != lra->tags.end()) {
		// throw Exception("Tag for the production " + category->name + " already exists in the current LRA");
		lra->addEmptyTransition(attachmentState, tagIt->second);
	} else {
		AFAState productionRootState = lra->addState();
		lra->addEmptyTransition(attachmentState, productionRootState);

		lra->tagState(productionRootState, productionTag);

		rule->regex->accept(lra, productionRootState, lookahead);
	}
}

void LRABuilder::visit(const RegexStatement* rule, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	rule->regex->accept(lra, startingState, lookahead);
}

void LRABuilder::visit(const DisjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	for (const auto& conjunctiveRegexPtr : regex->disjunction) {
		AFAState conjunctiveRegexStateAfterShift = lra->addState();
		lra->addEmptyTransition(startingState, conjunctiveRegexStateAfterShift);

		conjunctiveRegexPtr->accept(lra, conjunctiveRegexStateAfterShift, lookahead);
	}
}

void LRABuilder::visit(const ConjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector parentLookahead) const {
	auto it = regex->conjunction.cbegin();
	const auto endIt = regex->conjunction.cend();

	AFAState attachmentState = startingState;
	auto nextIt = ++it;
	for (; it != endIt;) {
		std::list<SymbolGroupPtrVector> lookaheadsForThisItem = this->computeItemLookaheads(nextIt, endIt, parentLookahead);

		for (const SymbolGroupPtrVector& lookahead : lookaheadsForThisItem) {
			(*it)->accept(lra, attachmentState, lookahead);

			AFAState newAttachmentState;
			if (lra->finalStates.size() == 1) {
				newAttachmentState = *lra->finalStates.cbegin();
			} else {
				newAttachmentState = lra->addState();
				for (AFAState endPointState : lra->finalStates) {
					lra->addEmptyTransition(endPointState, newAttachmentState);
				}
			}
			attachmentState = newAttachmentState;
			lra->finalStates.clear();
		}

		it = nextIt;
		++nextIt;
	}
}

void LRABuilder::visit(const RepetitiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector parentLookahead) const {
	/*
		Regex preparation phase
	*/
	const AtomicRegex* ar = regex->regex.get();
	ILLkFirstableCPtr arAsFirstable = dynamic_cast<ILLkFirstableCPtr>(ar);
	// because dealing with the ILLkNonterminal/ILLkFirstable diamond was too much for my gentle brain
	// and I have to hack this hierarchy ever sice.. tbf I remember I tried to diamond once and it was not great at all
	if (arAsFirstable == nullptr) {
		throw Exception("Internal exception -- the regex passed to what is essentially a LR(k) firster (LRABuilder::computeItemLookahead) was ILLkFirstableCPtr despite one's best attempts not to judge the regex by its pointer ");
		// TODO: change type to InternalException or something of the kind
	}

	/*
		Continuation lookaheads preparation phase
	*/
	const size_t lookaheadSize = parentLookahead.size();
	size_t lookaheadPolls = 0;
	std::list<SymbolGroupPtrVector> continuationLookaheads;
	if (regex->maxRepetitions == regex->INFINITE_REPETITIONS) {
		continuationLookaheads = this->computeItemLookaheads(
			[&arAsFirstable]() { return InformedLookaheadDescriptor(arAsFirstable, true); },
			[&lookaheadPolls]() { ++lookaheadPolls; },
			[&lookaheadPolls, &lookaheadSize]() { return lookaheadPolls == lookaheadSize; },
			parentLookahead
		);
	} else {
		continuationLookaheads = std::list<SymbolGroupPtrVector>({ parentLookahead });
	}

	/*
		Fixed prefix construction phase
	*/
	for(const auto& continuationLookahead : continuationLookaheads) {
		AFAState attachmentState = startingState;
		for (unsigned long i = 0; i < regex->minRepetitions; ++i) {
			unsigned long iNextCopy = ++i;
			std::list<SymbolGroupPtrVector> lookaheadsForThisItem = this->computeItemLookaheads(
				[&arAsFirstable]() { return InformedLookaheadDescriptor(arAsFirstable); },
				[&iNextCopy]() { ++iNextCopy; },
				[&iNextCopy, &regex]() { return iNextCopy == regex->minRepetitions; },
				continuationLookahead
			);
		
			for (const SymbolGroupPtrVector& lookahead : lookaheadsForThisItem) {
				ar->accept(lra, attachmentState, lookahead);

				AFAState newAttachmentState;
				if (lra->finalStates.size() == 1) {
					newAttachmentState = *lra->finalStates.cbegin();
				} else {
					newAttachmentState = lra->addState();
					for (AFAState endPointState : lra->finalStates) {
						lra->addEmptyTransition(endPointState, newAttachmentState);
					}
				}
				attachmentState = newAttachmentState;
				lra->finalStates.clear();
			}
		}
		lra->finalStates.insert(attachmentState);
	}

	/* Branch merging before the next phase */
	const auto interimAttachmentState = lra->addState();
	for (AFAState endPointState : lra->finalStates) {
		lra->addEmptyTransition(endPointState, interimAttachmentState);
	}
	lra->finalStates.clear();

	/*
		Optional postfix construction phase
	*/
	if(regex->maxRepetitions != regex->INFINITE_REPETITIONS) {
		AFAState attachmentState = interimAttachmentState;
		for (unsigned long i = regex->minRepetitions; i < regex->maxRepetitions; ++i) {
			unsigned long iNextCopy = ++i;
			std::list<SymbolGroupPtrVector> lookaheadsForThisItem = this->computeItemLookaheads(
				[&arAsFirstable]() { return InformedLookaheadDescriptor(arAsFirstable, true); },
				[&iNextCopy]() { ++iNextCopy; },
				[&iNextCopy, &regex]() { return iNextCopy == regex->minRepetitions; },
				parentLookahead
			);

			for (const SymbolGroupPtrVector& lookahead : lookaheadsForThisItem) {
				ar->accept(lra, attachmentState, lookahead);

				AFAState newAttachmentState;
				if (lra->finalStates.size() == 1) {
					newAttachmentState = *lra->finalStates.cbegin();
				} else {
					newAttachmentState = lra->addState();
					for (AFAState endPointState : lra->finalStates) {
						lra->addEmptyTransition(endPointState, newAttachmentState);
					}
				}
				lra->addEmptyTransition(newAttachmentState, interimAttachmentState);
				attachmentState = newAttachmentState;
				lra->finalStates.clear();
			}
		}
		lra->finalStates.insert(attachmentState);
	} else {
		/* here we handle the part under the Kleene star (in fact, the Kleene tail) */

		ar->accept(lra, interimAttachmentState, parentLookahead);
		AFAState endState;
		if (lra->finalStates.size() == 1) {
			endState = *lra->finalStates.cbegin();
		} else {
			endState = lra->addState();
			for (AFAState endPointState : lra->finalStates) {
				lra->addEmptyTransition(endPointState, endState);
			}
			
		}
		lra->finalStates.clear();
		lra->addEmptyTransition(endState, interimAttachmentState);
		lra->finalStates.insert(interimAttachmentState);
	}
}

void LRABuilder::visit(const EmptyRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	lra->finalStates.insert(startingState);
}

std::list<SymbolGroupPtrVector> LRABuilder::computeItemLookaheads(std::list<std::unique_ptr<RootRegex>>::const_iterator symbolPrecededByDotIt, std::list<std::unique_ptr<RootRegex>>::const_iterator endOfProductionIt, SymbolGroupPtrVector parentLookahead) const {
	this->computeItemLookaheads(
		[&symbolPrecededByDotIt]() {
			const RootRegex* regex = symbolPrecededByDotIt->get();
			ILLkFirstableCPtr regexAsFirstable = dynamic_cast<ILLkFirstableCPtr>(regex);
			// because dealing with the ILLkNonterminal/ILLkFirstable diamond was too much for my gentle brain
			// and I have to hack this hierarchy ever sice
			if (regexAsFirstable == nullptr) {
				throw Exception("Internal exception -- the regex passed to what is essentially a LR(k) firster (LRABuilder::computeItemLookahead) was ILLkFirstableCPtr despite one's best attempts not to judge the regex by its pointer ");
				// TODO: change type to InternalException or something of the kind
			}

			return InformedLookaheadDescriptor(regexAsFirstable);
		},
		[&symbolPrecededByDotIt]() {
			return ++symbolPrecededByDotIt;
		},
		[&symbolPrecededByDotIt, &endOfProductionIt]() {
			return symbolPrecededByDotIt == endOfProductionIt;
		},
		parentLookahead
	);
}

std::list<SymbolGroupPtrVector> LRABuilder::computeItemLookaheads(std::function<LRABuilder::InformedLookaheadDescriptor()> current, std::function<void()> advance, std::function<bool()> isAtEnd, SymbolGroupPtrVector parentLookahead) const {
	// this entire thing could be done very nicely as a recursion...
	// shame I did not notice until after the looping code has been written

	// setup
	const auto lookaheadSize = parentLookahead.size();
	SymbolGroupPtrVector capacitySettingEpsilonRootVector;
	capacitySettingEpsilonRootVector.reserve(lookaheadSize);
	std::list<SymbolGroupPtrVector> currentlyPossibleLookaheads({ capacitySettingEpsilonRootVector });
	std::list<SymbolGroupPtrVector> completePossibleLookaheads;

	// computation
	LLkFirster firster(&m_contextMachine);
	for (InformedLookaheadDescriptor currentDescriptor = current(); !isAtEnd(); advance()) {
		ILLkFirstableCPtr symbolAfterDot = currentDescriptor.symbolAfterDot;
		std::list<SymbolGroupPtrVector> newPossibleLookaheads;

		for (const SymbolGroupPtrVector& currentlyPossibleLookahead : currentlyPossibleLookaheads) {
			if (currentDescriptor.maySkipToEnd) {
				const SymbolGroupPtrVector lookaheadToAdd = this->truncatedConcat(currentlyPossibleLookahead, parentLookahead);
				completePossibleLookaheads.push_back(lookaheadToAdd);
			}

			SymbolGroupList possibleContinuations = symbolAfterDot->first(&firster, currentlyPossibleLookahead.toSymbolGroupList());
			auto& targetListForLongerLookaheads =
				currentlyPossibleLookahead.size() + 1 == lookaheadSize ? completePossibleLookaheads : newPossibleLookaheads;
			const auto toAdd = this->cross(currentlyPossibleLookahead, possibleContinuations.allButEmpty());
			if (possibleContinuations.containsEmpty()) {
				newPossibleLookaheads.push_back(currentlyPossibleLookahead);
			}
			targetListForLongerLookaheads.insert(targetListForLongerLookaheads.end(), toAdd.cbegin(), toAdd.cend());
		}

		if (newPossibleLookaheads.empty()) {
			break;
		}
		currentlyPossibleLookaheads = newPossibleLookaheads;
	}

	// if not firstable from the current context, fill the rest from the parent lookahead
	if (!currentlyPossibleLookaheads.empty()) {
		for (const SymbolGroupPtrVector& currentlyPossibleLookahead : currentlyPossibleLookaheads) {
			const SymbolGroupPtrVector lookaheadToAdd = this->truncatedConcat(currentlyPossibleLookahead, parentLookahead);
			completePossibleLookaheads.push_back(lookaheadToAdd);
		}
	}

	// at this point completePossibleLookaheads is guaranteed to be the complete list of possible lookaheads
	return completePossibleLookaheads;
}

std::list<SymbolGroupPtrVector> LRABuilder::cross(const SymbolGroupPtrVector& initialString, const SymbolGroupList& listOfPossibleUnitContinuations) const {
	std::list<SymbolGroupPtrVector> ret;

	for (const auto& symbolGroupPtr : listOfPossibleUnitContinuations) {
		SymbolGroupPtrVector initialStringCopy = initialString;
		initialStringCopy.push_back(symbolGroupPtr);
		ret.push_back(initialStringCopy);
	}

	return ret;
}

SymbolGroupPtrVector LRABuilder::truncatedConcat(const SymbolGroupPtrVector& initialString, const SymbolGroupPtrVector& continuation) const {
	SymbolGroupPtrVector ret(initialString);
	const auto amountToAdd = continuation.size() - initialString.size();

	auto copySrcEndIt = continuation.cbegin();
	std::advance(copySrcEndIt, amountToAdd);

	ret.insert(ret.end(), continuation.cbegin(), copySrcEndIt);
	return ret;
}
