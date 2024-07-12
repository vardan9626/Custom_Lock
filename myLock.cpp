#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class SpinMuteLock {
public:
    SpinMuteLock(int spin_loops) : spin_loops_(spin_loops), is_locked_(false) {}

    void lock() {
        for (int i = 0; i < spin_loops_; ++i) {
            if (try_lock()) {
                return;
            }
            // Spin wait
        }
        // Fallback to sleep and wait
        std::unique_lock<std::mutex> ul(mtx_);
        cv_.wait(ul, [this] { return !is_locked_; });
        is_locked_ = true;
    }

    void unlock() {
        {
            std::lock_guard<std::mutex> lg(mtx_);
            is_locked_ = false;
        }
        cv_.notify_one();
    }

    bool try_lock() {
        std::lock_guard<std::mutex> lg(mtx_);
        if (!is_locked_) {
            is_locked_ = true;
            return true;
        }
        return false;
    }

private:
    int spin_loops_;
    bool is_locked_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

void worker(SpinMuteLock& lock, int id) {
    lock.lock();
    std::cout << "Thread " << id << " has acquired the lock." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Thread " << id << " is releasing the lock." << std::endl;
    lock.unlock();
}

int main() {
    SpinMuteLock lock(1000);

    std::thread t1(worker, std::ref(lock), 1);
    std::thread t2(worker, std::ref(lock), 2);

    t1.join();
    t2.join();

    return 0;
}
