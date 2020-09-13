#include <iostream>
#include <fstream>
#include <string>

#include <pybind11/pybind11.h>

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