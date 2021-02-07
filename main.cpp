#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <future>
#include <experimental/filesystem>

#include "src/archive_wrapper.h"
#include "src/text_utils.h"
#include "src/thread_safe_queue.h"
#include "src/time_measurement.h"

namespace fs = std::experimental::filesystem;
const int MAX_ARCHIVE_SIZE = 10000000;
const int MAX_ITEMS = 10000;
// TODO make static
const bool VERBOSE = true;

class Logger {
    std::thread::id thread_id_m;
    std::string work_name_m;
public:
    Logger(std::thread::id thread_id, std::string work_name)
        : thread_id_m(thread_id)
        , work_name_m(work_name) { }

    void print(const std::string& msg) {
        if (!VERBOSE) return;
        std::cout << "[" << work_name_m << ": " << thread_id_m << "] " << msg << std::endl;
    }
};

void get_filenames(const std::string& data_folder, ThreadSafeQueue<std::string>& filename_queue) {
    Logger log(std::this_thread::get_id(), "index");

    int fn_counter = 0;
    for (const auto& entry: fs::recursive_directory_iterator(data_folder)) {
        if (entry.path().extension() == ".zip") {
            filename_queue.push(entry.path());

            std::stringstream ss;
            ss << "add filename: " << entry.path();
            log.print(ss.str());

            if ((MAX_ITEMS != -1) && (++fn_counter >= MAX_ITEMS)) {
                break;
            }

        }
    }
}

void read_files(
        ThreadSafeQueue<std::string>& filename_queue,
        ThreadSafeQueue<std::string>& file_content_queue
) {
    Logger log(std::this_thread::get_id(), "read");

    while (true) {
        try {
            std::string fn = filename_queue.pop();
            log.print("archive filename: " + fn);


            auto archive_reader = ArchiveReader(fn, MAX_ARCHIVE_SIZE);

            while (archive_reader.is_next_file()) {
                size_t entry_size = archive_reader.get_entry_size();

                if (entry_size > MAX_ARCHIVE_SIZE) {
                    log.print("archive entry is too large, skip it");
                    continue;
                }

                std::string file_content(entry_size, char{});
                archive_reader.read_next_file(&file_content[0], entry_size);
                file_content_queue.push(std::move(file_content));
            }
        } catch (const ThreadSafeQueueEmptyError& err) {
            break;
        }
    }
}


void count_words(
    ThreadSafeQueue<std::string>& file_content_queue,
    ThreadSafeQueue<std::future<vocabulary_type>>& vocabulary_queue,
    std::mutex& merge_mutex,
    std::condition_variable& merge_mutex_cv,
    std::condition_variable& count_mutex_cv
) {
    Logger log(std::this_thread::get_id(), "count");

    while (true) {
        try {
            std::string str = std::move(file_content_queue.pop());

            vocabulary_type word_vocab;
            text_to_vocabulary(std::move(str), word_vocab);

            std::promise<vocabulary_type> p;
            log.print("add vocabulary, size: " + std::to_string(word_vocab.size()));

            p.set_value(std::move(word_vocab));

            std::unique_lock<std::mutex> locker(merge_mutex);
            while(vocabulary_queue.size() >= vocabulary_queue.upper_bound_m) {
                count_mutex_cv.wait(locker);
            }
            vocabulary_queue.push(p.get_future());
            merge_mutex_cv.notify_all();
        } catch (const ThreadSafeQueueEmptyError& err) {
            break;
        }
    }
}


void merge_vocabularies(
    ThreadSafeQueue<std::future<vocabulary_type>>& vocabulary_queue,
    std::mutex& merge_mutex,
    std::condition_variable& merge_mutex_cv,
    std::condition_variable& count_mutex_cv
) {
    Logger log(std::this_thread::get_id(), "merge");

    while(true) {
        vocabulary_type first;
        vocabulary_type second;
        std::future<vocabulary_type> first_future;
        std::future<vocabulary_type> second_future;

        std::promise<vocabulary_type> p;
        {
            std::unique_lock<std::mutex> locker(merge_mutex);

            if (vocabulary_queue.size() > 1 || !vocabulary_queue.get_finished()) {
                log.print("start merge");

                try {
                    while(vocabulary_queue.size() < 1 && !vocabulary_queue.get_finished()) {
                        log.print("wait first");
                        merge_mutex_cv.wait(locker);
                        log.print("wakeup first");
                    }

                    log.print("first future before moved");
                    first_future = std::move(vocabulary_queue.pop());
                    log.print("first future after moved");
                } catch (const ThreadSafeQueueEmptyError& err) {
                    log.print("exit, first pop");
                    merge_mutex_cv.notify_all();
                    break;
                }

                try {
                    while(vocabulary_queue.size() < 1 && !vocabulary_queue.get_finished()) {
                        log.print("wait second");
                        merge_mutex_cv.wait(locker);
                        log.print("wakeup second");
                    }
                    log.print("second future before moved");
                    second_future = std::move(vocabulary_queue.pop());
                    log.print("second future after moved");
                } catch (const ThreadSafeQueueEmptyError& err) {
                    vocabulary_queue.push(std::move(first_future));
                    log.print("exit, second pop");
                    merge_mutex_cv.notify_all();
                    break;
                }

                vocabulary_queue.push(p.get_future());
                log.print("add future merged");
                merge_mutex_cv.notify_all();
                count_mutex_cv.notify_all();
            } else {
                std::stringstream ss;
                ss << " exit, vocab queue size: " << vocabulary_queue.size() \
                   << " is finished: "<< vocabulary_queue.get_finished();
                log.print(ss.str());

                merge_mutex_cv.notify_all();
                break;
            }
        }

        log.print("before future get");
        first  = first_future.get();
        log.print("first map size: " + std::to_string(first.size()));
        second = second_future.get();
        log.print("second map size: " + std::to_string(second.size()));

        for (const auto& vocab_pair: second) {
            auto vocab_itr = first.find(vocab_pair.first);
            if (vocab_itr != first.end()) {
                vocab_itr->second += vocab_pair.second;
            } else {
                first.insert(vocab_pair);
            }
        }
        second.clear();
        p.set_value(std::move(first));
        log.print("add merged vocabulary");
    }
}

int main() {
    auto start_time = get_current_time_fenced();
    std::string data_folder = "../data";

    ThreadSafeQueue<std::string> filename_queue(100);
    std::thread get_filenames_thread(get_filenames, std::ref(data_folder), std::ref(filename_queue));

    ThreadSafeQueue<std::string> file_content_queue(100);
    std::thread read_files_thread(read_files, std::ref(filename_queue), std::ref(file_content_queue));

    init_locale();
    ThreadSafeQueue<std::future<vocabulary_type>> vocabulary_queue(50);
    int n_count_words_threads = 5;
    std::mutex merge_mutex;
    std::condition_variable merge_mutex_cv;
    std::condition_variable count_mutex_cv;
    std::vector<std::thread> count_words_threads;
    for (int i = 0; i < n_count_words_threads; ++i) {
        count_words_threads.push_back(std::thread(
                count_words,
                std::ref(file_content_queue),
                std::ref(vocabulary_queue),
                std::ref(merge_mutex),
                std::ref(merge_mutex_cv),
                std::ref(count_mutex_cv)
        ));
    }

    int n_merge_threads = 10;
    std::vector<std::thread> merge_threads;
    for (int i = 0; i < n_merge_threads; ++i) {
        merge_threads.push_back(std::thread(
                merge_vocabularies,
                std::ref(vocabulary_queue),
                std::ref(merge_mutex),
                std::ref(merge_mutex_cv),
                std::ref(count_mutex_cv)
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
    merge_mutex_cv.notify_all();

    for (auto& th: merge_threads) {
        th.join();
    }


    auto vocab = vocabulary_queue.pop().get();
    std::cout << "vocab size: " << vocab.size() << std::endl;
    std::cout << std::endl;

    std::ofstream out_a_file;
    out_a_file.open ("res_a.txt");

    for (auto& p: vocab) {
        out_a_file << p.first << "\t\t" << p.second << std::endl;
    }
    out_a_file.close();


    auto finish_time = get_current_time_fenced();
    auto total_time = finish_time - start_time;
    std::cout << "total time: " << to_sec(total_time) << std::endl;
    return 0;
}
