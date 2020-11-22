Modified from https://github.com/foxweb/pico

# Client
### Build
```
cd client
make
```
### Send request
```
./client [# of threads] [# of requests per thread] [host] [port] [Method] [optional: file list]
```
Example: 
```
./client 2 2 localhost 8000 GET
```

By default, file list's name is `filelist.txt`.
It needs to have number of entries in the first line
Example:
```
2
NAVER.html
test.txt
```

If you want to see the content of the received file, change `line 13` of `client.c`
```
int debug_mode = FALSE;
```

# Server
### Build
```
cd server
make
```
### Start server
Default port is `8000` (if not specified)
```
./server [port]
```


# Execution example
Server
```
 ~/Desktop/parallel_2/server git:(master*) ./server
Server started http://127.0.0.1:8000
 + [GET] /txt?NAVER.html
 + [GET] /txt?NAVER.html
 + [GET] /txt?test.txt
 + [GET] /txt?test.txt
```

Client
```
 ~/desktop/parallel_2/client git:(master*) ./client 2 2 localhost 8000 GET
Request: GET /txt?NAVER.html HTTP/1.0
Request: GET /txt?NAVER.html HTTP/1.0
[203382784d] Total bytes received: 205312
Request: GET /txt?test.txt HTTP/1.0
[203919360d] Total bytes received: 205312
Request: GET /txt?test.txt HTTP/1.0
[203382784d] Total bytes received: 162
[203919360d] Total bytes received: 162
** Total received: 410948 **
```
