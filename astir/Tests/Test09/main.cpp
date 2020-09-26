#include <fstream>

#include "Output/TreeTokenizer.h"
#include "Output/TreeParser.h"

int main() {
	TextFileStream tfs("input.txt");

	TreeTokenizer::TreeTokenizer treeTokenizer;
	auto primaryStreamProcessed = treeTokenizer.processStreamWithIgnorance(tfs);

	ListProductionStream<TreeTokenizer::OutputTerminal> lcs(primaryStreamProcessed);

	TreeParser::TreeParser treeParser;
	auto secondaryStreamProcessed = treeParser.apply(lcs);

	return 0;
}