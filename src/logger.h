#ifndef CPP_2020_WORD_COUNTING_LOGGER_H
#define CPP_2020_WORD_COUNTING_LOGGER_H

class Logger {
    std::thread::id thread_id_m;
    std::string work_name_m;
public:
    static bool VERBOSE;

    Logger(std::thread::id thread_id, std::string work_name)
            : thread_id_m(thread_id)
            , work_name_m(work_name) { }

    void print(const std::string& msg) {
        if (!VERBOSE) return;
        std::cout << "[" << work_name_m << ": " << thread_id_m << "] " << msg << std::endl;
    }
};

#endif //CPP_2020_WORD_COUNTING_LOGGER_H
