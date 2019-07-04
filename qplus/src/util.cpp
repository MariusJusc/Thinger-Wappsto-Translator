#include "util.hpp"
#include "error.hpp"
#include <sstream>
#include <list>
#include <map>
#include "error.hpp"
#include <chrono>


using json = nlohmann::json;

namespace qplus {

    std::vector<std::string>& _split(const std::string &s, char delim, std::vector<std::string> &elems) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            if (!item.empty())
                elems.push_back(item);
        }
        return elems;
    }

    std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        _split(s, delim, elems);
        return elems;
    }
    
    std::string concat(const std::vector<std::string>& v) {
        std::string res;
        for (auto it = v.begin(); it != v.end(); ++it) {
           res += "/" + *it; 
        }
        return res;
    }

    std::vector<std::string> findJsons(const std::string& msg)
    {
        std::map<int, int> d = {};
        std::list<int> istart;
        int count = 0;
        
        for (size_t i = 0; i < msg.size(); ++i) {
            if (msg[i] == '{') {
                if (count == 0)
                    istart.push_back(i);
                count += 1;
            }
            if (msg[i] == '}') {
                if (count == 1) {
                   if (istart.empty()) {
                        throw error_t("Parsing json: too many closing parentheses");
                   }
                   else {
                        int idx = istart.back();
                        // map key as start and value as end.
                        d[idx] = i;
                        istart.pop_back();
                   }
                }
                count -= 1;
            }
        }

        if (!istart.empty()) {
            auto err = json::parse_error::create(101, 0, "too many opening parentheses.");
            throw err;
        }

        std::vector<std::string> result;
        for (auto k : d) {
            // substring 1 - position, 2 - length.
            result.push_back(msg.substr(k.first, (k.second+1) - k.first)); 
        }

        return result;
    }


}  // namespace qplus
