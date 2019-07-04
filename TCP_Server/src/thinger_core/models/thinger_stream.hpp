#ifndef STREAM_MODEL_HPP
#define STREAM_MODEL_HPP

#include <iostream>
#include "../pson.h"
#include "../thinger_resource.hpp"

using namespace thinger;

class thinger_stream {

public:
    thinger_stream() : stream_id(0), stream_state(PENDING), resource_name("") {}
    ~thinger_stream() {}

    enum state {
        PENDING               = 0, // Pending to be sent
        RESOURCE_REQUESTED    = 1, // Resource has been requested
        RESOURCE_RECEIVED     = 2, // Resource request has been received
        SUBRESOURCE_REQUESTED = 3, // Subresource request sent
        SUBRESOURCE_RECEIVED  = 4, // Subresource requests received
        CONFIRMED             = 5, // Resource confirmed
        PROCESSED             = 6  // Resource processed
    };

    struct subresource {
        std::string name;
        protoson::pson value;
    };

private:
    // Unique identifier
    uint16_t stream_id;

    // State of the stream
    state stream_state;

    // Resource name
    std::string resource_name;

    // Thinger_Resource Access Type
    thinger_resource::access_type resource_access_type = thinger_resource::PRIVATE;

    // Collections of Thinger IO resources
    std::vector<subresource> in;
    std::vector<subresource> out; 

public:
    // GETTERS & SETTERS
    auto get_stream_id()       -> uint16_t& { return stream_id; }
    auto get_stream_id() const -> const uint16_t& { return stream_id; }

    auto get_stream_state()       -> state& { return stream_state; }
    auto get_stream_state() const -> const state& { return stream_state; }

    auto get_resource_name()       -> std::string& { return resource_name; }
    auto get_resource_name() const -> const std::string& { return resource_name; }

    auto get_subresource_in()       -> std::vector<subresource>& { return in; }
    auto get_subresource_in() const -> const std::vector<subresource>& { return in; }

    auto get_subresource_out()       -> std::vector<subresource>& { return out; }
    auto get_subresource_out() const -> const std::vector<subresource>& { return out; }

    auto get_resource_access_type()       -> thinger_resource::access_type& { return resource_access_type; }
    auto get_resource_access_type() const -> const thinger_resource::access_type& { return resource_access_type; }

    // Insert input subresource
    void insert_subresource_in(subresource sub) {
        in.push_back(sub);
    }

    // Insert output subresource
    void insert_subresource_out(subresource sub) {
        out.push_back(sub);
    }
};

#endif