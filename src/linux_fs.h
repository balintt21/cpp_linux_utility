#ifndef LINUX_FS_H_
#define LINUX_FS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <functional>
#include <queue>
#include <vector>
#include <string>
#include <cstring>

namespace linux_fs
{
    int list_dir(const std::function<bool (const std::string&)>& yield, const std::string& path, bool recursive = false, bool hidden = false)
    {
        std::queue<std::string> directories;
        directories.push(path);
        while(!directories.empty())
        {
            std::string dir_path = directories.front();
            directories.pop();
            DIR* dir = opendir(dir_path.c_str());
            if( dir )
            {
                struct dirent * entry = readdir(dir);
                while(entry)
                {
                    if( entry->d_name[0] != '.' || (hidden && (strlen(entry->d_name) > 1) && entry->d_name[1] != '.') )
                    { 
                        std::string abs_path = dir_path + ((dir_path.back() != '/') ? "/" : "") + entry->d_name;
                        if( !yield(abs_path) ) { return 0; }
                        if(recursive)
                        {
                            if( entry->d_type == DT_UNKNOWN )
                            {
                                struct stat st;
                                if( lstat(abs_path.c_str(), &st) == 0 )
                                { 
                                    if( S_ISDIR(st.st_mode) )
                                    { directories.push(abs_path); } 
                                }
                                else 
                                { return errno; }
                            }
                            else if( entry->d_type == DT_DIR )
                            { directories.push(abs_path); }
                        }
                    }
                    entry = readdir(dir);
                    if(entry < 0){ return errno; }
                }
                closedir(dir);
            } else {
                return errno;
            }
        }
        return 0;
    }

    std::vector<std::string> list_dir(const std::string& path, bool recursive = false, bool hidden = false)
    {
        std::vector<std::string> result;
        list_dir([&result](const std::string& entry) -> bool { result.push_back(entry); return true; }, path, recursive, hidden);
        return result;
    }

    ssize_t dir_size(const std::string& path)
    {
        ssize_t result = 0;
        constexpr bool recursive = true;
        constexpr bool list_hidden = true;
        list_dir([&result](const std::string& entry) -> bool 
        { 
            struct stat st;
            if( lstat(entry.c_str(), &st) == 0 )
            {
                if(!S_ISDIR(st.st_mode))
                { result += st.st_size; }
            } else {
                result = -errno;
                return false;
            }
            return true;
        }, path, recursive, list_hidden);
        return result;
    }
}

#endif
