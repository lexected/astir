#include <fstream>

#include "Output/BinaryRecognizer.h"

int main() {
	TextFileStream tfs("input.txt");

	BinaryRecognizer::BinaryRecognizer tokenizer;
	tokenizer.apply(tfs);

	if(tokenizer.lastApplicationSuccessful()) {
		std::cout << "The input in 'input.txt' was recognized; it indeed belongs to our language" << std::endl;
		return 0;
	} else {
		std::cout << "The input in 'input.txt' was not recognized; it does not belong to our language" << std::endl;
		return 1;
	}
}