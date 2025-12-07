#!/bin/zsh

curl -v http://localhost:8080/ -H "Content-Type: */*" -H "Content-Length: 0" -H "Connection: close" || echo "Request failed, check server logs.";
