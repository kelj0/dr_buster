#include <iostream>
#include <fstream>
#include <string>

#include <pybind11/pybind11.h>

int get_code(std::string host, int port, std::string path){
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    return NULL;
}

int* parse_url(std::string url){
    // <summary>
    // parses given url and returns a pointer to the array of host, port and path
    // </summary>
    return NULL;
}

std::string** prepare_wordlists(std::string path){
    // <summary>
    // loads words from the wordlists in array of string arrays where number of 
    // string arrays depends on a CPU cores
    // </summary>
    return NULL;
}

int scan_host(std::string host, std::string port, std::string* wordlist, int process_id, std::string path){
    // <summary>
    // scans host word by word from the wordlist using get_code function on each word and yields findings
    // </summary>
    return NULL;
}

int start_scan(std::string url, std::string wordlist_path) {
    try {
        std::ofstream report;
        report.open ("cpp_generated_report.txt");
        report << url << std::endl << wordlist_path << std::endl;
        report.close();
    } catch (...) {
        return 1;
    }

    return 0;
}


namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
    m.def("start_scan", &start_scan, "Scan host");

#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
