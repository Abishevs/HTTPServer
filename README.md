# Simple HTTP Server in C 

Trying to learn how a HTTP server works with unix system calls.

Right now it can server a static webpage and thats good enough for me for this
project. 

I did kinda play around with HTTP Methods but I did end up using only GET.

## As of features
* Default port is `127.0.0.1:6969` aka `localhost`
* Can serve in `./` or in specified `dir` (realativ path)
* `200 OK` On valid request
* `404 Not Found` if resource not found.
* `400 Bad Request` if invalid header (if invalid HTTP Method)
* `501 Not Implemented` if POST method recieved.
* A Example test page in `static/` dir.

![](https://github.com/Abishevs/HTTPServer/blob/master/static/images/server_cli.png) 

## Getting Started

These instructions will get you a copy of the project up and running on your
local machine.

### Prerequisites

* make 
* gcc 
* Terminal

### Installation

1. Clone

```bash
git clone https://github.com/Abishevs/HTTPServer.git
cd HTTPServer 
make
```

### Usuage 
Test it with a example page
```bash
./build/server static/
```

## License
This project is under MIT license
