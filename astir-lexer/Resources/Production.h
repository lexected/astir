#pragma once

#include <memory>

#include "Location.h"

class ILocalizable {
public:
	virtual const std::shared_ptr<Location>& location() const = 0;
	std::string locationString() const { return location()->toString(); }

protected:
	ILocalizable() = default;
	virtual ~ILocalizable() = default;
};

class Production : public ILocalizable {
public:
	Production(const std::shared_ptr<Location>& occurenceLocation)
		: m_location(occurenceLocation) { }
	Production(const ILocalizable& underlyingEntity)
		: m_location(underlyingEntity.location()) { }

	const std::shared_ptr<Location>& location() const override;
private:
	std::shared_ptr<Location> m_location;
};