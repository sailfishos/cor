#include <cor/util.hpp>
#include <cor/mt.hpp>
#include <deque>

namespace cor
{

Mutex::WLock::WLock(Mutex const &m) : lock_(m.l_)
{
}

Mutex::WLock::WLock(WLock &&from)
    : lock_(std::move(from.lock_))
{
}

Mutex::WLock::~WLock()
{
}

void Mutex::WLock::unlock()
{
    lock_.unlock();
}

void Completion::up()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++counter_;
}

void Completion::down()
{
    std::unique_lock<std::mutex> lock(mutex_);
    --counter_;
    if (!counter_) {
        lock.unlock();
        done_.notify_one();
    }
}

void Completion::wait()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!counter_)
        return;

    done_.wait(lock);
}

class TaskQueueImpl
{
public:
    TaskQueueImpl();
    ~TaskQueueImpl();

    bool enqueue(std::packaged_task<void()>);
    void stop();
    void join() { thread_.join(); }
    bool empty() const;

private:

    void loop();
    void process();

    bool is_running_;
    std::deque<std::packaged_task<void()> > tasks_;
    mutable std::mutex mutex_;
    mutable std::condition_variable ready_;
    std::thread thread_;
};

TaskQueue::TaskQueue()
    : impl_(cor::make_unique<TaskQueueImpl>())
{
}

TaskQueue::TaskQueue(TaskQueue &&src)
    : impl_(std::move(src.impl_))
{
}

TaskQueue::~TaskQueue()
{
}

void TaskQueue::stop()
{
    impl_->stop();
}

void TaskQueue::join()
{
    impl_->join();
}

bool TaskQueue::empty() const
{
    return impl_->empty();
}

bool TaskQueue::enqueue(std::packaged_task<void()> task)
{
    return impl_->enqueue(std::move(task));
}

TaskQueueImpl::TaskQueueImpl()
    : is_running_(true)
    , thread_(std::bind(&TaskQueueImpl::loop, this))
{}

TaskQueueImpl::~TaskQueueImpl()
{
    stop();
    join();
}

void TaskQueueImpl::stop()
{
    if (!is_running_)
        return;

    std::unique_lock<std::mutex> lock(mutex_);
    if (!is_running_)
        return;

    is_running_ = false;
    lock.unlock();
    ready_.notify_one();
}

bool TaskQueueImpl::empty() const
{
    std::lock_guard<std::mutex> l(mutex_);
    return tasks_.empty();
}

bool TaskQueueImpl::enqueue(std::packaged_task<void()> task)
{
    if (!is_running_)
        return false;

    std::unique_lock<std::mutex> lock(mutex_);
    if (!is_running_)
        return false;

    tasks_.push_back(std::move(task));
    lock.unlock();

    ready_.notify_one();
    return true;
}

void TaskQueueImpl::loop()
{
    while (is_running_) {
        process();
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [this]() { return !is_running_ || !tasks_.empty(); });
    }
}

void TaskQueueImpl::process()
{
    while (tasks_.size()) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tasks_.size()) {
            std::packaged_task<void()> task = std::move(tasks_.front());
            tasks_.pop_front();
            lock.unlock();
            task();
        }
    }
}

} // cor
