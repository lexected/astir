#include <fstream>

#include "Output/TreeTokenizer.h"
#include "Output/TreeParser.h"

int main() {
	std::ifstream f("input.txt");
	TextFileStream tfs("input.txt", f);

	TreeTokenizer::TreeTokenizer treeTokenizer;
	auto primaryStreamProcessed = treeTokenizer.processStream(tfs);

	ListProductionStream<TreeTokenizer::OutputTerminal> lcs(primaryStreamProcessed);

	TreeParser::TreeParser treeParser;
	auto secondaryStreamProcessed = treeParser.apply(lcs);

	return 0;
}