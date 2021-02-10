#ifndef CPP_2020_WORD_COUNTING_STAGES_H
#define CPP_2020_WORD_COUNTING_STAGES_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <future>
#include <filesystem>

#include "archive_wrapper.h"
#include "text_utils.h"
#include "thread_safe_queue.h"
#include "logger.h"


namespace fs = std::filesystem;

const size_t MAX_ARCHIVE_SIZE = 10000000;
extern size_t MAX_ITEMS;


void get_filenames(
        const std::string& data_folder,
        ThreadSafeQueue<std::string>& filename_queue
);

void read_files(
        ThreadSafeQueue<std::string>& filename_queue,
        ThreadSafeQueue<std::string>& file_content_queue
);

void count_words(
        ThreadSafeQueue<std::string>& file_content_queue,
        ThreadSafeQueue<std::future<vocabulary_type>>& vocabulary_queue,
        std::mutex& merge_mutex,
        std::condition_variable& pop_during_merge_cv,
        std::condition_variable& push_during_merge_cv
);

void merge_vocabularies(
        ThreadSafeQueue<std::future<vocabulary_type>>& vocabulary_queue,
        std::mutex& merge_mutex,
        std::condition_variable& pop_during_merge_cv,
        std::condition_variable& push_during_merge_cv
);

#endif //CPP_2020_WORD_COUNTING_STAGES_H
