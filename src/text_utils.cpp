// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "text_utils.h"

void text_to_vocabulary(std::string&& str, vocabulary_type& vocab) {
    str = lc::normalize(str);
    str = lc::fold_case(str);

    lc::boundary::ssegment_index map(lc::boundary::word, str.begin(), str.end());
    map.rule(lc::boundary::word_letters);
    for(auto split_itr = map.begin(), e = map.end(); split_itr != e; ++split_itr) {
        auto vocab_itr = vocab.find(*split_itr);
        if (vocab_itr != vocab.end()) {
            vocab_itr->second += 1;
        } else {
            vocab.insert(std::make_pair(*split_itr, 1));
        }
    }
}

void init_locale() {
    lc::generator gen;
    std::locale::global(gen("en_US.UTF-8"));
}
