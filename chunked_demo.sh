#!/bin/zsh

curl -v -X GET --output - http://localhost:8080/lorem -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: close" || echo "\033[1;32mDemo is DONE\033[0m";
