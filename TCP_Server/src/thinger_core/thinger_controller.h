#include "pson.h"
#include "thinger_message.hpp"
#include "thinger_encoder.hpp"
#include "thinger_decoder.hpp"
#include "thinger_io.hpp"
#include "thinger_resource.hpp"
#include "models/peer.hpp"
#include "json.hpp" 
#include "wappsto_translator.hpp"
#include "backendsocket.hpp"

#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <random> // std::mt19937, std::uniform_int_distribution

#define LOG(x) std::cout << "[SERVER]: " << x << std::endl;

using json = nlohmann::json;

namespace thinger
{
    using namespace protoson;

    class thinger_controller : public thinger_io
    {
        public:
            thinger_controller() : encoder(*this), decoder(*this) { rand_gen.seed(std::time(0)); }
            virtual ~thinger_controller() {}

        private:
            thinger_write_encoder encoder;
            thinger_read_decoder decoder;

            std::mt19937 rand_gen;

            // Generates random uint for thinger communication
            int generate_rand_stream() 
            {
                std::uniform_int_distribution<uint32_t> rand_stream_id(1, 32767);
                return rand_stream_id(rand_gen);
            }

        public:
            // Encodes the thinger message before communicating with Thinger.io client
            bool send_message(thinger_message &message)
            {
                thinger_encoder sink;
                sink.encode(message);
                encoder.pb_encode_varint(MESSAGE);
                encoder.pb_encode_varint(sink.bytes_written());
                encoder.encode(message);
                return write(NULL, 0, true);
            }

            // Handles the received message appropriately
            // @param {peer} - Client connected object
            bool handle_input(peer &p)
            {
                thinger_message message;

                // Decodes the message
                if (read_message(message))
                {
                    // Handles the decoded message respectively
                    if (handle_request_received(message, p))
                        // Prepares the resource to be sent to Wappsto
                        if(process_resources(p))
                            return true;
                }
                return false;
            }

            // Decodes the received message from Thinger.io client
            // @param {thinger_message} - Thinger message object
            bool read_message(thinger_message &message)
            {
                uint32_t type = 0;
                if (decoder.pb_decode_varint32(type))
                {
                    switch (type)
                    {
                        case MESSAGE:
                        {
                            uint32_t size = 0;
                            return decoder.pb_decode_varint32(size) && decoder.decode(message, size);
                        }
                        case KEEP_ALIVE:
                        {
                            //keep_alive_response = true;
                            decoder.pb_skip_varint();
                            std::cout << "[SERVER]: Keep alive received." << std::endl;
                            return true;
                        }
                    }
                }
                return false;
            }

            // Responsible for filling attributes of the stream
            // @param {pson_object} - Reference to the object, from where to extract attributes
            // @param {thinger_stream} - Reference which object to fill out
            void fill_stream(pson_pair &pair, thinger_stream &stream, peer &p)
            {
                std::string resource_name = pair.name();
                int acess_type = pair.value()["al"].get_value<int>();

                // Check if stream with the generated id exist, regen if yes
                int stream_id = generate_rand_stream();
                if(p.stream_exist(stream_id)) {
                    stream_id = generate_rand_stream();
                }

                // Fill necessary stream data
                stream.get_resource_access_type() = thinger_resource::get_access_type_value(acess_type);
                stream.get_stream_id() = stream_id;
                stream.get_resource_name() = resource_name;
            }

        private:
            void request_client_resources(thinger_message &message, peer &p)
            {
                for (pson_object::iterator it = message.data_f().begin(); it.valid(); it.next())
                {
                    thinger_stream peer_stream;
                    fill_stream(it.item(), peer_stream, p);
                    p.insert_stream(peer_stream);

                    // Fill resource response
                    thinger_message resource_response;
                    resource_response.set_stream_id(peer_stream.get_stream_id());
                    resource_response.resources().add(peer_stream.get_resource_name()).add("api");

                    //resource_response.get_data()[testtt] = 15;

                    if (send_message(resource_response))
                        peer_stream.get_stream_state() = thinger_stream::SUBRESOURCE_REQUESTED;
                }
            }

            // Fills in subresources of Thinger.io
            void fill_io(pson_object &object, thinger_stream &stream, bool in)
            {
                for(pson_object::iterator it = object.begin(); it.valid(); it.next())
                {
                    thinger_stream::subresource res;
                    res.name = it.item().name();
                    res.value = it.item().value();

                    in == true ? stream.insert_subresource_in(res) : stream.insert_subresource_out(res);
                }
            }

            bool handle_request_received(thinger_message &message, peer &p)
            {
                return message.get_signal_flag() == thinger_message::AUTH ? hande_client_authentication(message, p) : handle_data_received(message, p);
            }

            bool hande_client_authentication(thinger_message &message, peer &p)
            {
                thinger_message response(message);

                if (p.get_is_auth())
                    return true;

                LOG("[SERVER]: Authentication request received, sending confirmation...");

                if (send_message(response))
                {
                    std::cout << "[SERVER]: Authentication confirmation sent!" << std::endl;
                    p.get_is_auth() = true;
                    read_message_resources(message, p);
                    return true;
                }

                return false;
            }

            bool handle_data_received(thinger_message &message, peer &p)
            {
                if (message.get_signal_flag() == thinger_message::REQUEST_OK)
                {
                    thinger_stream *peer_stream = p.get_stream(message.get_stream_id());

                    // Check if the message contains data and that the stream exists for the peer respectively
                    if (message.has_data() && &peer_stream != nullptr)
                    {
                        // Check if the stream is an api request
                        if (peer_stream->get_resource_name() == "api" && peer_stream->get_stream_state() == peer_stream->RESOURCE_REQUESTED)
                        {
                            peer_stream->get_stream_state() = peer_stream->RESOURCE_RECEIVED;
                            request_client_resources(message, p);
                        }
                        else
                        {
                            peer_stream->get_stream_state() = peer_stream->SUBRESOURCE_RECEIVED;

                            for(pson_object::iterator it = message.data_f().begin(); it.valid(); it.next())
                            {
                                std::string io_type = it.item().name();

                                if(io_type == "in") {
                                    fill_io(it.item().value(), *peer_stream, true);
                                } else if (io_type == "out") {
                                    fill_io(it.item().value(), *peer_stream, false);
                                }
                            }

                            peer_stream->get_stream_state() = peer_stream->CONFIRMED;
                        }

                        return true;
                    }
                    else
                    {
                        std::cout << "Resource confirmation OK!" << std::endl;
                        return true;
                    }

                    std::cout << "[SERVER]: Invalid request, no data!" << std::endl;
                    return false;
                }

                if (message.get_signal_flag() == thinger_message::NONE)
                {
                    // Is it KEEP ALIVE?
                    std::cout << "Some Message Received..." << std::endl;

                    // Request APIS
                    std::this_thread::sleep_for(std::chrono::seconds(3));

                    thinger_message message;
                    thinger_stream stream;

                    uint16_t rstream_id = generate_rand_stream();
                    message.set_stream_id(rstream_id);
                    stream.get_stream_id() = rstream_id;
                    stream.get_resource_name() = "api";
                    

                    message.resources().add("api");
                    if (send_message(message)) {
                        LOG("Requesting resources from the client.");
                        stream.get_stream_state() = thinger_stream::RESOURCE_REQUESTED;
                        p.insert_stream(stream);
                    }

                    return true;
                }
            }

            bool read_message_resources(thinger_message &message, peer &p)
            {
                if (!message.has_resource())
                {
                    return false;
                }
                else
                {
                    int counter = 1;
                    for (pson_array::iterator it = message.resources().begin(); it.valid(); it.next())
                    {
                        if (it.item().is_string())
                        {
                            std::string item = (char *)(it.item().get_value());

                            switch (counter)
                            {
                            case 1:
                                p.get_credentials().get_user_id() = item;
                                break;
                            case 2:
                                p.get_credentials().get_device_id() = item;
                                break;
                            case 3:
                                p.get_credentials().get_device_cred() = item;
                                break;
                            }
                            counter++;
                        }
                    }
                }
            }

            bool process_resources(peer& p)
            {
                int resource_stream_size = p.get_t_stream().size();

                // If all resources received
                if(check_sub_confirmed(p.get_t_stream()))
                {
                    LOG("------------------ ALL RESOURCES WERE PROCEEDED!!!");
                    LOG("------------------ BEGINING TRANSLATION");
                    wappsto_translator translator;
                    json wappsto = translator.begin_translation(p);
                    LOG("------------------ TRANSLATED");
                    LOG("------------------ BEGIN CONNECTION TO BACKEND!!!");
                    
                    backendsocket backend;
                    backend.connect();
                }
                else
                {
                    LOG("------------------ PENDING FOR PROCESSES TO BE PROCEDED!")
                }
                return true;
            }

            // Checks if all the subresources of Thinger.io was received and processed.
            bool check_sub_confirmed(std::vector<thinger_stream>& streams)
            {
                int confirmed_no = 0;
                int size = streams.size();

                if(size > 1)
                {
                    for(auto& s : streams)
                        if(s.get_resource_name() != "api" && s.get_stream_state() == thinger_stream::CONFIRMED)
                            confirmed_no++;

                    return size-1 == confirmed_no;

                } else { 
                    return false;
                }
            }
        };
} // namespace thinger
