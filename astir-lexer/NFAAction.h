#pragma once

#include <string>
#include <list>

#include "IGenerationVisitable.h"

enum class NFAActionType : unsigned char {
	Set = 1,
	Unset = 2,
	Flag = 3,
	Unflag = 4,
	Append = 5,
	Prepend = 6,
	Clear = 7,
	LeftTrim = 8,
	RightTrim = 9,

	CreateContext = 101,
	DestroyContext = 102, // UNSAFE IN VAST MAJORITY OF SCENARIOS DUE TO NATURE OF NFA EMPTY TRANSITIONS (and because my context encapsulation is far from perfect), DO NOT USE!
	ElevateContext = 103,
	SetContext = 104,
	AppendContext = 105,
	PrependContext = 106,

	None = 255
};

struct NFAAction : public IGenerationVisitable {
	NFAActionType type;
	std::string contextPath;
	std::string targetName;

	NFAAction(NFAActionType faAction, const std::string& contextPath, const std::string& targetName)
		: type(faAction), contextPath(contextPath), targetName(targetName) { }

	void accept(GenerationVisitor* visitor) const override;
};

class NFAActionRegister : public std::list<NFAAction>, public IGenerationVisitable {
public:
	NFAActionRegister() = default;

	NFAActionRegister operator+(const NFAActionRegister& rhs) const;
	const NFAActionRegister& operator+=(const NFAActionRegister& rhs);

	void accept(GenerationVisitor* visitor) const override;
};