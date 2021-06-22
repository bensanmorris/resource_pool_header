#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

/**
* A thread-safe resource pool containing a user specified number of resources of type T.
* \code{.cpp}
*    struct resource {};
*    resource_pool<resource> pool(POOL_SIZE, [] () {
*        return new resource();
*    });
*    auto resource = pool.acquire(); // nb. will block until resource is available or will return nullptr if pool is shutdown (from any thread)
*    // use resource...
*    pool.release(resource);
*    pool.shutdown();
* \endcode
*/
template <class T>
class resource_pool {

public:

    struct resource {

        resource(resource_pool<T>& pool_, const T& resource_, bool inUse_) :
            pool(pool_), raw_resource(resource_), inUse(inUse_) {}

        resource_pool<T>& pool;
        T raw_resource;
        bool inUse;
    };

private:

    std::vector< std::shared_ptr<resource> > resources;

    size_t size;
    std::function<T(void)> fn_new;
    int haveResources;
    std::mutex lock;
    std::condition_variable condition;
    bool stop;

public:

    /**
    * Constructs a resource pool containing size resources.
    * \arg size The number of resources
    * \new_fn A user specified factory function that creates a new T
    */
    resource_pool(size_t size_, std::function<T(void)> new_fn)
        : size(std::max<size_t>(size_, 1)), fn_new(new_fn), haveResources(size), stop(false) {
        resources.reserve(size);
        for (int i = 0; i < size; i++) {
            resources.push_back(nullptr);
        }
    }

    /**
    * Acquires a resource if one is available otherwise will block until either a resource becomes available or shutdown has been called (from any thread).
    * \returns A resource if one is available or nullptr if shutdown has been called
    * \sa shutdown
    */
    std::shared_ptr<resource> acquire() {
        {
            std::lock_guard<std::mutex> guard(lock);
            // no need to wait if we've been asked to stop
            if (stop) {
                return nullptr;
            }
        }

        std::unique_lock<std::mutex> guard(lock);
        condition.wait(guard, [this] {
            return haveResources > 0 || stop;
        });

        if (stop) {
            return nullptr;
        }

        std::shared_ptr<resource> availableResource;
        for (int i = 0; i < resources.size(); i++) {
            auto res = resources[i];
            if (res == nullptr) {
                availableResource = std::make_shared<resource>(*this, fn_new(), true);
                resources[i] = availableResource;
                break;
            } else if (!res->inUse) {
                availableResource = res;
                availableResource->inUse = true;
                break;
            }
        }

        haveResources--;
        return availableResource;
    }

    /**
    * Releases an acquired resource.
    */
    void release(std::shared_ptr<resource> raw_resource) {
        {
            std::lock_guard<std::mutex> guard(lock);
            if (!raw_resource || !raw_resource->inUse)
                return;
            raw_resource->inUse = false;
            haveResources++;
        }
        condition.notify_one();
    }

    /**
    * Returns the number of resources available
    */
    int resourcesAvailable() {
        std::lock_guard<std::mutex> guard(lock);
        return haveResources;
    }

    /**
    * Shuts the resource pool down. All threads blocking in acquire() will cease blocking.
    * \sa acquire
    */
    void shutdown() {
        {
            std::lock_guard<std::mutex> guard(lock);
            stop = true;
        }
        condition.notify_all(); // notify all blocking threads we are stopping
    }
};