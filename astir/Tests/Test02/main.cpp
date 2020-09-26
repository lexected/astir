#include "Output/Test02.h"

int main() {
    TextFileStream tfs("input.txt");

    Test02::Test02* tokenizer = new Test02::Test02();
    auto tokens = tokenizer->processStream(tfs);
    delete tokenizer;
    
    return 0;
}