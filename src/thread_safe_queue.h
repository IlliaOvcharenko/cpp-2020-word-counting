#ifndef CPP_2020_WORD_COUNTING_THREAD_SAFE_QUEUE_H
#define CPP_2020_WORD_COUNTING_THREAD_SAFE_QUEUE_H


#include <iostream>
#include <queue>
#include <vector>
#include <utility>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <chrono>
#include <condition_variable>

class ThreadSafeQueueEmptyError: public std::runtime_error{
public:
    explicit ThreadSafeQueueEmptyError(): std::runtime_error("Queue is empty") {}
};

template <typename T>
class ThreadSafeQueue {
    bool is_empty;
    std::queue<T> queue_m;
    std::mutex write_mutex_m;

    int upper_bound_m;
    std::condition_variable upper_bound_cv_m;
    std::condition_variable empty_cv_m;
public:
    explicit ThreadSafeQueue(int upper_bound) : upper_bound_m(upper_bound), is_empty(false) { }

    void push(T el) {
        std::unique_lock<std::mutex> locker(write_mutex_m);
        while(queue_m.size() >= upper_bound_m) {
            upper_bound_cv_m.wait(locker);
        }

        queue_m.push(el);

        empty_cv_m.notify_one();
    }

    T front_pop() {
        std::unique_lock<std::mutex> locker(write_mutex_m);

        while (queue_m.empty() || is_empty) {
            empty_cv_m.wait(locker);
        }

        if (is_empty) {
            throw ThreadSafeQueueEmptyError();
        }

        auto el = queue_m.front();
        queue_m.pop();

        if (queue_m.size() < upper_bound_m) {
            upper_bound_cv_m.notify_one();
        }

        return el;
    }

    size_t size() {
        return queue_m.size();
    }

    void set_empty() {
        is_empty = true;
    }
};



#endif //CPP_2020_WORD_COUNTING_THREAD_SAFE_QUEUE_H
