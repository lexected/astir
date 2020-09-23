#pragma once

#include "Field.h"
#include "ISemanticEntity.h"

#include <string>
#include <memory>

enum class RegexActionType : unsigned char {
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

	None = 255
};

struct RegexAction : public ISyntacticEntity {
	RegexActionType type = RegexActionType::None;
	std::string target;
	std::shared_ptr<Field> targetField;

	RegexAction()
		: targetField(nullptr) { }
	RegexAction(RegexActionType type, std::string target)
		: type(type), target(target), targetField(nullptr) { }
};