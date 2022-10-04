#ifndef GUARD_file_glob_hpp
#define GUARD_file_glob_hpp
//  ---------------------------------------------
//  Expands a glob in filesystem
//  ---------------------------------------------
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;
//#include <regex> // std::regex*

#include "string-utilities.hpp" // str::contains_wildcards, str::glob_match 

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
// file_glob("/aaa/bbb/*.txt");
[[nodiscard]] std::vector<fs::path> file_glob(const fs::path pth)
{
    if( str::contains_wildcards(pth.parent_path().string()) )
       {
        throw std::runtime_error("sys::file_glob: Wildcards in directories not supported");
       }

    //if( pth.is_relative() ) pth = fs::absolute(pth); // Ensure absolute path?
    fs::path parent_folder = pth.parent_path();
    if( parent_folder.empty() ) parent_folder = fs::current_path();

    const std::string filename_glob = pth.filename().string();
    std::vector<fs::path> result;
    if( str::contains_wildcards(filename_glob) && fs::exists(parent_folder) )
       {
        result.reserve(16); // Minimize initial allocations
        for( const auto& entry : fs::directory_iterator(parent_folder, fs::directory_options::follow_directory_symlink |
                                                                       fs::directory_options::skip_permission_denied) )
           {// Collect if matches
            if( entry.exists() && entry.is_regular_file() && str::glob_match(entry.path().filename().string().c_str(), filename_glob.c_str()) )
               {
                //const fs::path entry_path = parent_folder.is_absolute() ? entry.path() : fs::proximate(entry.path());
                result.push_back( entry.path() );
               }
           }

        // Using std::regex
        //// Create a regular expression from glob pattern
        //auto glob2regex = [](const std::string& glob_pattern) noexcept -> std::string
        //   {
        //    // Escape special characters in file name
        //    std::string regexp_pattern = std::regex_replace(glob_pattern, std::regex("([" R"(\$\.\+\-\=\[\]\(\)\{\}\|\^\!\:\\)" "])"), "\\$1");
        //    // Substitute pattern
        //    regexp_pattern = std::regex_replace(regexp_pattern, std::regex(R"(\*)"), ".*");
        //    regexp_pattern = std::regex_replace(regexp_pattern, std::regex(R"(\?)"), ".");
        //    //fmt::print("regexp_pattern: {}" ,regexp_pattern);
        //    return regexp_pattern;
        //   };
        //const std::regex reg(glob2regex(filename_glob), std::regex_constants::icase);
        //... std::regex_match(entry.path().filename().string(), reg)
       }
    else
       {// Nothing to glob
        result.reserve(1);
        result.push_back(pth);
       }

    return result;
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
