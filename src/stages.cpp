#include "stages.h"


void get_filenames(const std::string& data_folder, ThreadSafeQueue<std::string>& filename_queue) {
    Logger log(std::this_thread::get_id(), "index");

    int fn_counter = 0;
    for (const auto& entry: fs::recursive_directory_iterator(data_folder)) {
        if (entry.path().extension() == ".zip") {
            filename_queue.push(entry.path().string());

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
        std::condition_variable& pop_during_merge_cv,
        std::condition_variable& push_during_merge_cv
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
            while(vocabulary_queue.size() >= vocabulary_queue.get_upper_bound()) {
                push_during_merge_cv.wait(locker);
            }
            vocabulary_queue.push(p.get_future());
            pop_during_merge_cv.notify_one();
        } catch (const ThreadSafeQueueEmptyError& err) {
            break;
        }
    }
}


void merge_vocabularies(
        ThreadSafeQueue<std::future<vocabulary_type>>& vocabulary_queue,
        std::mutex& merge_mutex,
        std::condition_variable& pop_during_merge_cv,
        std::condition_variable& push_during_merge_cv
) {
    Logger log(std::this_thread::get_id(), "merge");

    while(true) {

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
                        pop_during_merge_cv.wait(locker);
                        log.print("wakeup first");
                    }

                    log.print("first future before moved");
                    first_future = std::move(vocabulary_queue.pop());
                    log.print("first future after moved");
                } catch (const ThreadSafeQueueEmptyError& err) {
                    log.print("exit, first pop");
                    pop_during_merge_cv.notify_all();
                    break;
                }

                try {
                    while(vocabulary_queue.size() < 1 && !vocabulary_queue.get_finished()) {
                        log.print("wait second");
                        pop_during_merge_cv.wait(locker);
                        log.print("wakeup second");
                    }
                    log.print("second future before moved");
                    second_future = std::move(vocabulary_queue.pop());
                    log.print("second future after moved");
                } catch (const ThreadSafeQueueEmptyError& err) {
                    vocabulary_queue.push(std::move(first_future));
                    log.print("exit, second pop");
                    pop_during_merge_cv.notify_all();
                    break;
                }

                vocabulary_queue.push(p.get_future());
                log.print("add future merged");
                pop_during_merge_cv.notify_one();
                push_during_merge_cv.notify_one();
            } else {
                std::stringstream ss;
                ss << " exit, vocab queue size: " << vocabulary_queue.size() \
                   << " is finished: "<< vocabulary_queue.get_finished();
                log.print(ss.str());

                pop_during_merge_cv.notify_all();
                break;
            }
        }
        vocabulary_type first;
        vocabulary_type second;

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
        p.set_value(std::move(first));
        log.print("add merged vocabulary");
    }
}
