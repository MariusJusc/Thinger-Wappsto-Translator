#ifndef THINGER_CREDENTIALS_HPP
#define THINGER_CREDENTIALS_HPP

#include <iostream>

class thinger_credentials {

private:
    std::string user_id;
    std::string device_id;
    std::string device_cred;

public:
    auto get_user_id()       -> std::string& { return user_id; }
    auto get_user_id() const -> const std::string& { return user_id; }

    auto get_device_id()       -> std::string& { return device_id; }
    auto get_device_id() const -> const std::string& { return device_id; }

    auto get_device_cred()       -> std::string& { return device_cred; }
    auto get_device_cred() const -> const std::string& { return device_cred; }
};

#endif