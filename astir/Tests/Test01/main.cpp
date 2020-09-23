#include "Output/Test01.h"

int main() {
	TextFileStream input("input.txt");

	Test01::Test01 tokenizer;
	auto tokenized = tokenizer.processStream(input);

	return 0;
}