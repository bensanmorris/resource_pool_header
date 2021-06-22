#include "gtest/gtest.h"
#include "resource_pool.h"
#include <chrono>
#include <functional>
#include <thread>

TEST(ResourcePoolTest, AcquireReleaseTest) {

    // an example resource
    struct resource {};

    // a Resource factory function
    auto resource_factory_function = [] () {
        return resource();
    };

    // create a pool with the above details
    static const int POOL_SIZE = 3;
    resource_pool<resource> pool(POOL_SIZE, resource_factory_function);

    // use it
    auto resource = pool.acquire();
    EXPECT_EQ(pool.resourcesAvailable(), POOL_SIZE-1);
    pool.release(resource);
    EXPECT_EQ(pool.resourcesAvailable(), POOL_SIZE); // pool doesn't shrink so that resources are ready for next callees
}

TEST(ResourcePoolTest, ShutdownTest) {

    static const int POOL_SIZE = 3;

    // pool (that will also observe server shutdown requests)
    struct resource {};
    resource_pool<resource> pool(POOL_SIZE, [] () {
        return resource();
    });

    // deliberately request too many resources forcing acquire() to block
    auto t1 = std::thread([&] () {
        for (int i = 0; i <= POOL_SIZE; i++) {
            pool.acquire();
        }
    });
    
    // shutdown the pool (from the main thread - nb. thread t1 which is blocking will return owing to shutdown having
    // been requested)
    while (pool.resourcesAvailable() > 0) {
        // sit and wait until all resources have been acquired
    }

    // okay all resource have been acquire except...1 more > pool size so let's sleep for a bit and wait for that to happen 
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // now request the pool shutdown, at this point all listeners are sitting and waiting, shutdown() will notify them all to exit
    pool.shutdown();

    // join the background thread used for testing resource acquisition
    t1.join();

    // if we get here then the test passed otherwise the test will hang
}

TEST(ResourcePoolTest, DoubleReleaseTest) {
    // an example resource
    struct resource {};

    // a Resource factory function
    auto resource_factory_function = [] () {
        return resource();
    };

    // create a pool with the above details
    static const int POOL_SIZE = 3;
    resource_pool<resource> pool(POOL_SIZE, resource_factory_function);

    // use it
    auto resource = pool.acquire();
    EXPECT_EQ(pool.resourcesAvailable(), POOL_SIZE - 1);
    pool.release(resource);
    EXPECT_EQ(pool.resourcesAvailable(), POOL_SIZE);

    // double releasing the same resource shouldn't increase the pool size
    pool.release(resource);
    EXPECT_EQ(pool.resourcesAvailable(), POOL_SIZE);
}
