#pragma once

#include <string>

namespace qplus {

    class error_t : public std::exception
    {
        private:
            std::string err_msg_;
        public:
            error_t(const std::string& msg) : err_msg_(msg) {};
            virtual ~error_t() throw() {};
            virtual const char* what() const throw()
            {
                return err_msg_.c_str();
            }
    };

} // namespace qplus 

