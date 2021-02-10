// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "output_utils.h"


void to_file(
        const std::vector<std::pair<std::string, int>>& vocab,
        const std::string& filename
) {
    std::ofstream out_a_file;
    out_a_file.open (filename);

    for (auto& p: vocab) {
        out_a_file << p.first << "\t\t" << p.second << std::endl;
    }
    out_a_file.close();
}
