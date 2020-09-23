#include "Output/TreeTokenizer.h"
#include "Output/TreeParser.h"

int main() {
	TextFileStream tfs("input.txt");

	TreeParser::TreeParser treeParser;
	auto secondaryStreamProcessed = treeParser.parse(tfs);

	return 0;
}