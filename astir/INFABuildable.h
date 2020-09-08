#pragma once

class NFA;
class NFABuilder;

class INFABuildable {
public:
	virtual NFA accept(const NFABuilder& nfaBuilder) const = 0;

	virtual ~INFABuildable() = default;
protected:
	INFABuildable() = default;
};