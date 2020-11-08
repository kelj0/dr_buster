#include <thread>
#include <iostream>

int main() {
    const int processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores => " << processor_count << std::endl;
    return 0;
}
