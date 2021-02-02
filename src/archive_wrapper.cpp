#include "archive_wrapper.h"

ArchiveReader::ArchiveReader(const std::string& filename, size_t max_size)
    : filename_m(filename)
{
    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    r = archive_read_open_filename(a, filename_m.c_str(), 10240);

    if (r != ARCHIVE_OK) {
        throw ArchiveNotFoundError();
    }
}

bool ArchiveReader::is_next_file() {
    return archive_read_next_header(a, &entry) == ARCHIVE_OK;
}

size_t ArchiveReader::get_entry_size() {
    return archive_entry_size(entry);
}

void ArchiveReader::read_next_file(void *file_content_pt, size_t size) {
    archive_read_data(a, file_content_pt, size);
}



