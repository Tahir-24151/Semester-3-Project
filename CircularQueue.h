#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <stdexcept>

using namespace std;

template<typename T, size_t CAPACITY = 100>
class CircularQueue {
private:
    T buffer[CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    
    mutable mutex mtx;
    condition_variable notEmpty;
    condition_variable notFull;
    
    atomic<bool> closed;

public:
    CircularQueue() : head(0), tail(0), count(0), closed(false) {}

    ~CircularQueue() {
        close();
    }

    bool enqueue(const T& item) {
        unique_lock<mutex> lock(mtx);
        
        notFull.wait(lock, [this] { 
            return count < CAPACITY || closed.load(); 
        });
        
        if (closed.load()) {
            return false;
        }
        
        buffer[tail] = item;
        tail = (tail + 1) % CAPACITY;
        count++;
        
        notEmpty.notify_one();
        
        return true;
    }

    bool dequeue(T& item) {
        unique_lock<mutex> lock(mtx);
        
        notEmpty.wait(lock, [this] { 
            return count > 0 || closed.load(); 
        });
        
        if (closed.load() && count == 0) {
            return false;
        }
        
        item = buffer[head];
        head = (head + 1) % CAPACITY;
        count--;
        
        notFull.notify_one();
        
        return true;
    }

    bool tryEnqueue(const T& item) {
        unique_lock<mutex> lock(mtx);
        
        if (count >= CAPACITY || closed.load()) {
            return false;
        }
        
        buffer[tail] = item;
        tail = (tail + 1) % CAPACITY;
        count++;
        
        notEmpty.notify_one();
        return true;
    }

    bool tryDequeue(T& item) {
        unique_lock<mutex> lock(mtx);
        
        if (count == 0) {
            return false;
        }
        
        item = buffer[head];
        head = (head + 1) % CAPACITY;
        count--;
        
        notFull.notify_one();
        return true;
    }

    size_t size() const {
        lock_guard<mutex> lock(mtx);
        return count;
    }

    bool isEmpty() const {
        lock_guard<mutex> lock(mtx);
        return count == 0;
    }

    bool isFull() const {
        lock_guard<mutex> lock(mtx);
        return count >= CAPACITY;
    }

    size_t capacity() const {
        return CAPACITY;
    }

    void close() {
        {
            lock_guard<mutex> lock(mtx);
            closed.store(true);
        }
        notEmpty.notify_all();
        notFull.notify_all();
    }

    bool isClosed() const {
        return closed.load();
    }

    void clear() {
        lock_guard<mutex> lock(mtx);
        head = 0;
        tail = 0;
        count = 0;
        notFull.notify_all();
    }
};

#endif
