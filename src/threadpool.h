#ifndef _TREADPOOL
#define _TREADPOOL

#include<iostream>
#include<thread>
#include<memory>
#include<mutex>
#include<functional>
#include<future>
#include<queue>

#define DEFAULT_PRIOR_LEVEL 20

class thread_task{
private:
    std::function<void()> task_f;
    int prior_level;    // smaller num means higher prior level
public:
    // Constructor
    thread_task(std::function<void()> task_f, int prior_level):task_f(task_f), prior_level(prior_level){}
    thread_task(thread_task&& t) = default;

    //excute
    void excute() const {
        task_f();
    }

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
public:
    //Built-in thread worker class
    class threadworker{
    private:
        size_t id;
        threadpool* tp_ptr;

    public:
        threadworker(size_t id, threadpool* tp_ptr): id(id), tp_ptr(tp_ptr){};
        ~threadworker();

        void operator()(){
            while (tp_ptr->is_alive){
                
                std::unique_lock<std::mutex> lck(tp_ptr->thread_sleep_mtx);

                tp_ptr->thread_sleep_cv.wait(lck, (!tp_ptr->task_queue.empty() || !tp_ptr->is_alive));

                if(!tp_ptr->task_queue.empty() && tp_ptr->is_alive){
                    std::unique_lock<std::mutex> lck(tp_ptr->task_queue_mtx);

                    const thread_task& task = tp_ptr->task_queue.top();

                    tp_ptr->task_queue.pop();

                    task.excute();
                }
            }
            
        }
        
    };
    friend class threadworker;

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

    //submit 1: default prior level 20
    template<typename F, typename... Args>
    auto submit(F f, Args&&... args) -> std::future<decltype(f(args...))> {
        
        std::function<decltype(f(args...))()> task_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...); 
    
        auto task_package_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(task_func);

        std::function<void()> wrapper_func = [task_package_ptr](){
            (*task_package_ptr)();
        };

        {
            std::unique_lock<std::mutex> q_lck(task_queue_mtx);
            task_queue.push(std::move(thread_task{wrapper_func, DEFAULT_PRIOR_LEVEL}));
        }

        thread_sleep_cv.notify_one();

        return task_package_ptr->get_future();
    }

    //submit 2: given prior level
    template<typename F, typename... Args>
    auto submit(int prior_level, F f, Args&&... args) -> std::future<decltype(f(args...))> {
        
        std::function<decltype(f(args...))()> task_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...); 
    
        auto task_package_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(task_func);

        std::function<void()> wrapper_func = [task_package_ptr](){
            (*task_package_ptr)();
        };

        {
            std::unique_lock<std::mutex> q_lck(task_queue_mtx);
            task_queue.push(std::move(thread_task{wrapper_func, prior_level}));
        }

        thread_sleep_cv.notify_one();

        return task_package_ptr->get_future();
    }

    //TODO: shutdown
    void shutdown(){
        is_alive = false;
        thread_sleep_cv.notify_all();

        for(auto& worker : threadworkers_pool){
            if(worker.joinable()){
                worker.join();
            }
        }
    }

};

#endif