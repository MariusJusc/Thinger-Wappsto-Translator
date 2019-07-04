#include <iostream>
#include <chrono>   // std::chrono::system_clock
#include <iomanip>  // std::put_time
#include <sstream>  // std::ostringstream

#include "models/peer.hpp"
#include "json.hpp"

class wappsto_translator {

using json = nlohmann::json;

public:
    wappsto_translator() {

    }

    json begin_translation(peer& p)
    {
        json wappsto;

        for(auto& stream: p.get_t_stream())
        {
            // Translate device
            device_to_json(wappsto, stream);

            // Translate values
            insert_value(wappsto, stream);                   
        }
        std::cout << wappsto.dump(4) << std::endl;

        return wappsto;
    }

private:
    void device_to_json(json& j, thinger_stream& stream)
    {
         j = json { 
            { "name", stream.get_resource_name() }, 
            { "manufacturer", "" },
            { "product", "" },
            { "version", "1" },
            { "serial", "" },
            { "protocol", "TCP/IP" },
            { "communication", "" },
            { "value", {}}
        };
    }

    void insert_value(json& wappsto, thinger_stream& stream)
    {
        if(stream.get_subresource_in().size() > 0)
        {
            for(auto &sub : stream.get_subresource_in())
            {
                value_to_json(wappsto, sub, true);
            }
        }
    }

    void value_to_json(json& j, thinger_stream::subresource& sub, bool is_report)
    {
        json value = json {
            { "name", sub.name },
            { "type", "" },
            { "status", "" },
            { "state", {} }
        };

        // Check permissions
        if(is_report)
            value["permission"] =  "r";
        else
            value["permission"] =  "w";
        
        // Check value type
        if(is_vnumber_type(sub))
            value["number"] = { 
                    {"min", "0" }, 
                    {"max", "0" }, 
                    {"step", "0" },            
                    {"unit", "" }            
                };
        else if (is_vstring_type(sub))
        {
            value["string"] = { 
                { "max", "0" }, 
                { "encoding", "UTF" }     
            };
        } else {}
        
        // TODO: Implement BLOB
        
        // Create state for the value
        state_to_json(value, sub);

        // Insert values to the device
        j["value"].push_back(value);
    }

    void state_to_json(json& j, thinger_stream::subresource& sub)
    {
        std::string name = sub.name;
        auto value = char(sub.value);

        json state = json {
            { "data", value },
            { "timestamp", currentISO8601TimeUTC() },
            { "type", "" },
            { "status", "Send" }
        };

        j["state"].push_back(state);
    }

    std::string currentISO8601TimeUTC() 
    {
        auto now = std::chrono::system_clock::now();
        auto itt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ss;
        ss << std::put_time(gmtime(&itt), "%FT%TZ");
        return ss.str();
    }

    bool is_vnumber_type(thinger_stream::subresource& sub)
    {
        int type = sub.value.get_type();
        return type == 1 ||
               type == 2 ||
               type == 3 ||
               type == 4 ||
               type == 7 ||
               type == 8;
    }

    bool is_vstring_type(thinger_stream::subresource& sub)
    {
        int type = sub.value.get_type();
        return type == 9;
    }

    std::string get_value_permission(thinger_stream& stream) 
    {
        //Depending on the size of the containers, decide which permission
        int in_sub_count = stream.get_subresource_in().size();
        int out_sub_count = stream.get_subresource_out().size();

        if(in_sub_count > 0 && out_sub_count > 0) {
            return "rw";                              // pson_in_pson_out
        } else if (in_sub_count > 0) {
            return "r";                               // pson_in
        } else if (out_sub_count > 0) {
            return "w";                               // pson_out
        } else {}
    }
};