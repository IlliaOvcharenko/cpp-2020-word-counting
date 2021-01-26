#ifndef CPP_2020_WORD_COUNTING_TEXT_UTILS_H
#define CPP_2020_WORD_COUNTING_TEXT_UTILS_H

#include <string>
#include <map>

#include <boost/locale.hpp>
#include <boost/version.hpp>


namespace lc = boost::locale;

typedef std::map<std::string, int> vocabulary_type;

// TODO does it really move string value instead of copy?
void text_to_vocabulary(std::string&& str, vocabulary_type& vocab);


void init_locale();
#endif //CPP_2020_WORD_COUNTING_TEXT_UTILS_H
