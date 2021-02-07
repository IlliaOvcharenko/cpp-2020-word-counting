#ifndef CPP_2020_WORD_COUNTING_OUTPUT_UTILS_H
#define CPP_2020_WORD_COUNTING_OUTPUT_UTILS_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>

void to_file(
        const std::vector<std::pair<std::string, int>>& vocab,
        const std::string& filename
);

#endif //CPP_2020_WORD_COUNTING_OUTPUT_UTILS_H
