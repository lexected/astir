#include <fstream>

#include "Output/TreeParser.h"

int main() {
	std::ifstream f("input.txt");
	TextFileStream tfs("input.txt", f);

	TreeParser::TreeParser treeParser;
	auto primaryStreamProcessed = treeParser.apply(tfs);

	return 0;
}