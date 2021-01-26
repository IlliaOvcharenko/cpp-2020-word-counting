#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <experimental/filesystem>

#include "src/archive_wrapper.h"
#include "src/text_utils.h"

namespace fs = std::experimental::filesystem;
const int MAX_ARCHIVE_SIZE = 10000000;

int main() {
    std::string data_folder = "../data";


    // Get archive filenames
    std::vector<std::string> archive_filenames;
    for (const auto& entry: fs::recursive_directory_iterator(data_folder)) {
        if (entry.path().extension() == ".zip") {
            archive_filenames.push_back(entry.path());
        }
    }
    std::cout << "number of archives: " << archive_filenames.size() << std::endl;
    // Select first 5 archives for test
    archive_filenames = std::vector<std::string>(archive_filenames.begin(), archive_filenames.begin() + 5);


    // Read archive content
    std::queue<std::string> file_content_queue;
    for (const auto& fn: archive_filenames) {
        std::cout << "archive filename: " << fn << std::endl;
        auto archive_reader = ArchiveReader(fn, MAX_ARCHIVE_SIZE);
        while (archive_reader.is_next_file()) {
            size_t entry_size = archive_reader.get_entry_size();

            if (entry_size > MAX_ARCHIVE_SIZE) {
                std::cout << "archive entry is too large, skip it" << std::endl;
                continue;
            }

            char file_content[entry_size];
            archive_reader.read_next_file(file_content, entry_size);
            file_content_queue.push(file_content);
        }

    }

    // Word counting
    init_locale();
    vocabulary_type word_vocab;
    while (!file_content_queue.empty()) {
        std::string str = file_content_queue.front();
        file_content_queue.pop();

        text_to_vocabulary(std::move(str), word_vocab);
    }

    // Print vocabulary
    for (const auto& el: word_vocab) {
        std::cout << el.first << " - " << el.second << std::endl;
    }

    return 0;
}
