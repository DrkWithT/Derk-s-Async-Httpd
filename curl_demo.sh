#!/bin/zsh

curl -i http://localhost:8080/ -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: keep-alive" && curl -i http://localhost:8080/foo -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: close" || echo "\033[1;32mDemo is DONE\033[0m";
