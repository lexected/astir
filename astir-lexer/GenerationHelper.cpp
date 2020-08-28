#include "GenerationHelper.h"

#include "GenerationException.h"

void GenerationHelper::macroWrite(const std::string& sourceString, const std::map<std::string, std::string>& macroPairs, std::ostream& output) {
	size_t it = 0;
	unsigned long lastIndentation = 0;
	size_t sourceStringLength = sourceString.length();
	while (it < sourceStringLength) {
		auto startOfMacro = sourceString.find("${{", it);
		if (startOfMacro == std::string::npos) {
			break;
		}

		size_t endOfMacro = sourceString.find("}}", startOfMacro);
		if (endOfMacro == std::string::npos) {
			throw GenerationException("Invalid specimen file supplied, a macro start '${{' without the matching '}}' encountered");
		}

		const std::string macroName = sourceString.substr(startOfMacro+3, endOfMacro-(startOfMacro + 3));
		auto macroPairIt = macroPairs.find(macroName);
		if (macroPairIt == macroPairs.end()) {
			throw GenerationException("Invalid specimen file supplied, unrecognized macro '" + macroName + "' encountered");
		}

		// identify the preceeding text
		const char* precedingTextStart = sourceString.c_str() + it;
		size_t precedingTextCount = startOfMacro - it;

		// copy the preceeding text unchanged
		output.write(precedingTextStart, precedingTextCount);
		
		// scan the preceeding text for relevant indentation offset to be used for the text inserted
		if (precedingTextCount > 0) {
			const char* precedingTextLastCharacter = precedingTextStart + precedingTextCount - 1;
			unsigned long currentIndentation = 0;
			while (precedingTextLastCharacter >= precedingTextStart) {
				if (*precedingTextLastCharacter == '\t') {
					++currentIndentation;
				} else if (*precedingTextLastCharacter == '\n') {
					break;
				} else {
					currentIndentation = 0;
				}

				--precedingTextLastCharacter;
			}
			if (precedingTextLastCharacter > precedingTextStart) {
				// i.e. a newline was encountered between the last macro occurence and the new one
				lastIndentation = currentIndentation;
			} else {
				// i.e. no newline was encountered between the last macro occurence and the new one
				// just use lastIndentation
			}
		}

		const std::string& macroReplacementText = macroPairIt->second;
		if (lastIndentation > 0) {
			const std::string indentationString(lastIndentation, '\t');
			size_t newTextLength = macroReplacementText.length();

			size_t theCharacterAfterLastNewlineOffset = 0;
			while (theCharacterAfterLastNewlineOffset < newTextLength) {
				size_t offsetFromCurrentPos = macroReplacementText.find('\n', theCharacterAfterLastNewlineOffset);
				if (offsetFromCurrentPos == std::string::npos) {
					output.write(macroReplacementText.c_str() + theCharacterAfterLastNewlineOffset, newTextLength - theCharacterAfterLastNewlineOffset);
					break;
				}
				++offsetFromCurrentPos;

				output.write(macroReplacementText.c_str() + theCharacterAfterLastNewlineOffset, offsetFromCurrentPos - theCharacterAfterLastNewlineOffset);
				output << indentationString;
				theCharacterAfterLastNewlineOffset = offsetFromCurrentPos;
			}
		} else {
			output << macroReplacementText;
		}
		
		it = endOfMacro + 2;
	}

	output.write(sourceString.c_str() + it, sourceString.length()-it);
	output.flush();
}
