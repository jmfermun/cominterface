ComInterface
============

ComInterface is a c++ library for use the following communication interfaces:
- Serial Port.
- TCP/IP Socket.

The interfaces are implemented using boost::asio, so it is cross platform.

It implements two types of read and write operations:
- Blocking operations with timeout.
- Non blocking operations.

License
-------

The ComInterface library is distributed under the terms of the MIT License.

Requirements
------------

* C++ compiler.
* Boost C++ libraries.

Installation
------------

ComInterface is configured to build using CMake system in version 2.8+.
