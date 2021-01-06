#pragma once

#include <memory>
#include <list>

#include "ILLkFirstable.h"

struct WeakContext;
struct WeakContext {
	std::shared_ptr<WeakContext> parent;
	std::list<ILLkFirstableCPtr> followedBy;

	WeakContext(std::shared_ptr<WeakContext> parent);
	WeakContext(std::shared_ptr<WeakContext> parent, std::list<ILLkFirstableCPtr> followedBy);

	bool equals(const WeakContext& rhs) const;
	bool operator==(const WeakContext& rhs) const;
protected:
	WeakContext() = default;
};

struct EndOfStreamContext : public WeakContext {
public:
	EndOfStreamContext() = default;
};