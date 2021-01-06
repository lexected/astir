#include "LRABuilder.h"

void LRABuilder::visit(const CategoryStatement* category, LRA* lra, AFAState attachmentState, std::shared_ptr<WeakContext> context) const {
	LRTag categoryTag;
	categoryTag.statement = category;
	categoryTag.context = context;

	auto tagIt = lra->tags.find(categoryTag);
	if (tagIt != lra->tags.end()) {
		// throw Exception("Tag for the category " + category->name + " already exists in the current LRA");
		lra->addEmptyTransition(attachmentState, tagIt->second);
	} else {
		AFAState categoryRootState = lra->addState();
		lra->addEmptyTransition(attachmentState, categoryRootState);

		lra->tagState(categoryRootState, categoryTag);

		std::shared_ptr<WeakContext> contextOfThisCategory = std::make_shared<WeakContext>(context);
		for (const auto& categoryReferencePair : category->references) {
			AFAState referenceStatementStateAfterShift = lra->addState();
			lra->addTransition(categoryRootState, referenceStatementStateAfterShift, std::make_shared<StatementSymbolGroup>(categoryReferencePair.second.statement, &m_contextMachine));

			categoryReferencePair.second.statement->visit(lra, categoryRootState, contextOfThisCategory);
		}
	}
}
