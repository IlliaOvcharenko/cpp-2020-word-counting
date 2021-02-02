#include <iostream>
#include <string>
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

void get_filenames(const std::string& data_folder, ThreadSafeQueue<std::string>& filename_queue) {
    // Get archive filenames
    int fn_counter = 0;
    for (const auto& entry: fs::recursive_directory_iterator(data_folder)) {
        if (entry.path().extension() == ".zip") {
            filename_queue.push(entry.path());
            std::cout << "add filename: " << entry.path() << std::endl;
            // Select first 5 archives for test
            if (++fn_counter >= 10) {
                break;
            }

        }
    }
}

void read_files(
        ThreadSafeQueue<std::string>& filename_queue,
        ThreadSafeQueue<std::string>& file_content_queue
) {
    while (true) {
        try {
            std::string fn = filename_queue.pop();

            std::cout << "archive filename: " << fn << std::endl;
            auto archive_reader = ArchiveReader(fn, MAX_ARCHIVE_SIZE);

            while (archive_reader.is_next_file()) {
                size_t entry_size = archive_reader.get_entry_size();

                if (entry_size > MAX_ARCHIVE_SIZE) {
                    std::cout << "archive entry is too large, skip it" << std::endl;
                    continue;
                }

                std::string file_content(entry_size, char{});
                archive_reader.read_next_file(&file_content[0], entry_size);
//                std::cout << file_content.length() << std::endl;
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
    std::condition_variable& merge_mutex_cv
) {
    while (true) {
        try {
            std::string str = std::move(file_content_queue.pop());

            vocabulary_type word_vocab;
            text_to_vocabulary(std::move(str), word_vocab);

            std::promise<vocabulary_type> p;
            std::cout << "add vocabulary, size: " << word_vocab.size() << std::endl;
            p.set_value(std::move(word_vocab));

            std::unique_lock<std::mutex> locker(merge_mutex);
            vocabulary_queue.push(p.get_future());
            merge_mutex_cv.notify_all();

//            std::cout << "add vocabulary, size: " << word_vocab.size() << std::endl;
        } catch (const ThreadSafeQueueEmptyError& err) {
            break;
        }
    }
}


void merge_vocabularies(
    ThreadSafeQueue<std::future<vocabulary_type>>& vocabulary_queue,
    std::mutex& merge_mutex,
    std::condition_variable& merge_mutex_cv
) {
    while(true) {
        vocabulary_type first;
        vocabulary_type second;
        std::future<vocabulary_type> first_future;
        std::future<vocabulary_type> second_future;

        std::promise<vocabulary_type> p;
        {
            std::unique_lock<std::mutex> locker(merge_mutex);

            if (vocabulary_queue.size() > 1 || !vocabulary_queue.get_finished()) {
                std::cout << "start merge" << std::endl;


                try {
                    while(vocabulary_queue.size() < 1 && !vocabulary_queue.get_finished()) {
                        std::cout << "wait first" << std::endl;
                        merge_mutex_cv.wait(locker);
                    }
                    first_future = std::move(vocabulary_queue.pop());
                } catch (const ThreadSafeQueueEmptyError& err) {
                    std::cout << "exit, first pop" << std::endl;
                    break;
                }

                try {
                    while(vocabulary_queue.size() < 1 && !vocabulary_queue.get_finished()) {
                        std::cout << "wait second" << std::endl;
                        merge_mutex_cv.wait(locker);
                    }
                    second_future = std::move(vocabulary_queue.pop());
                } catch (const ThreadSafeQueueEmptyError& err) {
                    vocabulary_queue.push(std::move(first_future));
                    std::cout << "exit, second pop" << std::endl;
                    break;
                }

                vocabulary_queue.push(p.get_future());
                std::cout << "add future merged" << std::endl;
                merge_mutex_cv.notify_all();
            } else {
                std::cout << "exit, vocab queue size: " << vocabulary_queue.size() \
                          << " is finished: "<< vocabulary_queue.get_finished() << std::endl;
                break;
            }
        }
        std::cout << "before future get" << std::endl;
        first  = first_future.get();
        std::cout << "first map size: " << first.size() << std::endl;
        second = second_future.get();
        std::cout << "second map size: " << second.size() << std::endl;

        // merge vocabs
        for (const auto& vocab_pair: second) {
            auto vocab_itr = first.find(vocab_pair.first);
            if (vocab_itr != first.end()) {
                vocab_itr->second += vocab_pair.second;
            } else {
                first.insert(vocab_pair);
            }
        }
        p.set_value(std::move(first));
        std::cout << "add merged vocabulary" << std::endl;
    }
}

int main() {
    auto start_time = get_current_time_fenced();
    std::string data_folder = "../data";

    ThreadSafeQueue<std::string> filename_queue(100);
    std::thread get_filenames_thread(get_filenames, std::ref(data_folder), std::ref(filename_queue));

//    get_filenames(std::ref(data_folder), std::ref(filename_queue));
//    while (!filename_queue.empty()) {
//        std::string fn = filename_queue.pop();
//        std::cout << "archive filename: " << fn << std::endl;
//    }
//    exit(0);

    ThreadSafeQueue<std::string> file_content_queue(100);
    std::thread read_files_thread(read_files, std::ref(filename_queue), std::ref(file_content_queue));

//    read_files(std::ref(filename_queue), std::ref(file_content_queue));
//    while (!filename_queue.empty()) {
//        std::cout << filename_queue.pop() << std::endl;
//    }

//    std::cout << std::endl;
//    while (!file_content_queue.empty()) {
//        std::cout << "after: " << file_content_queue.pop().length() << std::endl;
//    }

    init_locale();
    ThreadSafeQueue<std::future<vocabulary_type>> vocabulary_queue(100);
    int n_count_words_threads = 3;
    std::mutex merge_mutex;
    std::condition_variable merge_mutex_cv;
    std::vector<std::thread> count_words_threads;
    for (int i = 0; i < n_count_words_threads; ++i) {
        count_words_threads.push_back(std::thread(
                count_words,
                std::ref(file_content_queue),
                std::ref(vocabulary_queue),
                std::ref(merge_mutex),
                std::ref(merge_mutex_cv)
        ));
    }

    int n_merge_threads = 3;
    std::vector<std::thread> merge_threads;
    for (int i = 0; i < n_merge_threads; ++i) {
        merge_threads.push_back(std::thread(
                merge_vocabularies,
                std::ref(vocabulary_queue),
                std::ref(merge_mutex),
                std::ref(merge_mutex_cv)
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

    std::cout << std::endl;
    std::cout << "vocabulary left: " << vocabulary_queue.size() << std::endl;
    while (!vocabulary_queue.empty()) {
        auto vacab = vocabulary_queue.pop().get();
        std::cout << "vocab size: " << vacab.size() << std::endl;
        std::cout << std::endl;

    }

    auto finish_time = get_current_time_fenced();
    auto total_time = finish_time - start_time;
    std::cout << "total time: " << to_sec(total_time) << std::endl;
    return 0;
}
