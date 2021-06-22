# A header only resource pool

A small thread-safe fixed size resource manager that blocks until a requested resource is available (or shutdown has been requested).

# Demo

Windows / VS 2017:
```
git clone https://github.com/bensanmorris/resource_pool_header.git
cmake -B build -G "Visual Studio 15 2017" -A x64 -Dgtest_force_shared_crt=ON .
cd build
cmake --build . --config Release
ctest -C Release
```
