#ifndef CPP_2020_WORD_COUNTING_THREAD_SAFE_QUEUE_H
#define CPP_2020_WORD_COUNTING_THREAD_SAFE_QUEUE_H

#include <iostream>
#include <queue>
#include <vector>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>


class ThreadSafeQueueEmptyError: public std::runtime_error{
public:
    explicit ThreadSafeQueueEmptyError(): std::runtime_error("Queue is empty") {}
};

template <typename T>
class ThreadSafeQueue {
    std::queue<T> queue_m;
//    std::mutex write_mutex_m;
    bool is_finished;
    std::mutex write_mutex_m;

    int upper_bound_m;
    std::condition_variable upper_bound_cv_m;
    std::condition_variable empty_cv_m;
public:


    explicit ThreadSafeQueue(int upper_bound) : upper_bound_m(upper_bound), is_finished(false) { }

    void push(T&& el) {
        std::unique_lock<std::mutex> locker(write_mutex_m);
        while(queue_m.size() >= upper_bound_m) {
            upper_bound_cv_m.wait(locker);
        }

        queue_m.push(std::forward<T>(el));

        empty_cv_m.notify_one();
    }

    // TODO if i need to return reference here?
    T pop() {
        std::unique_lock<std::mutex> locker(write_mutex_m);

        while (queue_m.empty() && (!is_finished)) {
            empty_cv_m.wait(locker);
        }

        if (empty()) {
            throw ThreadSafeQueueEmptyError();
        }

        T el = std::move(queue_m.front());
        queue_m.pop();

        if (queue_m.size() < upper_bound_m) {
            upper_bound_cv_m.notify_one();
        }

        return el;
    }

    size_t size() {
        return queue_m.size();
    }

    void set_finished() {
        std::unique_lock<std::mutex> locker(write_mutex_m);
        is_finished = true;
        empty_cv_m.notify_all();
    }

    bool get_finished() {
        return is_finished;
    }

    bool empty() {
        return queue_m.empty() && is_finished;
    }
};


#endif //CPP_2020_WORD_COUNTING_THREAD_SAFE_QUEUE_H
