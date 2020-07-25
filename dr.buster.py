import socket, requests, itertools
from time import time
from sys import exit
from os.path import exists
from multiprocessing import cpu_count, Process

CPU_CORES = cpu_count()
WORD_LISTS = []
WORDLIST_PATH = "./words.txt"
FINDINGS = []
URL = ""

def parse_url(url):
    print("Validating url %s" % (url, ))

def prepare_wordlists(path):
    global WORD_LISTS, LOADED_WORDS
    lines = []
    print("Loading words from %s" % (path,))
    if exists(WORDLIST_PATH):
        lines = [line.rstrip() for line in open(path)]
    else:
        print("ERR: wordlist not found!")
        exit(1)
    
    print("Loaded %s words" % (len(lines), ))
    words_per_process = int(len(lines)/CPU_CORES)
    start = 0
    print("Detected %s cores on this system" % (CPU_CORES, ) )
    print("Loading %s words per process" % (words_per_process, ))
    for c in range(CPU_CORES):
        if c == CPU_CORES - 1:
            WORD_LISTS.append(lines[start:])
        else:
            WORD_LISTS.append(lines[start:start+words_per_process])
        start+=words_per_process
        print("process %s ready, loaded %s words" % (c+1, len(WORD_LISTS[c])))

def scan_url(url, wordlist, process_id=None):
    if not url.endswith('/'):
        url+="/"
    for n, word in enumerate(wordlist):
        requests.get(url + word) # IMPLEMENT WITH RAW SOCKETS
        if process_id:
            print("PROCESS [%s] - scanning %s\r" % (process_id, url+word), end="")
        else:
            print("Scanning %s\r" % (url+word, ), end="")


def start_scan(url):
    print("Starting scan on %s" % (url,))
    procs = []
    for n, wordlist in enumerate(WORD_LISTS):
        procs.append(Process(target=scan_url, args=(url,wordlist,n+1)))
    start_time = time()
    for p in procs:
        p.start()
    for p in procs:
        p.join()
    end_time = time()
    print()
    print("Scanned %s paths in %s s." % (len(list(itertools.chain.from_iterable(WORD_LISTS))), end_time-start_time))
    
def generate_report():
    pass

if __name__ == '__main__':
    print("Starting Dr.buster..")
    URL = "http://localhost:5000/"
    parse_url(URL)
    prepare_wordlists(WORDLIST_PATH)
    start_scan(URL)
    generate_report()
