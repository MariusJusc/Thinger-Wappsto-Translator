#ifndef THINGER_RESOURCE_HPP
#define THINGER_RESOURCE_HPP

#include "pson.h"
#include "thinger_message.hpp"

namespace thinger
{

class thinger_resource
{

public:
    // Device input/output type
    enum io_type
    {
        none = 0,            // Requires no parameters (e.g. wakeup)
        run = 1,             // Run a method
        pson_in = 2,         // Takes input parameter
        pson_out = 3,        // Returns output parameter
        pson_in_pson_out = 4 // Takes input and produces output parameter
    };

    // Function visibility
    enum access_type
    {
        PRIVATE = 0,
        PROTECTED = 1,
        PUBLIC = 2,
        NONE = 3
    };

private:
    // used for defining the resource
    io_type io_type_;
    access_type access_type_;

public:
    thinger_resource() : io_type_(none), access_type_(PRIVATE)
    {
    }

    thinger_resource &operator()(access_type type)
    {
        access_type_ = type;
        return *this;
    }

    io_type get_io_type()
    {
        return io_type_;
    }

    static io_type get_io_type_value(int &v)
    {
        switch (v)
        {
        case 1:
            return io_type::run;
        case 2:
            return io_type::pson_in;
        case 3:
            return io_type::pson_out;
        case 4:
            return io_type::pson_in_pson_out;
        default:
            return io_type::none;
        }
    }

    access_type get_access_type()
    {
        return access_type_;
    }

    static access_type get_access_type_value(int &v)
    {
        switch (v)
        {
        case 0:
            return access_type::PRIVATE;
        case 1:
            return access_type::PROTECTED;
        case 2:
            return access_type::PUBLIC;
        default:
            return access_type::NONE;
        }
    }

    void empty_api(protoson::pson_object &content)
    {

        for (protoson::pson_object::iterator it = content.begin(); it.valid(); it.next())
        {
            const char *name = it.item().name();
            auto val = it.item().value();

            if (strcmp("al", name) == 0)
            {
                std::cout << "Access type: " << val.get_value<int>() << std::endl;
            }
            else if (strcmp("fn", name) == 0)
            {
                std::cout << "I/O type: " << val.get_value<int>() << std::endl;
            }
            else
            {
                std::cout << "Something else?" << std::endl;
            }
        }
    }
};
} // namespace thinger
#endif