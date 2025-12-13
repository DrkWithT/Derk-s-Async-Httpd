#!/bin/zsh

curl -v -X POST --output - http://localhost:8080/ -H "Content-Type: text/plain" -H "Connection: close" -d "dis new http server is cooking, no cap" || echo "\033[1;32mDemo is DONE\033[0m";
