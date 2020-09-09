#include <fstream>

#include "Output/PrimaryAutomaton.h"
#include "Output/SecondaryAutomaton.h"

int main() {
	std::ifstream f("input.txt");

	TextFileStream tfs("input.txt", f);

	PrimaryAutomaton::PrimaryAutomaton primaryTokenizer;
	auto primaryStreamProcessed = primaryTokenizer.process(tfs);

	ListProductionStream<PrimaryAutomaton::OutputTerminal> lps(primaryStreamProcessed);

	SecondaryAutomaton::SecondaryAutomaton secondaryTokenizer;
	auto secondaryStreamProcessed = secondaryTokenizer.process(lps);

	return 0;
}