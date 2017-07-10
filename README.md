ComInterface
============

ComInterface is a library for use the following communication interfaces:
- Serial Port.
- TCP/IP Socket.

The Socket interface can be used to communicate through TCP/IP in a Serial Port manner. It can be used as client or server.

The interfaces are implemented using boost::asio, so it is cross platform.

It implements two types of read and write operations:
- Blocking operations with timeout (Read / Write): when the indicated number of data bytes are received or sent, or the timeout is reached, the function returns with the number of received or sent data bytes.
- Non blocking operations (ReadSome / WriteSome): if data can't be immediately read or sent, the function returns with 0, otherwise with the number of received or sent data bytes.

Requirements
------------

* C++ compiler.
* Boost C++ libraries.

Installation
------------

ComInterface is configured to build using CMake system in version 2.8+.
