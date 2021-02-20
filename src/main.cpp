#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <condition_variable>

int NOT_FOUND_CODE = 404;
bool converted_to_ip = false;
const char *target_ip = "";
int SYSTEM_THREADS;

std::mutex m;
std::condition_variable cv;
bool ready = false;

int get_code(const std::string& host, int port, const std::string& path) {
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    int sock;
    char buffer[14] = {0};
    const std::string request = "GET /" + path + " HTTP/1.1\r\nHost:" + host + "\r\n\r\n";
    struct sockaddr_in serv_addr = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        exit(-1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); // TODO add https support
    int valid_ip = 1;
    if (!converted_to_ip) {
        converted_to_ip = true;
        valid_ip = inet_pton(AF_INET, host.c_str(), &(serv_addr.sin_addr));
        if (valid_ip == 1) {
            target_ip = (const char *) host.c_str();
            inet_pton(AF_INET, target_ip, &(serv_addr.sin_addr));
        }
    } else {
        inet_pton(AF_INET, target_ip, &(serv_addr.sin_addr));
    }

    if (valid_ip == 0) {
        addrinfo hints = {};
        hints.ai_flags = AI_CANONNAME;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        addrinfo *res;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0) {
            std::cerr << "ERR: unable to resolve " << host << " to IP" << std::endl;
            exit(-1);
        } else {
            inet_aton(reinterpret_cast<const char *>(res->ai_addr), &serv_addr.sin_addr);
            target_ip = inet_ntoa(serv_addr.sin_addr);
            freeaddrinfo(res);
        }
        std::cout << "Converted " << host << " to " << inet_ntoa(serv_addr.sin_addr) << std::endl;

    } else if (valid_ip == -1) {
        std::cout << "Invalid IPv4 IP provided." << std::endl;
        exit(-1);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed" << std::endl;
        return -1;
    }
    send(sock, request.c_str(), request.size(), 0);
    read(sock, buffer, 14);
    close(sock);
    std::string str_code;
    for(int j = 9; j<12;++j){
        str_code.push_back(buffer[j]);
    }
    int code = std::stoi(str_code);
    return code;
}

std::vector<std::string> parse_url(std::string url) {
    // <summary>
    // parses given url and returns a pointer to the array of host, port and path
    // </summary>
    std::string tmp;
    std::string host;
    int port;
    std::string path;
    try {
        if (url.find("//") == std::string::npos) {
            url = "http://" + url;
        }
        if (url[url.length()-1] != '/'){
            url = url + "/";
        }
        tmp = url.substr(url.find("//") + 2, url.length());
        if (tmp.find(':') != std::string::npos) {
            host = tmp.substr(0, tmp.find(':'));
            port = stoi(tmp.substr(tmp.find(':') + 1, tmp.find('/')));
        } else {
            host = tmp.substr(0, tmp.find('/'));
            port = 80;
        }
        if (tmp.find('/') != std::string::npos) {
            path = tmp.substr(tmp.find('/') + 1, tmp.length());
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

std::vector<std::vector<std::string>> prepare_wordlists(const std::string& path) {
    // <summary>
    // loads words from the wordlists in vector<vector<string>> where number of
    // vectors in vector depends on a CPU cores
    // </summary>
    std::cout << "Preparing wordlists.." << std::endl;
    std::ifstream file(path);
    std::vector<std::vector<std::string>> wordlists(SYSTEM_THREADS);
    std::vector<std::string> wordlist;
    if (file.is_open()){
        std::string line;
        while(std::getline(file, line)) {
            if (!line.empty()) {
                wordlist.push_back(line);
            }
        }
        file.close();
    } else {
        std::cerr << "ERR: Cant open a wordlist!\n";
        file.close();
        exit(-1);
    }
    int words_per_process = int(wordlist.size()/SYSTEM_THREADS);
    std::cout << "Loading wordlist with " << wordlist.size() << " paths" << std::endl;
    int offset = 0;
    for (int i = 0; i < SYSTEM_THREADS; ++i) {
        if (i == SYSTEM_THREADS - 1) {
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.end());
        } else {
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.begin() + (offset+words_per_process));
        }
        offset += words_per_process;
    }
    std::cout << "Prepared " << words_per_process << " words per process, and " << wordlists.size() << " processes in total" << std::endl;
    return wordlists;
}

void scan_host(const std::string& host, int port, std::string path, const std::vector<std::string>& wordlist, std::ostream &report) {
    // <summary>
    // scans host word by word from the wordlist using get_code function on each word and yield findings
    // </summary>
    if (path.back() == '/') {
        path.pop_back();
    }
    if ( path.length() != 0 && path.at(0) == '/'){
        path.erase(path[0]);
    }
    for (const std::string& word : wordlist) {
        int code = get_code(host, port, path + word);
        if (code != 404) {
            std::cout << "http://" << host << ":" << port << path << "/" << word << " returned " << code << std::endl;
            report << "http://" << host << ":" << port << path << "/" << word << " [" << code << "]" << std::endl;
        }
    }

    std::unique_lock<std::mutex> lk(m);
    ready = true;
    std::notify_all_at_thread_exit(cv, std::move(lk));
}

int start_scan(const std::string& url, const std::string& wordlist_path) {
    try {
        std::cout << "Initiating the scan" << std::endl;
        std::cout << "Getting number of threads available..." << std::endl;
        SYSTEM_THREADS = std::thread::hardware_concurrency();
        if (!SYSTEM_THREADS) {
            SYSTEM_THREADS = 2;
        }
        std::cout << "Detected " << SYSTEM_THREADS << " available threads on this system" << std::endl;
        std::vector<std::string> parsed_url = parse_url(url);
        std::string host = parsed_url[0];
        int port = std::stoi(parsed_url[1]);
        std::string path = parsed_url[2];
        std::vector<std::vector<std::string>> wordlists = prepare_wordlists(wordlist_path);
        std::ofstream report;
        report.open("cpp_generated_report.txt");
        report << url << std::endl << wordlist_path << std::endl;

        std::vector<std::thread> v_threads(SYSTEM_THREADS);
        for (unsigned i = 0; i < SYSTEM_THREADS; ++i) {
            v_threads[i] = std::thread(scan_host, host,port, path, wordlists[i], std::ref(report));
        }
        for (unsigned i = 0; i < SYSTEM_THREADS; ++i) {
            v_threads[i].join();
        }
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []{ return ready;});
        std::cout << "Done with scans, writing a report" << std::endl;
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
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    print_banner();
    if (argc != 3){
        print_help();
        return -1;
    }
    start_scan(argv[1], argv[2]);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Runtime = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
    return 0;
}

