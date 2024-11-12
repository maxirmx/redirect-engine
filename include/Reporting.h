#pragma once
#include <memory>
#include <queue>
#include <thread>
#include <unistd.h>


template<class T>
class ReportingBackend
{
public:
    virtual void Report(const std::vector<T> &messages) = 0;
};

template<class T>
class Reporting
{
public:
    Reporting(std::unique_ptr<ReportingBackend<T>> backend, int messages_per_bulk)
            :messages_per_bulk(messages_per_bulk)
    {
        this->reporting = std::move(backend);
        workThread = std::unique_ptr<std::thread>(new std::thread(&Reporting::SenderThread, this));
    }

    ~Reporting()
    {
        bool needWait = false;
        {
            std::lock_guard lock(queue_mutex);
            needWait = !reports.empty();
        }

        for(int j = 0 ; j < 5 && needWait; ++j)
        {
            ::usleep(1000000);
            
            {
                std::lock_guard lock(queue_mutex);
                needWait = !reports.empty();
            }
        }

        needed = false;
        workThread->join();
    }

    void Report(T item)
    {
        std::lock_guard lock(queue_mutex);
        reports.push(item);
    }

private:
    void SenderThread()
    {
        while(needed)
        {
            std::vector<T> report_strings;

            {
                std::lock_guard lock(queue_mutex);
                for(int i = 0 ; i < 100; ++i)
                {
                    if(reports.empty())
                        break;

                    report_strings.push_back(reports.front());
                    reports.pop();
                }
            }

            if(!report_strings.empty())
            {
                try{
                    reporting->Report(report_strings);
                }catch(...)
                {
                    //on error ... return back to queue
                    std::lock_guard lock(queue_mutex);
                    for(auto rec : report_strings)
                        reports.push(rec);
                }
            }else
                ::usleep(1000000);
        }
    }

    volatile bool needed = { true };
    std::unique_ptr<std::thread> workThread;
    std::queue<T> reports;
    std::unique_ptr<ReportingBackend<T>> reporting;
    std::mutex queue_mutex;
    const int messages_per_bulk = 500;
};