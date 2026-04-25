#include "braille_cell.h"
#include "braille_dictionary.h"

BrailleCell::BrailleCell() {
    letterPattern = {{0,0},{0,0},{0,0}};
}

BrailleCell::BrailleCell(char letter) {
    setLetter(letter);
}

void BrailleCell::setLetter(char c) {
    letter = c;
    const int* p = getBraillePattern(c);
    if (p) {
        letterPattern = {{p[0], p[1]}, {p[2], p[3]}, {p[4], p[5]}};
    } else {
        letterPattern = {{0,0}, {0,0}, {0,0}};
    }
}

bool BrailleCell::isUpperCase() {
    return ::isUpper(letter);
}

char BrailleCell::getLetter() {
    return letter;
}

std::vector<std::vector<int>> BrailleCell::getPattern() const {
    return letterPattern;
}

int BrailleCell::getCase(const int angles[], char side, std::vector<std::vector<int>> pattern) {
    int col = (side == 'L') ? 0 : 1;
    int index = (pattern[0][col] << 2) | (pattern[1][col] << 1) | pattern[2][col];
    return angles[index];
}
