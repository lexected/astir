#include <fstream>

#include "Output/BinaryTokenizer.h"

int main() {
	TextFileStream tfs("input.txt");

	BinaryTokenizer::BinaryTokenizer tokenizer;
	auto listOfTokens = tokenizer.processStream(tfs);

	for(const auto& tokenPtr : listOfTokens) {
        if(tokenPtr->type == BinaryTokenizerTerminalType::BinaryString) {
            std::cout << "[" << tokenPtr->locationString() << "] BinaryString: " << tokenPtr->raw << std::endl;
        } else {
			std::cout << "[" << tokenPtr->locationString() << "] WhiteSpace of length " << tokenPtr->raw.length() << std::endl;
		}
    }

	return 0;
}