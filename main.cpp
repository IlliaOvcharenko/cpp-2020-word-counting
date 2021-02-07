#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>

#include "src/thread_safe_queue.h"
#include "src/time_measurement.h"
#include "src/logger.h"
#include "src/stages.h"
#include "src/output_utils.h"

int MAX_ITEMS = 100;
bool Logger::VERBOSE = true;


int main() {
    auto start_time = get_current_time_fenced();
    std::string data_folder = "../data";

    ThreadSafeQueue<std::string> filename_queue(50);
    std::thread get_filenames_thread(get_filenames, std::ref(data_folder), std::ref(filename_queue));

    ThreadSafeQueue<std::string> file_content_queue(50);
    std::thread read_files_thread(read_files, std::ref(filename_queue), std::ref(file_content_queue));

    init_locale();
    ThreadSafeQueue<std::future<vocabulary_type>> vocabulary_queue(50);
    int n_count_words_threads = 4;
    std::mutex merge_mutex;
    std::condition_variable pop_during_merge_cv;
    std::condition_variable push_during_merge_cv;
    std::vector<std::thread> count_words_threads;
    for (int i = 0; i < n_count_words_threads; ++i) {
        count_words_threads.push_back(std::thread(
                count_words,
                std::ref(file_content_queue),
                std::ref(vocabulary_queue),
                std::ref(merge_mutex),
                std::ref(pop_during_merge_cv),
                std::ref(push_during_merge_cv)
        ));
    }

    int n_merge_threads = 2;
    std::vector<std::thread> merge_threads;
    for (int i = 0; i < n_merge_threads; ++i) {
        merge_threads.push_back(std::thread(
                merge_vocabularies,
                std::ref(vocabulary_queue),
                std::ref(merge_mutex),
                std::ref(pop_during_merge_cv),
                std::ref(push_during_merge_cv)
        ));
    }


    get_filenames_thread.join();
    filename_queue.set_finished();

    read_files_thread.join();
    file_content_queue.set_finished();

    for (auto& th: count_words_threads) {
        th.join();
    }
    vocabulary_queue.set_finished();
    pop_during_merge_cv.notify_all();

    for (auto& th: merge_threads) {
        th.join();
    }

    auto vocab = vocabulary_queue.pop().get();
    std::cout << "vocab size: " << vocab.size() << std::endl;

    auto finish_time = get_current_time_fenced();
    auto total_time = finish_time - start_time;
    std::cout << "total time: " << to_sec(total_time) << std::endl;


    std::vector<std::pair<std::string, int>> vocab_vec;
    for (auto& p: vocab) {
        vocab_vec.push_back(std::move(p));
    }
    to_file(vocab_vec, "res_a.txt");

    std::sort(
            vocab_vec.begin(), vocab_vec.end(),
            [](auto& a, auto& b) -> bool {
                return a.second > b.second;
            }
    );
    to_file(vocab_vec, "res_n.txt");

    return 0;
}
