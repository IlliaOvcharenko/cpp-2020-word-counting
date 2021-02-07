#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <algorithm>

#include <yaml-cpp/yaml.h>

#include "src/thread_safe_queue.h"
#include "src/time_measurement.h"
#include "src/logger.h"
#include "src/stages.h"
#include "src/output_utils.h"

bool Logger::VERBOSE;
int MAX_ITEMS;

int main(int argc, char * argv[]) {

    // Read config
    // std::string config_filename = std::string(argv[1]);
    std::string config_filename = "../configs/base.yaml";
    YAML::Node config = YAML::LoadFile(config_filename);
    auto data_folder = config["data_folder"].as<std::string>();
    auto output_by_a_folder = config["output_by_a_folder"].as<std::string>();
    auto output_by_n_folder = config["output_by_n_folder"].as<std::string>();

    auto filename_queue_bound = config["filename_queue_bound"].as<int>();
    auto file_content_queue_bound = config["file_content_queue_bound"].as<int>();
    auto vocabulary_queue_bound = config["vocabulary_queue_bound"].as<int>();

    auto n_count_words_threads = config["count_words_threads"].as<int>();
    auto n_merge_threads = config["merge_threads"].as<int>();

    Logger::VERBOSE = config["verbose"].as<bool>();
    MAX_ITEMS = config["item_limit"].as<int>();


    auto start_time = get_current_time_fenced();
    // Run threads
    ThreadSafeQueue<std::string> filename_queue(filename_queue_bound);
    std::thread get_filenames_thread(
            get_filenames,
            std::ref(data_folder),
            std::ref(filename_queue)
    );

    ThreadSafeQueue<std::string> file_content_queue(file_content_queue_bound);
    std::thread read_files_thread(
            read_files,
            std::ref(filename_queue),
            std::ref(file_content_queue)
    );

    init_locale();
    ThreadSafeQueue<std::future<vocabulary_type>> vocabulary_queue(vocabulary_queue_bound);
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

    // Join threads
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

    // Get result
    auto vocab = vocabulary_queue.pop().get();
    std::cout << "vocab size: " << vocab.size() << std::endl;

    auto finish_time = get_current_time_fenced();
    auto total_time = finish_time - start_time;
    std::cout << "total time: " << to_sec(total_time) << std::endl;

    // Write to file
    std::vector<std::pair<std::string, int>> vocab_vec;
    for (auto& p: vocab) {
        vocab_vec.push_back(std::move(p));
    }
    to_file(vocab_vec, output_by_a_folder);

    std::sort(
            vocab_vec.begin(), vocab_vec.end(),
            [](auto& a, auto& b) -> bool {
                return a.second > b.second;
            }
    );
    to_file(vocab_vec, output_by_n_folder);

    return 0;
}
