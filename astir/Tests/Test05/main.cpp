#include <fstream>

#include "Test05.h"

int main() {
	std::ifstream f("input.txt");

	TextFileStream tfs("input.txt", f);

	Test05::Test05 tokenizer;
	tokenizer.apply(tfs);

	return 0;
}