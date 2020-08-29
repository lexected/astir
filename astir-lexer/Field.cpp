#include "Field.h"

#include "GenerationVisitor.h"

void FlagField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

void RawField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

void ItemField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

void ListField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}
