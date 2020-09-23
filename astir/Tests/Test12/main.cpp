#include <fstream>

#include "Output/CategoricalParser.h"

int main() {
	TextFileStream tfs("input.txt");

	CategoricalParser::CategoricalParser categoricalParser;
	auto primaryStreamProcessed = categoricalParser.parse(tfs);

	return 0;
}