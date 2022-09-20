#ifndef GUARD_winmerge_hpp
#define GUARD_winmerge_hpp
//  ---------------------------------------------
//  Launch external program winmerge
//  ---------------------------------------------
#include "system.hpp" // sys::find_executable(), sys::launch(), sys::expand_env_variables(), sys::file_write


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace winmerge //::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
void create_project(const std::string& prj_file, const std::string& pth1, const std::string& pth2, const std::string& pth3)
{
    sys::file_write f(prj_file);
    f<< "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<project>\n"
        "  <paths>\n"
        "    <left>" + pth1 + "</left>\n";
    if( !pth3.empty() )
       {
        f<< "    <middle>" + pth3 + "</middle>\n";
       }
    f<< "    <right>" + pth2 + "</right>\n"
        "    <filter>*.*</filter>\n"
        "    <subfolders>1</subfolders>\n"
        "    <left-readonly>0</left-readonly>\n"
        "    <right-readonly>0</right-readonly>\n"
        "  </paths>\n"
        "</project>\n";
}



//---------------------------------------------------------------------------
std::string find_exe()
{
    std::string exe_pth = "C:\\Macotec\\Apps\\WinMerge\\WinMergeU.exe";
    if( !fs::exists(exe_pth) )
       {
        exe_pth = sys::expand_env_variables("%PROGRAMFILES%\\WinMerge\\WinMergeU.exe");
        if( !fs::exists(exe_pth) )
           {
            exe_pth = sys::find_executable_by_ext(".WinMerge");
            if( !fs::exists(exe_pth) )
               {
                throw std::runtime_error("WinMerge executable not found");
               }
           }
       }
    return exe_pth;
}



//---------------------------------------------------------------------------
// Compare files/folders using WinMerge
void compare(const std::string& pth1, const std::string& pth2, const std::string& pth3 ="")
{
    //    /r recurse subfolders
    //    /e close WinMerge with a single Esc key press
    //    /f filter to restrict the comparison
    //    /x closes in case of identical files
    //    /s limits WinMerge windows to a single instance
    //    /u prevents WinMerge from adding paths to the Most Recently Used (MRU) list
    //    /wl /wr opens the side as read-only
    //    /dl /dr descriptions

    if( fs::is_directory(pth1) || fs::is_directory(pth2) )
       {
        const std::string prj_file( "~Compare.WinMerge" );
        create_project(prj_file, pth1, pth2, pth3);

        try{
            const std::string winmerge_exe = find_exe();
            sys::launch(winmerge_exe, prj_file);
           }
        catch(...)
           {
            sys::launch(prj_file); // Relying on '.WinMerge' extension association
           }

        sys::sleep_ms(250);
        sys::delete_file(prj_file);
       }
    else
       {
        const std::string winmerge_exe = find_exe();
        const std::string args = pth3.empty() ? fmt::format(" /e /u \"{}\" \"{}\"", pth1, pth2)
                                              : fmt::format(" /e /u \"{}\" \"{}\" \"{}\"", pth1, pth2, pth3);
        sys::launch(winmerge_exe, args);
       }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
