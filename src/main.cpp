#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string> 
#include <cstring>

#include <pybind11/pybind11.h>

int get_code(std::string host, int port, std::string path){
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    char buffer[14] = {0}; 
    const char* request = "GET / HTTP/1.1\r\nHost:kelj0.com\r\n\r\n";
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(80); 
    if(inet_pton(AF_INET, "46.101.220.126", &serv_addr.sin_addr)<=0) { 
        printf("\nInvalid address/Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 
    send(sock , request, strlen(request) , 0 ); 
    int i = 0; 
    read(sock, buffer, 14);
    for(;buffer[i]!='\r';++i);
    
    std::string str_code;
    for(int j = 9; j<i;++j){
        str_code.push_back(buffer[j]);
    };
    int code = stoi(str_code);
    printf("Status code => %d\n", code); 

    return code;
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
