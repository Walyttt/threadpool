#ifndef _TREADPOOL
#define _TREADPOOL

#include<iostream>
#include<thread>
#include<memory>
#include<mutex>
#include<functional>
#include<future>
#include<queue>


class thread_task{
private:
    std::packaged_task<void()> task_p;
    int prior_level;    // smaller num means higher prior level
public:
    // Constructor
    thread_task(std::function<void()> task_p, int prior_level):task_p(task_p), prior_level(prior_level){}
    thread_task(thread_task&& t) = default;

    //Disalllow copying
    thread_task& operator=(const thread_task&) = delete;
    thread_task(const thread_task&) = delete;

    // Overload the comparison operators
    bool operator>(const thread_task& t){
        return this->prior_level < t.prior_level;
    }
    bool operator<(const thread_task& t){
        return this->prior_level > t.prior_level;
    }
    bool operator>=(const thread_task& t){
        return this->prior_level <= t.prior_level;
    }
    bool operator<=(const thread_task& t){
        return this->prior_level >= t.prior_level;
    }
    bool operator==(const thread_task& t){
        return this->prior_level == t.prior_level;
    }

};

class threadpool{

    //Built-in thread worker class
    class threadworker{
    private:
        size_t id;
        threadpool* tp_ptr;

    public:
        threadworker(size_t id, threadpool* tp_ptr): id(id), tp_ptr(tp_ptr){};
        ~threadworker();

        void operator()(){
            //TODO
        }
        
    };

private:
    int pool_size;
    std::atomic_bool is_alive;
    std::vector<std::thread> threadworkers_pool;
    std::priority_queue<thread_task, std::vector<thread_task>, std::greater<thread_task>> task_queue;
    std::mutex task_queue_mtx;
    std::mutex thread_sleep_mtx;
    std::condition_variable thread_sleep_cv;

public:
    //Constructors
    threadpool(int pool_size): pool_size(pool_size), is_alive(true){}
    threadpool(const threadpool&) = delete;
    threadpool(threadpool&&) = delete;
    threadpool& operator=(const threadpool) = delete;
    threadpool& operator=(threadpool&&) = delete;

    void init(){
        threadworkers_pool = std::vector<std::thread>(this->pool_size);
        for(int i = 0; i < this->pool_size; i++){
            threadworkers_pool[i] = std::thread(threadworker(i, this));
        }
    }

    //TODO: submit
    template<typename F, typename... Args>
    auto submit(F f, Args&&... args) -> std::future<decltype(f(args...))> {
        
    }

    //TODO: shutdown
};

#endif