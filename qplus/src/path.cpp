#include "path.hpp"
#include <iostream>
#include "error.hpp"
#include <sys/types.h>

namespace qplus {

    Path::Path(const std::string& path)
    {
        DIR* dird;
        DIR* dirf;
        struct dirent* pd;
        struct dirent* pf;
    
        dit_ = dirnames_.end();
        fit_ = filenames_.end();
        
        if ( (dird = opendir(path.c_str())) == NULL)  {
            std::string msg = "Can not open " + path + " directory!";
            std::cout << msg << "\n";
            throw error_t(msg); 
        }

        while ( (pd = readdir(dird)) != NULL) {
            std::string dirName = pd->d_name; 
            if (pd->d_type == DT_DIR && dirName != ".." && dirName != ".") {
                dirnames_.push_back(dirName); 
            } 
        }

        dirf = opendir(path.c_str());
        while ( (pf = readdir(dirf)) != NULL) {
            std::string fileName = pf->d_name; 
            if (pf->d_type == DT_REG) {
                filenames_.push_back(fileName); 
            } 
         }

        closedir(dird);
        closedir(dirf);
        
        if (!dirnames_.empty())
            dit_ = dirnames_.begin();
        
        if (!filenames_.empty())
            fit_ = filenames_.begin();
    }
    
    Path::~Path()
    {

    }
    
    void Path::mkdirectory(const std::string& name) 
    {
        //mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
        mode_t mode = 0755;
        int fd = mkdir(name.c_str(), mode); 
        if (fd == -1) {
            throw error_t("Directory: '" + name + "'  has not been created!"); 
        }
    }
    
    void Path::mkfile(const std::string& name)
    {
        mode_t mode = O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC;
        int fd = open(name.c_str(), mode);
        if (fd == -1) {
            throw error_t("File: '" + name + "'  has not been created!"); 
        }
    }


    piterator& Path::file()
    {
        return fit_;
    }
    
    piterator& Path::directory()
    {
         return dit_;
    }
    
    piterator Path::files_end()
    {
        return filenames_.end();
    }
   
    piterator Path::dirs_end()
    {
        return dirnames_.end();
    }

} // namespace qplus
