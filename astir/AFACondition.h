#pragma once

#include <memory>

class AFACondition {
public:
	virtual bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const;

	virtual bool isEmpty() const;

protected:
	AFACondition() = default;
};