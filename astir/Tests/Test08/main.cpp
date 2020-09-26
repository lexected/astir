#include <fstream>

#include "Output/TreeParser.h"

int main() {
	TextFileStream tfs("input.txt");

	TreeParser::TreeParser treeParser;
	auto primaryStreamProcessed = treeParser.apply(tfs);

	return 0;
}