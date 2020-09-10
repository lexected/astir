#include <fstream>

#include "Output/BinaryRecognizer.h"

int main() {
	std::ifstream f("input.txt");

	TextFileStream tfs("input.txt", f);

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