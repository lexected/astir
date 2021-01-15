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

void LRABuilder::visit(const ConjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	auto it = regex->conjunction.cbegin();
	const auto endIt = regex->conjunction.cend();

	AFAState attachmentState = startingState;
	auto nextIt = ++it;
	for (; it != endIt;) {
		SymbolGroupPtrVector lookaheadForThisItem = this->computeItemLookahead(nextIt, endIt, lookahead);
		(*it)->accept(lra, attachmentState, lookaheadForThisItem);

		AFAState newAttachmentState = lra->addState();
		for (AFAState endPointState : lra->finalStates) {
			lra->addEmptyTransition(endPointState, newAttachmentState);
		}
		lra->finalStates.clear();
		attachmentState = newAttachmentState;

		it = nextIt;
		++nextIt;
	}
}

std::list<SymbolGroupPtrVector> LRABuilder::computeItemLookahead(std::list<std::unique_ptr<RootRegex>>::const_iterator symbolPrecededByDotIt, std::list<std::unique_ptr<RootRegex>>::const_iterator endOfProductionIt, SymbolGroupPtrVector parentLookahead) const {
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
	for (; symbolPrecededByDotIt != endOfProductionIt; ++symbolPrecededByDotIt) {
		std::list<SymbolGroupPtrVector> newPossibleLookaheads;

		for(const SymbolGroupPtrVector& currentlyPossibleLookahead : currentlyPossibleLookaheads) {
			const RootRegex* regex = symbolPrecededByDotIt->get();
			ILLkFirstableCPtr regexAsFirstable = dynamic_cast<ILLkFirstableCPtr>(regex);
			// because dealing with the ILLkNonterminal/ILLkFirstable diamond was too much for my gentle brain
			// and I have to hack this hierarchy ever sice
			if (regexAsFirstable == nullptr) {
				throw Exception("Internal exception -- the regex passed to what is essentially a LR(k) firster (LRABuilder::computeItemLookahead) was ILLkFirstableCPtr despite one's best attempts not to judge the regex by its pointer ");
				// TODO: change type to InternalException or something of the kind
			}

			SymbolGroupList possibleContinuations = regexAsFirstable->first(&firster, currentlyPossibleLookahead.toSymbolGroupList());
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
			completePossibleLookaheads.insert(completePossibleLookaheads.end(), lookaheadToAdd);
		}
	}

	// at this point completePossibleLookaheads is guaranteed to be the complete list of possible lookaheads
	return completePossibleLookaheads;
}
