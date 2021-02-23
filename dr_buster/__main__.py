import argparse
from sys import argv
from dr_buster.core import main

if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument("url", help="Url of web page you want to scan")
    p.add_argument("wordlist", help="Path to wordlist")
    if len(argv) != 3:
        p.print_help()
        exit(1)
    a = p.parse_args()
    main(a.url, a.wordlist)
