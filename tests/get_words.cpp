#include <fstream>
#include <vector>

int main() {
    int NUMBER_OF_CORES = 4;
    int processes_count = 0;
    if (NUMBER_OF_CORES <= 4){
        processes_count = 32;
    } else {
        processes_count = 64;
    }

    std::ifstream file("./wordlist.txt");
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
        printf("ERR: Cant open wordlist!\n");
        return 0;
    }
    int words_per_process = int(wordlist.size()/processes_count);
    int offset = 0;
    for (int i = 0; i < processes_count; ++i) {
        if (i == processes_count-1){
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.end()); 
        } else {
            wordlists[i] = std::vector<std::string>(wordlist.begin() + offset, wordlist.begin() + (offset+words_per_process)); 
        }
        offset += words_per_process;
    }
    printf("Number of processes: %d\nwordlists %d\n", processes_count, wordlists.size());
    for(std::vector<std::string> a: wordlists) {
        printf("Words per proc => %d\n", a.size());
    }
    return 0;
}
