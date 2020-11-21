#pragma once
#include <memory>
#include <condition_variable>
#include <memory>


class RWLock {
public:
    RWLock()
    : shared()
    , readerQ(), writerQ()
    , active_readers(0), waiting_writers(0), active_writers(0)
    {}

    void ReadLock() {
        std::unique_lock<std::mutex> lk(shared);
        while( waiting_writers != 0 )
            readerQ.wait(lk);
        ++active_readers;
        lk.unlock();
    }

    void ReadUnlock() {
        std::unique_lock<std::mutex> lk(shared);
        --active_readers;
        lk.unlock();
        writerQ.notify_one();
    }

    void WriteLock() {
        std::unique_lock<std::mutex> lk(shared);
        ++waiting_writers;
        while( active_readers != 0 || active_writers != 0 )
            writerQ.wait(lk);
        ++active_writers;
        lk.unlock();
    }

    void WriteUnlock() {
        std::unique_lock<std::mutex> lk(shared);
        --waiting_writers;
        --active_writers;
        if(waiting_writers > 0)
            writerQ.notify_one();
        else
            readerQ.notify_all();
        lk.unlock();
    }

private:
    std::mutex              shared;
    std::condition_variable readerQ;
    std::condition_variable writerQ;
    int                     active_readers;
    int                     waiting_writers;
    int                     active_writers;
};

class RWWriteGuard
{
public:
    RWWriteGuard(RWLock *lock)
    {
        this->lock = lock;
        lock->WriteLock();
    }

    ~RWWriteGuard()
    {
        lock->WriteUnlock();
    }
    
private:
    

    RWLock *lock;
};

class RWReadGuard
{
public:
    RWReadGuard(RWLock *lock)
    {
        this->lock = lock;
        lock->ReadLock();
    }

    ~RWReadGuard()
    {
        lock->ReadUnlock();
    }
private:
    

    RWLock *lock;
};