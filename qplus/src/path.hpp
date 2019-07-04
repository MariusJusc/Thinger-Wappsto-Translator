#pragma once

#include <dirent.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h> 

namespace qplus {
 
    typedef std::vector<std::string>::const_iterator piterator;

    class Path {

        private:
            piterator fit_;
            piterator dit_;

            std::vector<std::string> filenames_;
            std::vector<std::string> dirnames_;

        public:
            Path(const std::string& dir);
            ~Path();

            piterator& file();
            piterator& directory();
            piterator files_end();
            piterator dirs_end();

			static inline bool exists (const std::string& name) {
	  			struct stat buffer;   
	 			return (stat (name.c_str(), &buffer) == 0); 
			}

            static void mkdirectory(const std::string& name); 
            static void mkfile(const std::string& name); 
    };

} // namespace qplus
