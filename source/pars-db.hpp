#ifndef GUARD_pars_db_hpp
#define GUARD_pars_db_hpp
//  ---------------------------------------------
//  Parameters database details
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <fmt/core.h> // fmt::format

#include "system.hpp" // sys::MemoryMappedFile, sys::edit_text_file
#include "json-parser.hpp" // json::parse()


/////////////////////////////////////////////////////////////////////////////
class ParsDB final
{
 public:

    //-----------------------------------------------------------------------
    // Parse a db json file
    void parse(const fs::path& pth, std::vector<std::string>& issues)
       {
        std::vector<std::string> parse_issues;
        try{
            const sys::MemoryMappedFile dbfile_buf(pth.string());
            json::parse(dbfile_buf.path(), dbfile_buf.as_string_view(), i_root, parse_issues, true);
           }
        catch( parse_error& e)
           {
            sys::edit_text_file( e.file_path(), e.pos() );
            throw;
           }

        // Append parsing issues to overall issues list
        if( !parse_issues.empty() )
           {
            const std::string prefix{ pth.filename().string()};
            for(const auto& issue_entry : parse_issues )
               {
                issues.push_back( fmt::format("[{}]: {}", prefix , issue_entry) );
               }
           }
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] std::string info() const { return fmt::format("{} first level nodes ({} values)", i_root.direct_childs_count(), i_root.overall_leafs_count()); }
    [[nodiscard]] std::string string() const { return i_root.string(); }
    void print() const { return i_root.print(); }

 private:
    json::Node i_root;
};



//---- end unit -------------------------------------------------------------
#endif
