#!/bin/zsh

curl -i -X GET --output - http://localhost:8080/lorem -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: close" -H "If-Modified-Since: Mon, 16 Dec 2025 23:59:59 GMT" || echo "\033[1;32mDemo is DONE\033[0m";
