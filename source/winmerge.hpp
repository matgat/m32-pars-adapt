#ifndef GUARD_winmerge_hpp
#define GUARD_winmerge_hpp
//  ---------------------------------------------
//  Launch external program winmerge
//  ---------------------------------------------
#include "system.hpp" // sys::*


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace winmerge //::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
std::string find_exe() // Must be a null terminated string for winapi
{
    std::string exe_pth = "C:\\Macotec\\Apps\\WinMerge\\WinMergeU.exe";
    if( !fs::exists(exe_pth) )
       {
        //exe_pth = sys::expand_env_variables("%PROGRAMFILES%\\WinMerge\\WinMergeU.exe");
        exe_pth = "C:\\Program Files\\WinMerge\\WinMergeU.exe";
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
// Launch WnMerge to compare files/folders
void compare(const std::string_view pth1, const std::string_view pth2, const std::string_view pth3 ="")
{
    // -r :recurse subfolders
    // -e :close WinMerge with a single Esc key press
    // -f :filter to restrict the comparison
    // -x :closes in case of identical files
    // -s :limits WinMerge windows to a single instance
    // -u :prevents WinMerge from adding paths to the Most Recently Used (MRU) list
    // -wl -wr :opens the side as read-only
    // -dl -dr :descriptions

    const std::string winmerge_exe = find_exe();
    const std::string args = pth3.empty() ? fmt::format(" -s -e -u -r -f \"*.*\" \"{}\" \"{}\"", pth1, pth2)
                                          : fmt::format(" -s -e -u -r -f \"*.*\" \"{}\" \"{}\" \"{}\"", pth1, pth2, pth3);
    sys::shell_execute(winmerge_exe.c_str(), args.c_str());
}


//---------------------------------------------------------------------------
// Launch WinMerge to compare files/folders (waiting)
[[maybe_unused]] int compare_wait(const std::string_view pth1, const std::string_view pth2, const std::string_view pth3 ="")
{
    const std::string winmerge_exe = find_exe();
    const std::string args = pth3.empty() ? fmt::format(" -e -u -r -f \"*.*\" \"{}\" \"{}\"", pth1, pth2)
                                          : fmt::format(" -e -u -r -f \"*.*\" \"{}\" \"{}\" \"{}\"", pth1, pth2, pth3);
    return sys::shell_execute_wait(winmerge_exe.c_str(), args.c_str());
}


//---------------------------------------------------------------------------
void create_project(const std::string& prj_file, const std::string_view pth1, const std::string_view pth2, const std::string_view pth3)
{
    sys::file_write f(prj_file);
    f<< "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<project>\n"
        "  <paths>\n"
        "    <left>" << pth1 << "</left>\n";
    if( !pth3.empty() )
       {
        f<< "    <middle>" << pth3 << "</middle>\n";
       }
    f<< "    <right>" << pth2 << "</right>\n"
        "    <filter>*.*</filter>\n"
        "    <subfolders>1</subfolders>\n"
        "    <left-readonly>0</left-readonly>\n"
        "    <right-readonly>0</right-readonly>\n"
        "  </paths>\n"
        "</project>\n";
}


//---------------------------------------------------------------------------
// Launch WnMerge on a temporary project file
void compare_using_project(const std::string_view pth1, const std::string_view pth2, const std::string_view pth3 ="")
{
    const std::string prj_file( "~Compare.WinMerge" );
    create_project(prj_file, pth1, pth2, pth3);
    
    try{
        const std::string winmerge_exe = find_exe();
        sys::shell_execute(winmerge_exe.c_str(), prj_file.c_str());
       }
    catch(...)
       {
        sys::shell_execute(prj_file.c_str()); // Relying on '.WinMerge' extension association
       }
    
    sys::sleep_ms(250);
    fs::remove(prj_file);
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
