#ifndef THINGER_DECODER_HPP
#define THINGER_DECODER_HPP

#include "pson.h"
#include "thinger_message.hpp"

namespace thinger{

    class thinger_decoder : public protoson::pson_decoder{
    public:
        bool decode(thinger_message&  message, size_t size){
            size_t start_read = bytes_read();
            while(size-(bytes_read()-start_read)>0) {
                protoson::pb_wire_type wire_type;
                uint32_t field_number;
                if(!pb_decode_tag(wire_type, field_number)) return false;
                switch (wire_type) {
                    case protoson::length_delimited:{
                        uint32_t size = 0;
                        if(!pb_decode_varint32(size)) return false;
                        pb_skip(size);
                    }
                        break;
                    case protoson::varint: {
                        switch (field_number) {
                            case thinger_message::SIGNAL_FLAG:
                            {
                                uint32_t signal_flag = 0;
                                if(!pb_decode_varint32(signal_flag)) return false;
                                message.set_signal_flag((thinger_message::signal_flag)(signal_flag));
                            }
                                break;
                            case thinger_message::STREAM_ID:
                            {
                                uint32_t stream_id = 0;
                                if(!pb_decode_varint32(stream_id)) return false;
                                message.set_stream_id(stream_id);
                            }
                                break;
                            default:
                                if(!pb_skip_varint()) return false;
                                break;
                        }
                        break;
                    }
                    case protoson::pson_type:
                        switch(field_number){
                            case thinger_message::IDENTIFIER:
                                if(!protoson::pson_decoder::decode(message.get_identifier())) return false;
                                break;
                            case thinger_message::RESOURCE:
                                if(!protoson::pson_decoder::decode(message.get_resources())) return false;
                                break;
                            case thinger_message::PAYLOAD:
                                 if(!protoson::pson_decoder::decode(((protoson::pson&) message))) return false;
                                break;
                            default:
                                break;
                        }
                        break;
                    case protoson::fixed_32:
                        if(!pb_skip(4)) return false;
                        break;
                    case protoson::fixed_64:
                        if(!pb_skip(8)) return false;
                        break;
                    default:
                        break;
                }
            }
            return true;
        }
    };

    class thinger_read_decoder : public thinger_decoder{
    public:
        thinger_read_decoder(thinger_io& io) : io_(io)
        {}

    protected:
        virtual bool read(void* buffer, size_t size){
            return io_.read((char*)buffer, size) && protoson::pson_decoder::read(buffer, size);
        }

    private:
        thinger_io& io_;
    };

    class thinger_memory_decoder : public thinger_decoder{

    public:
        thinger_memory_decoder(uint8_t* buffer, size_t size) : buffer_(buffer), size_(size){}

    protected:
        virtual bool read(void* buffer, size_t size){
            if(read_+size<=size_){
                memcpy(buffer, buffer_ + read_, size);
                return protoson::pson_decoder::read(buffer, size);
            }else{
                return false;
            }
        }

    private:
        uint8_t* buffer_;
        size_t size_;
    };

}

#endif