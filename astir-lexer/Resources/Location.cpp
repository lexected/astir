#include "Location.h"

void TextLocation::note(char c) {
    ++column;
    if (c == '\n') {
        ++line;
        column = 0;
    }
}

void TextLocation::advance() {
    ++column;
}

std::string TextLocation::toString() const {
    return std::to_string(line) + ":" + std::to_string(column);
}

std::shared_ptr<Location> TextLocation::clone() const {
    return std::make_shared<TextLocation>(*this);
}

std::string TextFileLocation::toString() const {
    return fileName + ":" + this->TextLocation::toString();
}

std::shared_ptr<Location> TextFileLocation::clone() const {
    return std::make_shared<TextFileLocation>(*this);
}