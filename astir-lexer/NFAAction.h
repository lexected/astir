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
	AssignContext = 102,
	AppendContext = 103,
	PrependContext = 104,

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