#include <fstream>

#include "Output/Test05.h"

int main() {
	TextFileStream tfs("input.txt");

	Test05::Test05 tokenizer;
	auto token = tokenizer.apply(tfs);

	return 0;
}