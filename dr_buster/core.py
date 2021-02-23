#!/usr/bin/env python3

import itertools
from socket import socket, gaierror, AF_INET, SOCK_STREAM
from ssl import wrap_socket, SSLError, PROTOCOL_TLSv1_2
from time import time
from datetime import datetime
from sys import exit
from os.path import exists
from multiprocessing import cpu_count, Process

PROCESSES_COUNT = 32 if cpu_count() <= 4 else 64
WORD_LISTS = []
WORDLIST_PATH = ""
URL = ""
SSL_SUPPORTED = True
TIME = datetime.now().strftime("%d-%m-%Y_%H-%M-%S")
NOT_FOUND_CODE = 404

def get_code(host, port, path):
    global SSL_SUPPORTED
    s = socket(AF_INET, SOCK_STREAM)
    response = None
    if path.startswith("/"):
        path = path.lstrip('/')
    
    if SSL_SUPPORTED:
        s = wrap_socket(s, ssl_version=PROTOCOL_TLSv1_2)
    try:
        s.connect((host, port))
    except (gaierror, TimeoutError):
        print("Name or service not known!")
        exit(1)
    except SSLError:
        print(
                "%s doesnt seem to support TLSv1. \nI'm trying http..."
                % ("https://"+ host + ":" + str(port) + "/" + path, ))
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
    
    if path.endswith("\n"):
        path = path.rstrip("\n")

    request = "GET /%s HTTP/1.1\r\nHost:%s\r\n\r\n" % (path,host)
    s.send(request.encode())  
    response = s.recv(12)
    code = int(repr(response.decode()).split()[1].rstrip("'"))
    s.close()
    return code

def parse_url(url):
    global SSL_SUPPORTED, NOT_FOUND_CODE
    print("Validating url %s" % (url, ))
    https = False
    host = None
    port = None
    path = ""
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
        path = '/'.join(url.split('/')[1:])
    except Exception:
        print("Cant parse url!")
        exit(1)

    print("Initial GET to see if host is up") 
    c = get_code(host, port, ".")
    print("[UP] => got %s" % (c,))
    print("Requesting path /aaaabbbb2 to set NOT_FOUND_CODE.")
    print("Some sites dont have 404 for not found, but rather retirect to the homepage if path doesnt exist")
    NOT_FOUND_CODE = get_code(host, port, path + "/aaaabbbb2")
    print("NOT_FOUND_CODE is %s" % (NOT_FOUND_CODE,))
    return (host, port, path)

def prepare_wordlists(path):
    global WORD_LISTS
    lines = []
    print("Loading words from %s" % (path,))
    if exists(path):
        try:
            lines = [line.rstrip() for line in open(path)]
        except Exception as e:
            print("ERR: problems parsing wordlist")
            exit(1)
    else:
        print("ERR: wordlist not found!")
        exit(1)
    
    print("Loaded %s words" % (len(lines), ))
    words_per_process = int(len(lines)/PROCESSES_COUNT)
    start = 0
    print("Detected %s cores on this system, starting %s processes" % (cpu_count(), PROCESSES_COUNT ))
    print("Loading %s words per process" % (words_per_process, ))
    for p in range(PROCESSES_COUNT):
        if p == PROCESSES_COUNT - 1:
            WORD_LISTS.append(lines[start:])
        else:
            WORD_LISTS.append(lines[start:start+words_per_process])
        start+=words_per_process

def scan_host(host, port, wordlist, process_id=None, path=""):
    for word in wordlist:
        try:
            code = get_code(host, port, path+word)
        except Exception:
            print(host,port,path,word)
            exit(1)
        if code != NOT_FOUND_CODE and code != 400:
            print("%s:%s/%s%s returned [%s]!                \r" 
                    % ("http://"+host if not SSL_SUPPORTED else "https://"+host, port, path, word, code))
            finding = ("%s:%s/%s%s [%s]\n"
                    % ("http://"+host if not SSL_SUPPORTED else "https://"+host, port, path, word, code))
            write_to_report(finding)

def start_scan(url, wordlist_path):
    global WORD_LISTS
    print("Starting scan on %s.." % (url,))
    host, port, path = parse_url(url)
    prepare_wordlists(wordlist_path)
    procs = []
    for n, wordlist in enumerate(WORD_LISTS):
        procs.append(Process(target=scan_host, args=(host, port, wordlist, n+1, path)))
    for p in procs:
        p.start()
    for p in procs:
        p.join()

def write_to_report(finding):
    fname = "./dr.buster.report."+TIME
    with open(fname, "a") as f:
        f.write(finding)

def main(url, wordlist_path):
    URL = url
    WORDLIST_PATH = wordlist_path
    print("Starting Dr.buster..\nURL: %s \nWORDLIST: %s" % (URL, WORDLIST_PATH))
    start_time = time()
    start_scan(URL, WORDLIST_PATH)
    end_time = time()
    print()
    print("\nScanned %s paths in %s s." % (len(list(itertools.chain.from_iterable(WORD_LISTS))), end_time-start_time))

