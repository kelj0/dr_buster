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
const char *target_ip = "";
int SYSTEM_THREADS;
bool supra_mode = false;

int get_code(const std::string& host, int port, const std::string& path) {
    // <summary>
    // makes GET request to the host:port/path and returns status code
    // </summary>
    int sock;
    char buffer[12] = {0};
    const std::string request = "GET /" + path + " HTTP/1.1\r\nHost:" + host + "\r\n\r\n";
    struct sockaddr_in serv_addr = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        exit(-1);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); // TODO add https support
    inet_pton(AF_INET, target_ip, &(serv_addr.sin_addr));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed" << std::endl;
        return -1;
    }
    send(sock, request.c_str(), request.size(), 0);
    read(sock, buffer, 12);
    close(sock);
    int code = std::stoi(std::string() + buffer[9] + buffer[10] + buffer[11]);
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

    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, host.c_str(), &(sa.sin_addr));
    if (result == 0) {
        struct hostent *he = gethostbyname(host.c_str());
        if (he == NULL) {
            std::cerr << "ERR: failed to convert [http://" << host << "] to a valid IPv4 address" << std::endl;
        }
        target_ip = inet_ntoa(*(struct in_addr *) he->h_addr_list[0]);
        std::cout << "Converted " << host << " to " << target_ip << std::endl;
    } else {
        target_ip = host.c_str();
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
    if (supra_mode) {
        std::cout << "Prepared " << words_per_process << " words per thread, and " << wordlists.size()
                  << " threads in total" << std::endl;
    }
    return wordlists;
}

template <typename T1, typename T2>
void scan_host(const std::string host, int port, std::string path, const std::vector<std::string> wordlist, std::ostream &report, bool &ready, T1 m, T2 cv) {
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
    if (typeid(*m).name() == typeid(std::mutex).name()) {
        std::unique_lock<std::mutex> lk(*m);
        ready = true;
        std::notify_all_at_thread_exit(*cv, std::move(lk));
    }
}

void scan_host(const std::string host, int port, std::string path, const std::vector<std::string> wordlist, std::ostream &report) {
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
}

int start_scan(const std::string& url, const std::string& wordlist_path) {
    try {
        std::cout << "Initiating the scan" << std::endl;
        std::cout << "Getting number of threads available..." << std::endl;
        SYSTEM_THREADS = std::thread::hardware_concurrency();
        if (!SYSTEM_THREADS) {
            SYSTEM_THREADS = 2;
        }
        if (supra_mode) {
            std::cout << "Detected " << SYSTEM_THREADS << " available threads on this system" << std::endl;
        }
        std::vector<std::string> parsed_url = parse_url(url);
        std::string host = parsed_url[0];
        int port = std::stoi(parsed_url[1]);
        std::string path = parsed_url[2];
        std::vector<std::vector<std::string>> wordlists = prepare_wordlists(wordlist_path);
        std::ofstream report;
        report.open("cpp_generated_report.txt");
        report << url << std::endl << wordlist_path << std::endl;
        if (supra_mode) {
            std::vector<std::thread> v_threads(SYSTEM_THREADS);
            std::mutex m, m2;
            std::condition_variable cv, cv2;
            bool ready = false;
            std::mutex *ptr_m = &m;
            std::mutex *ptr_m2 = &m2;
            std::condition_variable *ptr_cv = &cv;
            std::condition_variable *ptr_cv2 = &cv2;

            for (unsigned i = 0; i < SYSTEM_THREADS; ++i) {
                if (i == SYSTEM_THREADS - 1) {
                    v_threads[i] = std::thread(
                            static_cast<void (*)(
                                    const std::basic_string<char>,
                                    int,
                                    std::basic_string<char>,
                                    const std::vector<std::basic_string<char>>,
                                    std::basic_ostream<char> &,
                                    bool &,
                                    std::mutex *,
                                    std::condition_variable *)>
                            (&scan_host), host, port, path, wordlists[i], std::ref(report),
                            std::ref(ready), ptr_m, ptr_cv);
                } else {
                    v_threads[i] = std::thread(
                            static_cast<void (*)(
                                    const std::basic_string<char>,
                                    int,
                                    std::basic_string<char>,
                                    const std::vector<std::basic_string<char>>,
                                    std::basic_ostream<char> &,
                                    bool &,
                                    std::mutex *,
                                    std::condition_variable *)>
                            (&scan_host), host, port, path, wordlists[i], std::ref(report),
                            std::ref(ready), ptr_m2, ptr_cv2);
                }
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(i, &cpuset);
                int rc = pthread_setaffinity_np(v_threads[i].native_handle(),
                                                sizeof(cpu_set_t), &cpuset);
                if (rc != 0) {
                    std::cerr << "Error setting thread affinity!" << std::endl;
                }

            }
            for (unsigned i = 0; i < SYSTEM_THREADS; ++i) {
                v_threads[i].join();
            }
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [ready] { return ready; });
        } else {
            for (std::vector<std::string> wordlist: wordlists) {
                scan_host(host, port, path, wordlist, report);
            }
        }
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
    std::cout << "====================================================" << std::endl;
    std::cout << "OPTIONAL: --supra" << std::endl;
    std::cout << "\t enables uber fast mode, utilizing optimal number of threads USE WITH CAUTION!" << std::endl;
    std::cout << "\t I suggest you to use supra mode only on local tests." << std::endl;
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
    if (argc < 3){
        print_help();
        return -1;
    }
    
    if (argc == 4) {
        std::cout << std::string(argv[3]) << std::endl;
        if (argv[3] == "--supra") {
            std::cout << "TEST" << std::endl;
        }
        if (std::string(argv[3]) == "--supra") {
            std::cout << "Achtung! Warning! Enabling the supra mode. " << std::endl;
            supra_mode = true;
            start_scan(argv[1], argv[2]);
        } else if (std::string(argv[1]) == "--supra") {
            std::cout << "Achtung! Warning! Enabling the supra mode. " << std::endl;
            supra_mode = true;
            start_scan(argv[2], argv[3]);
        }    
    } else {
        start_scan(argv[1], argv[2]);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Runtime = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
    return 0;
}

