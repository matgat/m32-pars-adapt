#pragma once
//  ---------------------------------------------
//  Manual compare or replace original file
//  ---------------------------------------------
//  #include "handle_output_file.hpp" // app::handle_output_file()
//  ---------------------------------------------
#include <string_view>

#include "filesystem_utilities.hpp" // fs::*, fsu::backup_file
#include "compare_text_files.hpp" // sys::compare_files_wait()

using namespace std::literals; // "..."sv


namespace app //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
[[nodiscard]] bool is_temp(const fs::path& pth)
{
    return pth.extension() == ".tmp"sv;
}


//---------------------------------------------------------------------------
void replace_file_with(const fs::path& old_pth, const fs::path& new_pth)
{
    fsu::backup_file( old_pth );
    fs::remove( old_pth );
    fs::rename( new_pth, old_pth );
}

//---------------------------------------------------------------------------
[[nodiscard]] const fs::path& empty_if_or(const bool wants_empty, const fs::path& pth)
{
    static const fs::path empty;
    return wants_empty ? empty : pth;
}




//---------------------------------------------------------------------------
void handle_output_file(const bool quiet, const fs::path& adapted_file, const fs::path& original_file, const fs::path& template_file ={})
{
    if( quiet )
       {// No user intervention
        // If created a temporary, swap it with the original file
        if( is_temp(adapted_file) )
           {
            replace_file_with(original_file, adapted_file);
           }
       }
    else
       {// Manual merge
        sys::compare_files_wait( adapted_file.string().c_str(), original_file.string().c_str() );

        if( not template_file.empty() )
           {
            sys::compare_files_wait( template_file.string().c_str(), original_file.string().c_str() );
           }

        // If created a temporary, remove it after manual merge
        if( is_temp(adapted_file) )
           {
            fs::remove( adapted_file );
           }
       }
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
