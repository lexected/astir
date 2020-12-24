#pragma once

#include <memory>

class AFACondition {
public:
	virtual bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const = 0;

protected:
	AFACondition() = default;
};

class EmptyAFACondition : public virtual AFACondition {
public:
	EmptyAFACondition() = default;

	bool equals(const std::shared_ptr<AFACondition>& anotherCondition) const override;
};