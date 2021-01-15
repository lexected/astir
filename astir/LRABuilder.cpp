#include "LRABuilder.h"

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
	rule->regex->visit(lra, startingState, lookahead);
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

		rule->regex->visit(lra, productionRootState, lookahead);
	}
}

void LRABuilder::visit(const RegexStatement* rule, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	rule->regex->visit(lra, startingState, lookahead);
}

void LRABuilder::visit(const DisjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	for (const auto& conjunctiveRegexPtr : regex->disjunction) {
		AFAState conjunctiveRegexStateAfterShift = lra->addState();
		lra->addEmptyTransition(startingState, conjunctiveRegexStateAfterShift);

		conjunctiveRegexPtr->visit(lra, conjunctiveRegexStateAfterShift, lookahead);
	}
}

void LRABuilder::visit(const ConjunctiveRegex* regex, LRA* lra, AFAState startingState, SymbolGroupPtrVector lookahead) const {
	auto it = regex->conjunction.cbegin();
	const auto endIt = regex->conjunction.cend();

	AFAState attachmentState = startingState;
	auto nextIt = ++it;
	for (; it != endIt;) {
		SymbolGroupPtrVector lookaheadForThisItem = this->computeItemLookahead(nextIt, endIt, lookahead);
		(*it)->visit(lra, attachmentState, lookaheadForThisItem);

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

SymbolGroupPtrVector LRABuilder::computeItemLookahead(std::list<std::unique_ptr<RootRegex>>::const_iterator symbolPrecededByDot, std::list<std::unique_ptr<RootRegex>>::const_iterator endOfProduction, SymbolGroupPtrVector parentLookahead) const {
	return SymbolGroupPtrVector();
}
