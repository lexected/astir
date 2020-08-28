#include "Field.h"

#include "GenerationVisitor.h"

bool FlagField::flaggable() const {
	return true;
}

bool FlagField::settable() const {
	return false;
}

bool FlagField::listable() const {
	return false;
}

void FlagField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

bool RawField::flaggable() const {
	return false;
}

bool RawField::settable() const {
	return true;
}

bool RawField::listable() const {
	return true;
}

void RawField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

bool ItemField::flaggable() const {
	return false;
}

bool ItemField::settable() const {
	return true;
}

bool ItemField::listable() const {
	return false;
}

void ItemField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}

bool ListField::flaggable() const {
	return false;
}

bool ListField::settable() const {
	return false;
}

bool ListField::listable() const {
	return true;
}

void ListField::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}
