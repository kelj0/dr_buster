#include <string> 
#include <cstring>
#include <vector>
#include <iostream>

std::vector<std::string> parse_url(std::string input) {
    std::string host = "";
    int port = 0;
    std::string path = "";
    try {
        std::string url = input.substr(input.find("//")+2, input.length());
        host = url.substr(0, url.find(":"));
        port = stoi(url.substr(url.find(":")+1, url.find("/")));
        path = url.substr(url.find("/")+1, url.length());
    } catch (...) { 
        return std::vector<std::string>();
    }
    return std::vector<std::string> { host, std::to_string(port), path };
}



int main() {
    std::string input = "https://test.com:8080/test_path";

    std::vector<std::string> a = parse_url(input);
    std::string test1 = a[0];
    int test2 = std::stoi(a[1]);
    std::string test3 = a[2];
    std::cout << test1 << " " << test2 << " " << test3 << std::endl;

    return 0;
}
