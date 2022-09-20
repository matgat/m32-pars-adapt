//  ---------------------------------------------
//  Utility to handle the parametrization of
//  m32 based machines
//  ---------------------------------------------
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept> // std::runtime_error
#include <fmt/core.h> // fmt::*

#include "system.hpp" // sys::*, fs::*
//#include "string-utilities.hpp" // str::tolower

#include "winmerge.hpp" // winmerge::*
#include "machine-type.hpp" // MachineType
#include "pars-db.hpp" // ParsDB
#include "udt-file.hpp" // udt::File

using namespace std::literals; // "..."sv



/////////////////////////////////////////////////////////////////////////////
class JobUnit final
{
 public:
    [[nodiscard]] const auto& machine_type() const noexcept { return i_machinetype; }
    void set_machine_type(const std::string_view s) { i_machinetype = s; }

    [[nodiscard]] auto& target_file() const noexcept { return i_targetfile; }
    void set_target_file(const std::string_view s) { i_targetfile = s; }

    [[nodiscard]] auto& db_file() const noexcept { return i_dbfile; }
    void set_db_file(const std::string_view s) { i_dbfile = s; }

    [[nodiscard]] bool is_complete() const noexcept { return !i_machinetype.empty() && !i_targetfile.empty() && !i_dbfile.empty(); }
    [[nodiscard]] std::string info() const { return fmt::format("{}, {} => {}", i_machinetype, i_dbfile.filename().string(), i_targetfile.filename().string()); }

 private:
    std::string i_machinetype;
    fs::path i_targetfile;
    fs::path i_dbfile;
};


/////////////////////////////////////////////////////////////////////////////
class Arguments final
{
 public:
    Arguments(const int argc, const char* const argv[]) // const std::span args
       {
        // Expecting pll file paths
        i_jobs.reserve( static_cast<std::size_t>(argc/3) );
        try{
            enum class STS
               {
                SEE_ARG,
                GET_MACHTYPE,
                GET_PARSFILE,
                GET_PARSDB
               } status = STS::SEE_ARG;

            //for( const auto arg : args | std::views::transform([](const char* const a){ return std::string_view(a);}) )
            for( int i=1; i<argc; ++i )
               {
                const std::string_view arg{ argv[i] };
                switch( status )
                   {
                    case STS::GET_MACHTYPE :
                        if( !i_jobs.empty() && !i_jobs.back().is_complete() )
                           {
                            throw std::invalid_argument("Preceding job was not completely defined");
                           }
                        i_jobs.emplace_back();
                        i_jobs.back().set_machine_type(arg);
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_PARSFILE :
                        if( !fs::exists(arg) )
                           {
                            throw std::invalid_argument(fmt::format("Parameter file doesn't exists: {}",arg));
                           }
                        i_jobs.back().set_target_file(arg);
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_PARSDB :
                        if( !fs::exists(arg) )
                           {
                            throw std::invalid_argument(fmt::format("DB file doesn't exists: {}",arg));
                           }
                        i_jobs.back().set_db_file(arg);
                        status = STS::SEE_ARG;
                        break;

                    default :
                        if( arg.length()>1 && arg[0]=='-' )
                           {// A command switch!
                            // Skip hyphen, tolerate also doubled ones
                            const std::size_t skip = arg[1]=='-' ? 2 : 1;
                            const std::string_view swtch{ arg.data()+skip, arg.length()-skip};
                            if( swtch=="machine"sv || swtch=="m"sv )
                               {
                                status = STS::GET_MACHTYPE;
                               }
                            else if( swtch=="target"sv || swtch=="tgt"sv || swtch=="t"sv )
                               {
                                status = STS::GET_PARSFILE;
                               }
                            else if( swtch=="db"sv )
                               {
                                status = STS::GET_PARSDB;
                               }
                            else if( swtch=="quiet"sv )
                               {
                                i_quiet = true;
                               }
                            else if( swtch=="verbose"sv || swtch=="v"sv )
                               {
                                i_verbose = true;
                               }
                            else if( swtch=="help"sv || swtch=="h"sv )
                               {
                                print_help();
                                throw std::invalid_argument("Aborting after printing help");
                               }
                            else
                               {
                                throw std::invalid_argument(fmt::format("Unknown command switch: {}",swtch));
                               }
                           }
                        else
                           {
                            throw std::invalid_argument(fmt::format("Unrecognized argument: {}",arg));
                           }
                   }
               } // each argument
           }
        catch( std::exception& e)
           {
            throw std::invalid_argument(e.what());
           }
       }

    static void print_help() noexcept
       {
        fmt::print( "\nm32-pars-adapt (ver. " __DATE__ ")\n"
                    "An utility to handle the parametrization of m32 based machines\n"
                    "\n" );
       }

    static void print_usage() noexcept
       {
        fmt::print( "\nUsage (ver. " __DATE__ "):\n"
                    "   m32-pars-adapt --verbose --quiet --machine StratoWR-4.9/4.6 --target path/to/MachSettings.udt --db path/to/msetts_pars.txt\n"
                    "       --machine <string> (Machine type string)\n"
                    "       --target <path> (Parameter file to adapt)\n"
                    "       --db <path> (Parameters database json file)\n"
                    "       --quiet (Force actions, no manual checks)\n"
                    "       --verbose (Print more info on stdout)\n"
                    "       --help (Just print help info and abort)\n"
                    "\n" );
       }

    [[nodiscard]] const auto& jobs() const noexcept { return i_jobs; }
    [[nodiscard]] bool quiet() const noexcept { return i_quiet; }
    [[nodiscard]] bool verbose() const noexcept { return i_verbose; }


 private:
    std::vector<JobUnit> i_jobs;
    bool i_quiet = false;
    bool i_verbose = false;
};




//---------------------------------------------------------------------------
int main( const int argc, const char* const argv[] )
{
    try{
        Arguments args(argc, argv); // std::span(argv, argc)
        std::vector<std::string> issues;

        if( args.verbose() )
           {
            fmt::print( "---- m32-pars-adapt (ver. " __DATE__ ") ----\n" );
            fmt::print( "Running in: {}\n", fs::current_path().string() );
           }

        if( args.jobs().empty() )
           {
            throw std::invalid_argument("No jobs passed");
           }

        for( const auto& job : args.jobs() )
           {
            if( !job.is_complete() )
               {
                throw std::invalid_argument("Job not completely defined");
               }

            if( args.verbose() )
               {
                fmt::print("\n-------------------------------------------------\n");
                fmt::print("[{}]\n", job.info());
               }

            // [Target file]
            udt::File udt_file(job.target_file(), issues);
            if( args.verbose() )
               {
                fmt::print("UDT file: {}\n", udt_file.info());
               }


            // [Machine type]
            MachineType mach( job.machine_type() );
            if( args.verbose() )
               {
                fmt::print("Machine type: {}\n", mach.to_str());
               }

            // [Parameters DB]
            ParsDB pars_db;
            pars_db.parse( job.db_file(), issues );
            if( args.verbose() )
               {
                fmt::print( "Parameters DB: {}\n", pars_db.info() );
                //pars_db.print();
               }



            udt_file.modify_value("vqCo_Viscosity", "1223344");
            fs::path tmp_udt_pth{ job.target_file().parent_path() / fmt::format("~{}.tmp", job.target_file().filename().string()) };
            udt_file.write( tmp_udt_pth );
            winmerge::compare(job.target_file().string(), tmp_udt_pth.string());
            sys::sleep_ms(1500);
            sys::delete_file( tmp_udt_pth.string() );


            // Parse the parameter file
            //const std::string parfile_fullpath{ job.target_file().string() };

            //// Show file name and size
            //if( args.verbose() )
            //   {
            //    fmt::print("\nProcessing {}", file_fullpath);
            //   }
            //
            //const std::string file_basename{ file_path_obj.stem().string() };
            //plcb::Library lib( file_basename ); // This will refer to 'file_buf'!
            //
            //// Recognize by file extension
            //const std::string file_ext{ str::tolower(file_path_obj.extension().string()) };
            //if( file_ext == ".pll" )
            //   {// pll -> plclib
            //    parse_buffer(pll::parse, file_buf.as_string_view(), file_path_obj, file_fullpath, lib, args, issues);
            //  #ifdef PLL_TEST
            //    test_pll(file_basename, lib, args, issues);
            //  #else
            //    const fs::path out_plclib_pth{ args.output() / fmt::format("{}.plclib", file_basename) };
            //    write_plclib(lib, out_plclib_pth.string(), args);
            //  #endif
            //   }
            //else if( file_ext == ".h" )
            //   {// h -> pll,plclib
            //    parse_buffer(h::parse, file_buf.as_string_view(), file_path_obj, file_fullpath, lib, args, issues);
            //
            //    const fs::path out_pll_pth{ args.output() / fmt::format("{}.pll", file_basename) };
            //    write_pll(lib, out_pll_pth.string(), args);
            //
            //    const fs::path out_plclib_pth{ args.output() / fmt::format("{}.plclib", file_basename) };
            //    write_plclib(lib, out_plclib_pth.string(), args);
            //   }
            //else
            //   {
            //    throw std::runtime_error( fmt::format("Unhandled extension {} of {}"sv, file_ext, file_path_obj.filename().string()) );
            //   }
           }

        // ########
        //fmt::print("\n{}\n\n\n", json::create_test_tree().to_str());


        if( issues.size()>0 )
           {
            fmt::print("[!] {} issues found\n", issues.size());
            for( const auto& issue : issues )
               {
                fmt::print("    {}\n",issue);
               }
            return 1;
           }

        return 0;
       }

    catch( std::invalid_argument& e )
       {
        fmt::print("!! {}\n", e.what());
        Arguments::print_usage();
       }

    catch( std::exception& e )
       {
        fmt::print("!! Error: {}\n", e.what());
       }

    return 2;
}
