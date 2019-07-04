#include "serializer.hpp"
#include <ftw.h>
#include <fts.h>
#include "path.hpp"

namespace qplus {
    
    void serialize(const json& ajson, const std::string& filename)
    {
        std::ofstream os(filename.c_str(), std::ios::out);
        if (!os) {
            throw error_t("Can't open file: " + filename + " for writing.");
        }
        os << ajson.dump();
        os.close();
    }
        
    void deserialize(const std::string& filename, json& ajson)
    {
        std::ifstream is(filename.c_str()); 
        if (!is.good()) {
            throw error_t("Can't open file: " + filename + " for reading.");
        }
        try {
            is >> ajson;
            is.close();
        }
        catch(std::exception& e) {
            is.close();
            throw error_t("Deserialization failed: " +  filename + " " + e.what());
        }
    }

    void removeDir(const std::string& path) 
    {
        if (Path::exists(path)) {
            recursiveDelete(path.c_str());
        }
    }

    int recursiveDelete(const char *dir)
    {
        int ret = 0;
        FTS *ftsp = NULL;
        FTSENT *curr;

        // Cast needed (in C) because fts_open() takes a "char * const *", instead
        // of a "const char * const *", which is only allowed in C++. fts_open()
        // does not modify the argument.
        char *files[] = { (char *) dir, NULL };

        // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
        //                in multithreaded programs
        // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
        //                of the specified directory
        // FTS_XDEV     - Don't cross filesystem boundaries
        ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
        if (!ftsp) {
            fprintf(stderr, "%s: fts_open failed: %s\n", dir, strerror(errno));
            ret = -1;
            goto finish;
        }

        while ((curr = fts_read(ftsp))) {
            switch (curr->fts_info) {
            case FTS_NS:
            case FTS_DNR:
            case FTS_ERR:
                fprintf(stderr, "%s: fts_read error: %s\n",
                        curr->fts_accpath, strerror(curr->fts_errno));
                break;

            case FTS_DC:
            case FTS_DOT:
            case FTS_NSOK:
                // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
                // passed to fts_open()
                break;

            case FTS_D:
                // Do nothing. Need depth-first search, so directories are deleted
                // in FTS_DP
                break;

            case FTS_DP:
            case FTS_F:
            case FTS_SL:
            case FTS_SLNONE:
            case FTS_DEFAULT:
                if (remove(curr->fts_accpath) < 0) {
                    fprintf(stderr, "%s: Failed to remove: %s\n",
                            curr->fts_path, strerror(errno));
                    ret = -1;
                }
                break;
            }
        }

    finish:
        if (ftsp) {
            fts_close(ftsp);
        }

        return ret;
    }
}
