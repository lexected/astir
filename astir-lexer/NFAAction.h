#pragma once

#include <string>
#include <list>

enum class NFAActionType : unsigned char {
	Flag = 1,
	Unflag = 2,

	Capture = 3,
	Empty = 4,
	Append = 5,
	Prepend = 6,

	Set = 7,
	Unset = 8,
	Push = 9,
	Pop = 10,
	Clear = 11,

	CreateContext = 101,
	TerminalizeContext = 102,
	ElevateContext = 103,
	InitiateCapture = 104,

	None = 255
};


struct NFAAction {
	NFAActionType type;
	std::string contextPath;
	std::string targetName;
	std::string payload;

	NFAAction(NFAActionType faAction, const std::string& contextPath, const std::string& targetName)
		: type(faAction), contextPath(contextPath), targetName(targetName), payload() { }
	NFAAction(NFAActionType faAction, const std::string& contextPath, const std::string& targetName, const std::string& payload)
		: type(faAction), contextPath(contextPath), targetName(targetName), payload(payload) { }

	bool operator==(const NFAAction& rhs) const;
};

class NFAActionRegister : public std::list<NFAAction> {
public:
	NFAActionRegister() = default;

	NFAActionRegister operator+(const NFAActionRegister& rhs) const;
	const NFAActionRegister& operator+=(const NFAActionRegister& rhs);
	bool operator==(const NFAActionRegister& rhs) const;
};