#!/bin/zsh

curl -v -X POST --output - http://localhost:8080/ -H "Content-Type: text/plain" -H "Connection: close" -H "Transfer-Encoding: chunked" --data-binary "5\r\nhello\r\n5\r\nworld\r\n0\r\n" || echo "\033[1;32mDemo is DONE\033[0m";
