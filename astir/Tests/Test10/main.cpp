#include <fstream>

#include "Output/TreeTokenizer.h"
#include "Output/TreeParser.h"

int main() {
	std::ifstream f("input.txt");
	TextFileStream tfs("input.txt", f);

	TreeParser::TreeParser treeParser;
	auto secondaryStreamProcessed = treeParser.parse(tfs);

	return 0;
}