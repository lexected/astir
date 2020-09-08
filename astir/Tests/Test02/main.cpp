#include "Test02.h"

#include <iostream>
#include <fstream>

int main() {
    std::ifstream exampleInput("../exampleInput.txt");
    TextFileStream tfs("../exampleInput.txt", exampleInput);

    Test02::Test02* tokenizer = new Test02::Test02();
    auto tokens = tokenizer->process(tfs);
    delete tokenizer;
    
    return 0;
}