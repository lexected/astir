#include <fstream>

#include "Output/Test04.h"

int main() {
	TextFileStream tfs("input.txt");

	Test04::Test04 tokenizer;
	auto token = tokenizer.apply(tfs);

	return 0;
}