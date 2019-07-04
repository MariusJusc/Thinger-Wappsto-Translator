Qplus - IoT.seluxit C++ client
===================================================

[![Build Status](https://travis-ci.org/Seluxit/qplus.svg?branch=master)](https://travis-ci.org/Seluxit/qplus) 

Copyright 2017 Seluxit A/S.

[https://iot.seluxit.com][iot.seluxit]

[iot.seluxit]: https://iot.seluxit.com

Intro
-----------------------

Qplus is a C++ client for communication with *Seluxit cloud*. Before you run client you need to be a register user and
download certificates from [https://iot.seluxit.com][iot.seluxit]. Downloaded certificates can be places under 'certs' directory or
provide explicit path using configuration file `qplus.conf` where those certificates are stored. Besides provided
certificates to each registered user will be assigned 'network' ID. Is is 128-bit UUID and it should be placed as a
directory under 'network'. See example provided in the source code. 
Generation of devices, values and states can be done using `generate_data.sh` script.

Installation of dependencies on Linux 
-----------------------

To build Qplus, the following tools/libraries are needed:

  * g++ - C++ compiler with c++11 support: g++4.8 and above.
  * [CMake][cmake-org] - cross platform tool for building Qplus  
  * libev - high performance event loop - event library.
  * openssl - tool for Transport Layer Security (TLS) and Secure Sockets Layer (SSL) protocols. 

[cmake-org]: https://cmake.org/

On Ubuntu, you can install them with:

    $ sudo apt-get install cmake libev-dev openssl 


## Build 

For building Qplus you need to run `build.sh` script. It will create 'build' directory for temporary [CMake][cmake-org] 
files, will run 'cmake' for configuration and 'make' to build the Qplus. 
Once build is finished it should produce `qplus` executable. Executable will read 'network' directory structure with all
devices, values and states, wrap such data in json format and send to Q server. It also will listen on event from Q
server for controlling/updating devices and states. All communication is secure. 

## License

See the `LICENSE` file for details. In summary, Qplus is licensed under the
MIT license, or public domain if desired and recognized in your jurisdiction.
