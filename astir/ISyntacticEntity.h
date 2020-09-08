#pragma once

#include "IFileLocalizable.h"

class ISyntacticEntity : public IFileLocalizable {
public:
	const FileLocation& location() const override;

	void copyLocation(const IFileLocalizable& anotherLocalizableThingy);
protected:
	ISyntacticEntity() = default;
	virtual ~ISyntacticEntity() = default;

private:
	FileLocation m_location;
};