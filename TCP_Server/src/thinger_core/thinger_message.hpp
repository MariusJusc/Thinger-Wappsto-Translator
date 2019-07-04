#ifndef THINGER_MESSAGE_HPP
#define THINGER_MESSAGE_HPP

#include <iostream>
#include "pson.h"

namespace thinger{

    enum message_type {
        MESSAGE             = 1,
        KEEP_ALIVE          = 2
    };

    class thinger_message{

    public:

        // fields for a thinger message (encoded as in protocol buffers)
        enum fields{
            STREAM_ID       = 1,
            SIGNAL_FLAG     = 2,
            IDENTIFIER      = 3,
            RESOURCE        = 4,
            UNUSED1         = 5,
            PAYLOAD         = 6
        };

        // flags for describing a thinger message
        enum signal_flag {
            // GENERAL USED FLAGS
            REQUEST_OK          = 1,    // the request with the given stream id was successful
            REQUEST_ERROR       = 2,    // the request with the given stream id failed

            // SENT BY THE SERVER
            NONE                = 0,    // default resource action: just execute the given resource
            START_STREAM        = 3,    // enable a streaming resource (with stream_id, and resource filled, sample interval (in payload) is optional)
            STOP_STREAM         = 4,    // stop the streaming resource (with stream_id, and resource filled)

            // SENT BY DEVICE
            AUTH                = 5,
            STREAM_EVENT        = 6,    // means that the message data is related to a stream event
            STREAM_SAMPLE       = 7,    // means that the message is related to a periodical streaming sample
            CALL_ENDPOINT       = 8,    // call the endpoint with the provided name (endpoint_id in identifier, value passed in payload)
            CALL_DEVICE         = 9,    // call a given device (device_id in identifier, resource in resource, and value, passed in payload)
            BUCKET_DATA         = 10    // call the bucket with the provided name (bucket_id in identifier, value passed in payload)
        };

    public:

        /**
         * Initialize a default response  message setting the same stream id of the source message,
         * and initializing the signal flag to ok. All remaining data or fields are empty
         */
        thinger_message(thinger_message& other) :
                stream_id(other.stream_id),
                flag(REQUEST_OK),
                identifier(NULL),
                resource(NULL),
                data(NULL),
                data_allocated(false)
        {}

        /**
         * Initialize a default empty message
         */
        thinger_message() :
                stream_id(0),
                flag(NONE),
                identifier(NULL),
                resource(NULL),
                data(NULL),
                data_allocated(false)
        {}

        ~thinger_message(){
            // deallocate identifier
            destroy(identifier, protoson::pool);
            // deallocate resource
            destroy(resource, protoson::pool);
            // deallocate paylaod if was allocated here
            if(data_allocated){
                destroy(data, protoson::pool);
            }
        }

    private:
        /// used for identifying a unique stream
        uint16_t stream_id;
        /// used for setting a stream signal
        signal_flag flag;
        /// used to identify a device, an endpoint, or a bucket
        protoson::pson* identifier;
        /// used to identify an specific resource over the identifier
        protoson::pson* resource;
        /// used to send a data payload in the message
        protoson::pson* data;
        /// flag to determine when the payload has been reserved
        bool data_allocated;

        message_type type;

    public:

        uint16_t get_stream_id(){
            return stream_id;
        }

        message_type get_message_type() {
            return type;
        }

        signal_flag get_signal_flag(){
            return flag;
        }

        bool has_data(){
            return data!=NULL;
        }

        bool has_identifier(){
            return identifier!=NULL;
        }

        bool has_resource(){
            return resource!=NULL;
        }

    public:
        void set_stream_id(uint16_t stream_id) {
            thinger_message::stream_id = stream_id;
        }

        void set_signal_flag(signal_flag const &flag) {
            thinger_message::flag = flag;
        }

        void set_identifier(const char* id){
            if(identifier==NULL){
                identifier = new (protoson::pool) protoson::pson;
            }
            (*identifier) = id;
        }

        void clean_identifier(){
            destroy(identifier, protoson::pool);
            identifier = NULL;
        }

        void clean_resource(){
            destroy(resource, protoson::pool);
            resource = NULL;
        }

        void clean_data(){
            if(data_allocated){
                destroy(data, protoson::pool);
            }
            data = NULL;
        }

        void print_info()
        {
            std::cout << "---------------- RESOURCE INFO ----------------" << std::endl;
            std::cout << "Stream_ID: " << get_stream_id() << std::endl;
            std::cout << "Signal_Flag: " << get_signal_flag() << std::endl;
            std::cout << "Has_Resource: " << has_resource() << std::endl;
            std::cout << "Has_Data: " << has_data() << std::endl;
            std::cout << "--------------------- END ---------------------" << std::endl;
        }

    public:

        void operator=(const char* str){
            ((protoson::pson &) * this) = str;
        }

        operator protoson::pson&(){
            if(data==NULL){
                data = new (protoson::pool) protoson::pson;
                data_allocated = true;
            }
            return *data;
        }

        protoson::pson_array& resources(){
            return (protoson::pson_array&)get_resources();
        }

        protoson::pson_object& data_f(){
            return (protoson::pson_object&) get_data_f();
        }

        protoson::pson& get_resources(){
            if(resource==NULL){
                resource = new (protoson::pool) protoson::pson;
            }
            return *resource;
        }

        protoson::pson& get_identifier(){
            if(identifier==NULL){
                identifier = new (protoson::pool) protoson::pson;
            }
            return *identifier;
        }

        protoson::pson& get_data_f() {
            if(data == NULL) {
                data = new (protoson::pool) protoson::pson;
            }
            return *data;
        }

        protoson::pson& get_data(){
            return *this;
        }

        void set_data(protoson::pson& pson_data){
            if(data==NULL){
                data = &pson_data;
                data_allocated = false;
            }
        }

    };
}

#endif
