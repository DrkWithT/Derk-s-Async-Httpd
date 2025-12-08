#!/bin/zsh

# curl -i --output - http://localhost:8080/ -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: keep-alive" && curl -i --output - http://localhost:8080/index.js -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: close" || echo "\033[1;32mDemo is DONE\033[0m";

curl -v -X POST --output - http://localhost:8080/ -H "Content-Type: text/plain" -H "Connection: close" -d "hello world!" || echo "\033[1;32mDemo is DONE\033[0m";
