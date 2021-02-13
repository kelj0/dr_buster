#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>

int NOT_FOUND_CODE = 404;

int get_code(const std::string host, int port, std::string path) {
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    int sock = 0;
    struct sockaddr_in serv_addr = {0};
    char buffer[14] = {0};
    const std::string request = "GET /" + path + " HTTP/1.1\r\nHost:" + host + "\r\n\r\n";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80); // TODO add https support
    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <=0 ) {
        addrinfo hints = {};
        hints.ai_flags = AI_CANONNAME;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        addrinfo *res;
        if (getaddrinfo(host.c_str(), NULL, &hints, &res) != 0) {
            std::cerr << "ERR: unable to resolve " << host << " to IP" << std::endl;
            exit(-1);
        } else {
            inet_aton(reinterpret_cast<const char *>(res->ai_addr), &serv_addr.sin_addr);
            freeaddrinfo(res);
        }
        std::cout << "Converted " << host << " to " << inet_ntoa(serv_addr.sin_addr) << std::endl;
    }


    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed" << std::endl;
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
    std::cout << "Getting number of cores..." << std::endl;
    const int cores = std::thread::hardware_concurrency();
    std::cout << "Detected " << cores << " cores on this system" << std::endl;
    return cores;
}

std::vector<std::string> parse_url(std::string url) {
    // <summary>
    // parses given url and returns a pointer to the array of host, port and path
    // </summary>
    std::string tmp = "";
    std::string host = "";
    int port = 0;
    std::string path = "";
    try {
        if (url.find("//") == std::string::npos) {
            url = "http://" + url;
        }
        if (url[url.length()-1] != '/'){
            url = url + "/";
        }
        tmp = url.substr(url.find("//") + 2, url.length());
        if (tmp.find(":") != std::string::npos) {
            host = tmp.substr(0, tmp.find(":"));
            port = stoi(tmp.substr(tmp.find(":") + 1, tmp.find("/")));
        } else {
            host = tmp.substr(0, tmp.find("/"));
            port = 80;
        }
        if (tmp.find("/") != std::string::npos) {
            path = tmp.substr(tmp.find("/") + 1, tmp.length());
        } else {
            path = "";
        }
    } catch (std::exception &e) {
        std::cerr << "ERR: at parsing url [" << e.what() << "]" << std::endl;
        exit(-1);
    }
    std::cout << "Initial GET to see if host is up" << std::endl;
    int c = get_code(host, port, "");
    if (c != -1) {
        std::cout << "[UP] => got " << std::to_string(c) << std::endl;
        std::cout << "Requesting path /aaacabbbb2 to set NOT_FOUND_CODE." << std::endl;
        std::cout << "Some sites dont have 404 for not found, but rather retirect to the homepage if path doesnt exist" << std::endl;
        NOT_FOUND_CODE = get_code(host, port, "aaacabbbb2");
        std::cout << "NOT_FOUND_CODE is "  << std::to_string(NOT_FOUND_CODE) << std::endl;
    } else {
        exit(-1);
    }
    return std::vector<std::string> { host, std::to_string(port), path };
}

std::vector<std::vector<std::string>> prepare_wordlists(std::string path) {
    // <summary>
    // loads words from the wordlists in vector<vector<string>> where number of
    // vectors in vector depends on a CPU cores
    // </summary>
    std::cout << "Preparing wordlists.." << std::endl;
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
        std::cerr << "ERR: Cant open wordlist!\n";
        exit(-1);
    }
    int words_per_process = int(wordlist.size()/processes_count);
    std::cout << "Loading wordlist with " << wordlist.size() << " paths" << std::endl;
    int offset = 0;
    for (int i = 0; i < processes_count; ++i) {
        if (i == processes_count-1){
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.end());
        } else {
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.begin() + (offset+words_per_process));
        }
        offset += words_per_process;
    }
    std::cout << "Prepared " << words_per_process << " words per process, and " << wordlists.size() << " processes in total" << std::endl;
    std::cout << words_per_process << " words per process" << std::endl;
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
        std::cout << "Initiating the scan" << std::endl;
        std::vector<std::string> parsed_url = parse_url(url);
        std::string host = parsed_url[0];
        int port = std::stoi(parsed_url[1]);
        std::string path = parsed_url[2];
        std::vector<std::vector<std::string>> wordlists = prepare_wordlists(wordlist_path);
        std::vector<std::vector<std::string>> findings = scan_host(host, port, path, wordlists[0], 1);
        std::cout << "Done with scans, writing a report" << std::endl;
        std::ofstream report;
        report.open("cpp_generated_report.txt");
        report << url << std::endl << wordlist_path << std::endl;

        for(std::vector<std::string> finding : findings) {
            report << finding[0] << " " << finding[1] << std::endl;
        }
        report.close();
    } catch (std::exception &e) {
        std::cerr << "ERR: " << e.what();
        exit(-1);
    }
    return 0;
}

void print_help() {
    std::cout << "dr_buster help: " << std::endl;
    std::cout << "run: ./dr_buster <URL> <PATH/TO/WORDLIST>" << std::endl;
    std::cout << "\tURL:  URL/IP of server you want to scan" << std::endl;
    std::cout << "\tPATH: path to wordlist you want to use for a scan" << std::endl;
}

void print_banner() {
    std::cout << R"(=====================================================)" << std::endl;
    std::cout << R"(|     | |       | |                | |              |)" << std::endl;
    std::cout << R"(|   __| | _ __  | |__   _   _  ___ | |_  ___  _ __  |)" << std::endl;
    std::cout << R"(|  / _` || '__| | '_ \ | | | |/ __|| __|/ _ \| '__| |)" << std::endl;
    std::cout << R"(| | (_| || |    | |_) || |_| |\__ \| |_|  __/| |    |)" << std::endl;
    std::cout << R"(|  \__,_||_|    |_.__/  \__,_||___/ \__|\___||_|    |)" << std::endl;
    std::cout << R"(=====================================================)" << std::endl;
}

int main(int argc, char* argv[]) {
    print_banner();
    /*if (argc != 3){
        print_help();
        return -1;
    }*/
    start_scan("localhost", "argv[2]"); // testing

    return 0;
}

