#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <unistd.h> 
#include <string> 
#include <cstring>

#include <pybind11/pybind11.h>

int get_code(const std::string host, int port, std::string path){
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    char buffer[14] = {0}; 
    std::string ip;
    const char* request = "GET /"+path+" HTTP/1.1\r\nHost:"+host+"\r\n\r\n";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(80); // TODO add https support
    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <=0 ) { 
        printf("\nInvalid address or you entered hostname and not IP! I'll convert it\n");
        std::string ip = inet_ntoa(**(in_addr**)gethostbyname(host.c_str())->h_addr_list);
        if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) { 
            printf("Failed to convert %s to IP\n", host.c_str());
        }else{
            printf("Coverted %s to %s\n", host.c_str(), ip.c_str());
        }
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 
    send(sock, request, strlen(request), 0); 
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

str::string* parse_url(std::string url){
    // <summary>
    // parses given url and returns a pointer to the array of host, port and path
    // </summary>
    std::string host = "";
    int port = 0;
    std::string path = "";
    try {
        std::string tmp = url.substr(url.find("//")+2, url.length());
        host = tmp.substr(0, tmp.find(":"));
        port = stoi(tmp.substr(tmp.find(":")+1, tmp.find("/")));
        path = tmp.substr(tmp.find("/")+1, tmp.length());
    } catch (...) { 
        return -1;
    }
    str::string ret[3] = { host.c_str(), std::to_string(port), path.c_str() };
    return ret;
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
