#include "Output/PrimaryAutomaton.h"
#include "Output/SecondaryAutomaton.h"

int main() {
	TextFileStream tfs("input.txt");

	PrimaryAutomaton::PrimaryAutomaton primaryAutomaton;
	auto primaryStreamProcessed = primaryAutomaton.processStream(tfs);

	ListProductionStream<PrimaryAutomaton::OutputProduction> lps(primaryStreamProcessed);

	SecondaryAutomaton::SecondaryAutomaton secondaryAutomaton;
	auto secondaryStreamProcessed = secondaryAutomaton.processStreamWithIgnorance(lps);

	return 0;
}