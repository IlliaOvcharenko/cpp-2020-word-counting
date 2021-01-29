#include <iostream>
#include <vector>
#include <utility>
#include <thread>
#include <chrono>

#include "src/thread_safe_queue.h"

typedef ThreadSafeQueue<std::pair<int, int>> queue_type;

void add_value_in_queue(int value, queue_type& q, int sleep_time=1) {
    int count = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        ++count;
        q.push(std::make_pair(count, value));
    }
}

void get_value_from_queue(queue_type& q, int sleep_time=1) {
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        auto val = q.front_pop();
        std::cout << "pop number: " << val.first << " from thread: " << val.second << std::endl;
    }
}

void print_queue(queue_type& q) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
//        std::unique_lock<std::mutex> locker(q.write_mutex_m);
        std::cout << "queue size: " << q.size() << std::endl;
    }

}


int main() {

    int n_add_threads = 7;
    int n_get_threads = 2;
    int upper_bound = 10;
//    int n_add_threads = 2;
//    int n_get_threads = 7;

    queue_type q(upper_bound);

    std::vector<std::thread> add_threads;
    for (int i = 0; i < n_add_threads; ++i) {
        add_threads.push_back(std::thread(add_value_in_queue, i+1, std::ref(q), 1));
    }

    std::vector<std::thread> get_threads;
    for (int i = 0; i < n_get_threads; ++i) {
        get_threads.push_back(std::thread(get_value_from_queue, std::ref(q), 1));
    }

    std::thread print_th(print_queue, std::ref(q));


    for (auto& th: add_threads) {
        th.join();
    }
    for (auto& th: get_threads) {
        th.join();
    }
    print_th.join();

    return 0;
}

