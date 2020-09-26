#include "Output/TreeTokenizer.h"
#include "Output/TreeParser.h"

int main() {
	TextFileStream tfs("input.txt");

	TreeTokenizer::TreeTokenizer treeTokenizer;
	auto primaryStreamProcessed = treeTokenizer.processStreamWithIgnorance(tfs);

	ListProductionStream<TreeTokenizer::OutputProduction> lps(primaryStreamProcessed);

	TreeParser::TreeParser treeParser;
	auto secondaryStreamProcessed = treeParser.parse(lps);

	return 0;
}