#include "Output/Test03.h"

int main() {
    TextFileStream tfs("input.txt");

    Test03::Test03* tokenizer = new Test03::Test03();
    auto tokens = tokenizer->processStream(tfs);
    delete tokenizer;
    
    return 0;
}