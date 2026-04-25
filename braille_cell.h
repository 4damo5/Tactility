#ifndef BRAILLE_CELL_H
#define BRAILLE_CELL_H

#include <vector>
#include "braille_dictionary.h"

class BrailleCell {
    private:
        std::vector<std::vector<int>> letterPattern;
        char letter;

    public:
        BrailleCell();
        BrailleCell(char letter);


        char getLetter();
        bool isUpperCase();
        void setLetter(char letter);
        std::vector<std::vector<int>> getPattern() const;
        int getCase(const int angles[], char side, std::vector<std::vector<int>> pattern);
};

#endif