#ifndef THINGER_IO_HPP
#define THINGER_IO_HPP

namespace thinger {

    class thinger_io {
    public:
        thinger_io(){}
        virtual ~thinger_io(){}

    public:
        virtual bool read(char *buffer, size_t size) = 0;
        virtual bool write(const char *buffer, size_t size, bool flush = false) = 0;
    };
}

#endif
