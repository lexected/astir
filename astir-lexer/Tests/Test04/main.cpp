#include <fstream>

#include "Test04.h"

int main() {
	std::ifstream f("input.txt");

	TextFileStream tfs("input.txt", f);

	Test04::Test04 tokenizer;
	tokenizer.apply(tfs);

	return 0;
}