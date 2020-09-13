import argparse
import itertools
from sys import exit, argv
from time import time

from dr_buster.core import start_scan, WORD_LISTS

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument("url", help="Url of web page you want to scan")
    p.add_argument("wordlist", help="Path to wordlist")
    if len(argv) != 3:
        p.print_help()
        exit(1)
    a = p.parse_args()
    URL=a.url
    WORDLIST_PATH=a.wordlist
    print("Starting Dr.buster..\nURL: %s \nWORDLIST: %s" % (URL, WORDLIST_PATH))
    start_time = time()
    start_scan(URL, WORDLIST_PATH)
    end_time = time()
    print()
    print("\nScanned %s paths in %s s." % (len(list(itertools.chain.from_iterable(WORD_LISTS))), end_time-start_time))