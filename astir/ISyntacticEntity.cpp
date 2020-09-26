#include "ISyntacticEntity.h"

const FileLocation& ISyntacticEntity::location() const {
	return m_location;
}

void ISyntacticEntity::copyLocation(const IFileLocalizable& anotherLocalizableThingy) {
	m_location = anotherLocalizableThingy.location();
}
