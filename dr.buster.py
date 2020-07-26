import itertools
from socket import socket, gaierror
from socket import AF_INET, SOCK_STREAM
from ssl import wrap_socket, SSLError
from ssl import PROTOCOL_TLSv1
from time import time
from datetime import datetime
from sys import exit
from os.path import exists
from multiprocessing import cpu_count, Process

CPU_CORES = cpu_count()
WORD_LISTS = []
WORDLIST_PATH = "./words.txt"
URL = "http://localhost:5000"
SSL_SUPPORTED = True

def get_code(host, port, path):
    global SSL_SUPPORTED
    s = socket(AF_INET, SOCK_STREAM)
    response = None
    if SSL_SUPPORTED:
        s = wrap_socket(s, ssl_version=PROTOCOL_TLSv1)
    try:
        s.connect((host, port))
    except (gaierror, TimeoutError):
        print("Name or service not known!")
        exit(1)
    except SSLError:
        print("%s doesnt seem to support TLSv1. \nI'm trying http..." % (url, ))
        s = socket(AF_INET, SOCK_STREAM)
        try:
            s.connect((host, port))
        except Exception:
            print("Not working")
            exit(1)
        print("It worked.. I'm continuing with http requests")
        SSL_SUPPORTED = False
    except ConnectionRefusedError:
        print("Host seems down..")
        exit(1)

    request = "GET /%s HTTP/1.1\r\nHost:%s\r\n\r\n" % (path,host)
    s.send(request.encode())  
    response = s.recv(12)
    code = int(repr(response.decode()).split()[1].rstrip("'"))
    s.close()
    return code

def parse_url(url):
    global SSL_SUPPORTED
    print("Validating url %s" % (url, ))
    host = None
    port = None
    if not url.endswith('/'):
        url+="/"
    try:
        if url.startswith("https"):
            url = url.split("//")[1]
            https = True
        elif url.startswith("http"):
            SSL_SUPPORTED = False
            url = url.split("//")[1]
        if ":" in url:
            host, port_path = url.split(":")
            port = int(port_path.split("/")[0])
        else:
            host = url.split("/")[0]
            port = 443 if https else 80
    except Exception:
        print("Cant parse url!")
        exit(1)

    print("Initial GET to see if host is up") 
    get_code(host, port, ".")
    print("[OK]")
    return (host, port)

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

def scan_host(host, port, wordlist, process_id=None):
    for n, word in enumerate(wordlist):
        code = get_code(host, port, word)
        if code != 404:
            print("%s:%s/%s returned [%s]!                \r" 
                    % ("http://"+host if not SSL_SUPPORTED else "https://"+host, port, word, code))
            finding = ("%s:%s/%s [%s]\n"
                    % ("http://"+host if not SSL_SUPPORTED else "https://"+host, port, word, code))
            write_to_report(finding)
        if process_id:
            print("PROCESS [%s] - scanning %s:%s/%s\r" % (process_id, host, port, word), end="")
        else:
            print("Scanning %s\r" % (url+word, ), end="")

def start_scan(url):
    print("Starting scan on %s.." % (url,))
    host, port = parse_url(URL)
    prepare_wordlists(WORDLIST_PATH)
    procs = []
    for n, wordlist in enumerate(WORD_LISTS):
        procs.append(Process(target=scan_host, args=(host, port, wordlist, n+1)))
    for p in procs:
        p.start()
    for p in procs:
        p.join()

def write_to_report(finding):
    fname = "./dr.buster.report."+datetime.now().strftime("%d.%m.%Y_%H")
    with open(fname, "a") as f:
        f.write(finding)

if __name__ == '__main__':
    print("Starting Dr.buster..")
    start_time = time()
    start_scan(URL)
    end_time = time()
    print()
    print("Scanned %s paths in %s s." % (len(list(itertools.chain.from_iterable(WORD_LISTS))), end_time-start_time))


