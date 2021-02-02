#ifndef CPP_2020_WORD_COUNTING_ARCHIVE_WRAPPER_H
#define CPP_2020_WORD_COUNTING_ARCHIVE_WRAPPER_H

#include <stdexcept>
#include <iostream>

#include <archive.h>
#include <archive_entry.h>

class ArchiveReader {
    std::string filename_m;
    struct archive *a = nullptr;
    struct archive_entry *entry = nullptr;
    int r = 1;

public:
    explicit ArchiveReader(const std::string& filename, size_t max_filesize);

    bool is_next_file();
    size_t get_entry_size();
    void read_next_file(void *file_content_pt, size_t size);

    // TODO is this method required?
    void close();
};


class ArchiveNotFoundError: public std::runtime_error{
public:
    explicit ArchiveNotFoundError(): std::runtime_error("Archive file not found") {}
};

//class ArchiveMaxSizeAchieved: public std::runtime_error{
//public:
//    explicit ArchiveMaxSizeAchieved(): std::runtime_error("Archive max size achieved") {}
//};


#endif //CPP_2020_WORD_COUNTING_ARCHIVE_WRAPPER_H
