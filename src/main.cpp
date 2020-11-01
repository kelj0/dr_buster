#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <unistd.h> 
#include <string> 
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>
#include <pybind11/pybind11.h>

int get_code(const std::string host, int port, std::string path) {
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    char buffer[14] = {0}; 
    std::string ip;
    const std::string request = "GET /" + path + " HTTP/1.1\r\nHost:" + host + "\r\n\r\n";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        std::cout << "\n Socket creation error \n"; 
        return -1; 
    } 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(80); // TODO add https support
    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <=0 ) { 
        std::cout << "\nInvalid address or you entered hostname and not IP! I'll convert it\n";
        std::string ip = inet_ntoa(**(in_addr**)gethostbyname(host.c_str())->h_addr_list);
        if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) { 
            std::cout << "Failed to convert " << host << " to IP\n";
        }else{
            std::cout << "Converted " << host << " to " << ip << std::endl;
        }
    } 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        std::cout << "\nConnection Failed \n";
        return -1; 
    } 
    send(sock, request.c_str(), request.size(), 0); 
    int i = 0; 
    read(sock, buffer, 14);
    for(;buffer[i]!='\r';++i);
    
    std::string str_code;
    for(int j = 9; j<i;++j){
        str_code.push_back(buffer[j]);
    };
    int code = std::stoi(str_code);
    std::cout << "Status code => " << code << std::endl; 

    return code;
}

int get_cpu_cores() {
    // <summary>
    // returns number of cores in integer
    // </summary>
    std::cout << "Getting number of cores..\n";
    const int cores = std::thread::hardware_concurrency();
    std::cout << "Detected " << cores << " cores on this system\n";
    return cores;
}

std::vector<std::string> parse_url(std::string url) {
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
        return std::vector<std::string>();
    }
    return std::vector<std::string> { host, std::to_string(port), path };
}

std::vector<std::vector<std::string>> prepare_wordlists(std::string path) {
    // <summary>
    // loads words from the wordlists in vector<vector<string>> where number of 
    // vectors in vector depends on a CPU cores
    // </summary>
    int processes_count = 0;
    if (get_cpu_cores() <= 4){
        processes_count = 32;
    } else {
        processes_count = 64;
    }
    std::ifstream file(path);
    std::vector<std::vector<std::string>> wordlists(processes_count);
    std::vector<std::string> wordlist;
    if (file.is_open()){
        std::string line;
        while(std::getline(file, line)) {
             if (line.size() > 0) {
                 wordlist.push_back(line);
             }
        }
        file.close();
    } else {
        std::cout << "ERR: Cant open wordlist!\n";
        return std::vector<std::vector<std::string>>();
    }
    int words_per_process = int(wordlist.size()/processes_count);
    std::cout << "Loading wordlist with " << wordlist.size() << " paths\n";
    int offset = 0;
    for (int i = 0; i < processes_count; ++i) {
        if (i == processes_count-1){
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.end()); 
        } else {
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.begin() + (offset+words_per_process)); 
        }
        offset += words_per_process;
    }
    std::cout << "Prepared " << words_per_process << " words per process, and " << wordlists.size() << " processes in total\n";
    std::cout << words_per_process << " words per process\n";
    return wordlists;
}

std::vector<std::vector<std::string>> scan_host(std::string host, int port, std::string path, 
        std::vector<std::string> wordlist, int process_id) {
    // <summary>
    // scans host word by word from the wordlist using get_code function on each word and yield findings
    // </summary>
    if(path.back() == '/') {
        path.pop_back();
    }
    if(path.at(0) == '/'){
        path.erase(path[0]);
    }
    std::vector<std::vector<std::string>> ret;
    for(std::string word : wordlist) {
        if(word.at(0) != '/'){
            word = "/" + word;
        }
        int code = get_code(host, port, path+word);
        if(code != 404) {
            ret.push_back(
                    std::vector<std::string> {
                        host + ":" + std::to_string(port) + "/" + path + word , 
                        std::to_string(code)}
                    );
            std::cout << host << ":" << port << "/" << path << word << " returned " << code << std::endl;
        }
    }
    return ret;
}

int start_scan(std::string url, std::string wordlist_path) {
    try {
        std::cout << "TEST TEST TEST TEST!\n";
        std::vector<std::string> parsed_url = parse_url(url);
        std::string host = parsed_url[0];
        int port = std::stoi(parsed_url[1]);
        std::string path = parsed_url[2];
        std::vector<std::vector<std::string>> wordlists = prepare_wordlists(wordlist_path);
        std::vector<std::vector<std::string>> findings = scan_host(host, port, path, wordlists[0], 1);
        
        std::ofstream report; 
        report.open("cpp_generated_report.txt");
        report << url << std::endl << wordlist_path << std::endl;
        
        for(std::vector<std::string> finding : findings) {
            report << finding[0] << " " << finding[1] << std::endl;
        }
        report.close();
    } catch (...) {
        std::cout << "EXCEPTION!\n";
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
