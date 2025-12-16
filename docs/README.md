# README

## Brief
My redux of a toy C++ HTTP/1.x server, but it focuses on event-driven handling: It will use socket polling & `std::async` for 'non-blocking' I/O operations.

## References
 - [Beej Sockets Chapter 5](https://beej.us/guide/bgnet/html/split/system-calls-or-bust.html)
    - Important BSD socket API notes
 - [HTTP Made Really Easy](https://jmarshall.com/easy/http/#structure)
    - Crash course of HTTP/1.x basics

## Basic Demonstration
<img src="imgs/Derk_Httpd_New_Page.png" alt="test page with text echoing" height="50%" width="50%">

## Roadmap
 - Add support for `If-Unmodified-Since` header & properly handling it.
 - Handle "absolute host" URIs... Maybe match the `host:port` segment against the `Host` and server-set address info.
 - Test GET query param handling.
 - Add colored logging.
