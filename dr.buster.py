import socket, ssl, itertools
from time import time
from sys import exit
from os.path import exists
from multiprocessing import cpu_count, Process

CPU_CORES = cpu_count()
WORD_LISTS = []
WORDLIST_PATH = "./words.txt"
FINDINGS = []
URL = ""

def get(url):
    host = None
    port = None
    path = None
    https = False
    try:
        if url.startswith("https"):
            url = url.split("//")[1]
            https = True
        elif url.startswith("http"):
            url = url.split("//")[1]
        if ":" in url:
            host, port_path = url.split(":")
            port = int(port_path.split("/")[0])
            path = "/".join(port_path.split("/")[1:])
        else:
            host = url.split("/")[0]
            path = "/".join(url.split("/")[1:])
    except Exception:
        print("Cant parse url!")
        exit(1)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    response = None
    if https:
        s = ssl.wrap_socket(s, ssl_version=ssl.PROTOCOL_TLSv1)
        port = port if port else 443
    else:
        port = port if port else 80
    try:
        s.connect((host, port))
    except (socket.gaierror, TimeoutError):
        print("Name or service not known!")
        exit(1)
    except ssl.SSLError:
        print("%s doesnt seem to support TLSv1 (ELI5 dont use https in url!)" % (url, ))
        exit(1)
    except ConnectionRefusedError:
        print("Host seems down..")
        exit(1)

    request = "GET /%s HTTP/1.1\r\nHost:%s\r\n\r\n" % (path,host)
    s.send(request.encode())  
    response = s.recv(12)
    http_response = repr(response.decode())
    #print(http_response.split())
    http_response_len = len(http_response)
    s.close() 

def parse_url(url):
    print("Validating url %s" % (url, ))
    
def prepare_wordlists(path):
    global WORD_LISTS
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
        get(url + word)
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
    print("Initial GET to see if host is up") 
    get(URL)
    prepare_wordlists(WORDLIST_PATH)
    start_scan(URL)
    generate_report()
