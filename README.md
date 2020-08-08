# dr.buster
Simple, yet effective web path finder implemented with multiprocessing in Python


### Usage
```
$ python3 dr.buster.py https://example.com /home/user/wordlist.txt

starts scan on example.com with wordlist wordlist.txt
and generates report dr.buster.report.$datetime$
```

##### Example output when started with words.txt on testserver
![alt text](res/usage.png)

### Test it out on testserver
```
$ pip3 install flask
$ export FLASK_APP=testserver.py
$ python3 -m flask run

and run dr.buster in another terminal
```

## ~~TODO:~~
* [x] - add argsparse
* [x] - add support for dirbusting on specific path

##### DISCLAMER: I AM NOT RESPONSIBLE FOR ANY ACTIONS DONE WITH THIS SCRIPT, USE IT ONLY IF YOU HAVE PERMISSION
