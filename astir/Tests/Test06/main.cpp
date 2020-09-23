#include <fstream>

#include "Output/PrimaryAutomaton.h"
#include "Output/SecondaryAutomaton.h"

int main() {
	TextFileStream tfs("input.txt");

	PrimaryAutomaton::PrimaryAutomaton primaryTokenizer;
	auto primaryStreamProcessed = primaryTokenizer.processStream(tfs);

	ListProductionStream<PrimaryAutomaton::OutputTerminal> lps(primaryStreamProcessed);

	SecondaryAutomaton::SecondaryAutomaton secondaryTokenizer;
	auto secondaryStreamProcessed = secondaryTokenizer.processStream(lps);

	return 0;
}