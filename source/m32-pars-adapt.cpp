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
#include "string-utilities.hpp" // str::tolower, str::unquoted

#include "text-files-tools.hpp" // sys::edit_text_file, sys::compare
#include "machine-type.hpp" // macotec::MachineType
#include "extract-db.hpp" // macotec::extract_db
#include "pars-db.hpp" // ParsDB
#include "udt-file.hpp" // udt::File

using namespace std::literals; // "..."sv



/////////////////////////////////////////////////////////////////////////////
class JobUnit final
{
    /////////////////////////////////////////////////////////////////////////
    class file_t final
       {
        private:
            enum class file_type : std::uint8_t
               {
                unknown=0, // !Consecutive indexes!
                udt, // MachSettings.udt, ParDefaults.udt
                parax, // par2kax.txt
                //par, // Sipro parameter file (par2k*.txt)
                txt // generic text (could be a json db or a sipro parameter file)
               };

        public:
            [[nodiscard]] operator bool() const noexcept { return !i_path.empty(); }

            [[nodiscard]] const auto& path() const noexcept { return i_path; }

            void assign( const std::string_view s )
               {
                i_path = s;
                if( !fs::exists(i_path) )
                   {
                    throw std::invalid_argument( fmt::format("File doesn't exists: {}",s) );
                   }

                // Recognize the file type
                const std::string fnam{ str::tolower(i_path.filename().string()) };
                const std::string ext{ str::tolower(i_path.extension().string()) };
                     if( ext==".udt" ) i_type = file_type::udt;
                else if( fnam.starts_with("par2kax") ) i_type = file_type::parax;
                //else if( fnam.starts_with("par2k") && ext==".txt" ) i_type = file_type::par;
                else if( ext==".txt" ) i_type = file_type::txt;
                else throw std::invalid_argument( fmt::format("Unrecognized file: {}",s) );
               }

            [[nodiscard]] bool is_udt() const noexcept { return i_type==file_type::udt; }
            [[nodiscard]] bool is_parax() const noexcept { return i_type==file_type::parax; }
            [[nodiscard]] bool is_txt() const noexcept { return i_type==file_type::txt; }

        private:
            fs::path i_path;
            file_type i_type = file_type::unknown;
       };

 public:
    [[nodiscard]] const auto& machine_type() const noexcept { return i_machinetype; }
    void set_machine_type(const std::string_view s) { i_machinetype.assign(s); }

    [[nodiscard]] const auto& target_file() const noexcept { return i_targetfile; }
    void set_target_file(const std::string_view s) { i_targetfile.assign(s); }

    [[nodiscard]] const auto& db_file() const noexcept { return i_dbfile; }
    void set_db_file(const std::string_view s) { i_dbfile.assign(s); }

 private:
    macotec::MachineType i_machinetype;
    file_t i_targetfile;
    file_t i_dbfile;
};


/////////////////////////////////////////////////////////////////////////////
class Arguments final
{
 public:
    Arguments(const int argc, const char* const argv[]) // const std::span args
       {
        try{
            enum class STS
               {
                SEE_ARG,
                GET_MACHTYPE,
                GET_TGTFILEPATH,
                GET_DBFILEPATH
               } status = STS::SEE_ARG;

            //for( const auto arg : args | std::views::transform([](const char* const a){ return std::string_view(a);}) )
            for( int i=1; i<argc; ++i )
               {
                const std::string_view arg{ argv[i] };
                switch( status )
                   {
                    case STS::GET_MACHTYPE :
                        if( i_job.machine_type() )
                           {
                            throw std::invalid_argument( fmt::format("Machine type was already set to {}",i_job.machine_type().string()) );
                           }
                        i_job.set_machine_type(arg);
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_TGTFILEPATH :
                        if( i_job.target_file() )
                           {
                            throw std::invalid_argument( fmt::format("Target file was already set to {}",i_job.target_file().path().string()) );
                           }
                        i_job.set_target_file(arg);
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_DBFILEPATH :
                        if( i_job.db_file() )
                           {
                            throw std::invalid_argument( fmt::format("DB file was already set to {}",i_job.db_file().path().string()) );
                           }
                        i_job.set_db_file(arg);
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
                                status = STS::GET_TGTFILEPATH;
                               }
                            else if( swtch=="db"sv )
                               {
                                status = STS::GET_DBFILEPATH;
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
                                throw std::invalid_argument( fmt::format("Unknown command switch: {}",swtch) );
                               }
                           }
                        else
                           {
                            throw std::invalid_argument( fmt::format("Unrecognized argument: {}",arg) );
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
                    "   m32-pars-adapt --verbose --tgt path/to/MachSettings.udt --db path/to/msetts_pars.txt --machine ActiveW-4.9/4.6\n"
                    "       --machine <string> (Machine type string)\n"
                    "       --target <path> (Parameter file to adapt)\n"
                    "       --db <path> (Parameters database json file)\n"
                    "       --quiet (Force actions, no manual checks)\n"
                    "       --verbose (Print more info on stdout)\n"
                    "       --help (Just print help info and abort)\n"
                    "\n" );
       }

    [[nodiscard]] const auto& job() const noexcept { return i_job; }
    [[nodiscard]] auto& modify_job() noexcept { return i_job; }
    [[nodiscard]] bool quiet() const noexcept { return i_quiet; }
    [[nodiscard]] bool verbose() const noexcept { return i_verbose; }

 private:
    JobUnit i_job;
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

        // Check job data
        // Target abd DB files must always be given
        if( !args.job().target_file() )
           {
            throw std::invalid_argument("No target file specified");
           }
        if( !args.job().db_file() )
           {
            throw std::invalid_argument("No DB file specified");
           }

        // Recognize cases
        //====================================================================
        if( args.job().target_file().is_udt() && args.job().db_file().is_txt() )
           {// Adapting an udt file given overlays database and machine type

            // [The UDT file to adapt]
            udt::File udt_file(args.job().target_file().path(), issues);

            // [Machine type]
            // If machine type is not given, I'll try to extract it from the udt file
            if( args.job().machine_type() )
               {// Ensure to set the specified machine type in the file
                udt_file.modify_value_if_present("vaMachName", str::quoted(args.job().machine_type().string()));
               }
            else
               {// Extract machine type from udt file
                const std::string_view s = str::unquoted( udt_file.get_value_of("vaMachName") );
                args.modify_job().set_machine_type(s);
               }

            // [Parameters DB]
            ParsDB pars_db;
            pars_db.parse( args.job().db_file().path(), issues );
            // Extract the pertinent data for this machine
            const auto mach_db = macotec::extract_db(pars_db.root(), args.job().machine_type(), issues);

            // [Summarize the job]
            if( args.verbose() )
               {
                fmt::print("Adapting {} for {} basing on DB {}\n", args.job().target_file().path().filename().string(), args.job().machine_type().string(), args.job().db_file().path().filename().string());
                fmt::print("  .UDT file: {}\n", udt_file.info());
                fmt::print("  .Parameters DB: {}\n", pars_db.info());
                //pars_db.print();
               }

            // Overwrite values from database
            for( const auto group : mach_db )
               {
                for( const auto& [nam, node] : group.get().childs() )
                   {
                    if( !node.has_value() )
                       {
                        issues.push_back( fmt::format("Node {} hasn't a value", nam) );
                       }
                    //fmt::print("{}={}\n",nam,node.value());

                    if( const auto field = udt_file.get_field(nam) )
                       {
                        field->modify_value(node.value());
                       }
                    else
                       {
                        udt_file.add_issue( fmt::format("Not found: {}={}",nam,node.value()) );
                       }
                   }
               }

            const fs::path tmp_udt_pth{ args.job().target_file().path().parent_path() / fmt::format("~{}.tmp", args.job().target_file().path().filename().string()) };
            udt_file.write( tmp_udt_pth );
            if( args.verbose() )
               {
                fmt::print("  .Modified {} values, {} issues\n", udt_file.modified_values_count(), udt_file.issues_count());
               }

            if( args.quiet() )
               {// No user intervention
                sys::backup_file_same_dir( args.job().target_file().path() );
                fs::remove( args.job().target_file().path() );
                fs::rename( tmp_udt_pth, args.job().target_file().path() );
               }
            else
               {// Manual merge
                sys::compare(tmp_udt_pth.string().c_str(), args.job().target_file().path().string().c_str());
                sys::sleep_ms(2000);
                fs::remove( tmp_udt_pth );
               }
           }

        //====================================================================
        else if( args.job().target_file().is_udt() && args.job().db_file().is_udt() )
           {// Updating an old udt file (db) to a newer one (target)

            // [Machine type]
            // The machine type shouldn't be explicitly given
            if( args.job().machine_type() )
               {
                throw std::invalid_argument( "Machine type shouldn't be specified for an UDT update" );
               }

            // [The template UDT file (newest)]
            udt::File new_udt_file(args.job().target_file().path(), issues);

            // [The original UDT file to upgrade (oldest)]
            udt::File old_udt_file(args.job().db_file().path(), issues);

            // [Summarize the job]
            if( args.verbose() )
               {
                fmt::print("Upgrading {} using {}\n", old_udt_file.path(), new_udt_file.path());
                fmt::print("  .Old UDT: {}\n", old_udt_file.info());
                fmt::print("  .New UDT: {}\n", new_udt_file.info());
               }

            // Overwrite values in newest file using the old as database
            new_udt_file.overwrite_values_from( old_udt_file );

            const fs::path tmp_udt_pth{ args.job().db_file().path().parent_path() / fmt::format("~{}.tmp", args.job().db_file().path().filename().string()) };
            new_udt_file.write( tmp_udt_pth );
            if( args.verbose() )
               {
                fmt::print("  .Modified {} values, {} issues\n", new_udt_file.modified_values_count(), new_udt_file.issues_count());
               }

            if( args.quiet() )
               {// No user intervention
                sys::backup_file_same_dir( args.job().db_file().path() );
                fs::remove( args.job().db_file().path() );
                fs::rename( tmp_udt_pth, args.job().db_file().path() );
               }
            else
               {// Manual merge
                sys::compare( args.job().target_file().path().string().c_str(),
                              tmp_udt_pth.string().c_str(),
                              args.job().db_file().path().string().c_str() );
                sys::sleep_ms(2000);
                fs::remove( tmp_udt_pth );
               }
           }

        //====================================================================
        //else if( args.job().target_file().is_parax() && args.job().db_file().is_txt() )
        //   {// Adapting a par2kax.txt file given overlays database and machine type
        //   }

        else
           {
            throw std::invalid_argument( fmt::format("Don't know what to do with {} and {}", args.job().target_file().path().filename().string(), args.job().db_file().path().filename().string()) );
           }


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

    catch( parse_error& e)
       {
        fmt::print("!! {}\n", e.what());
        sys::edit_text_file( e.file_path(), e.line(), e.pos() );
       }

    catch( std::exception& e )
       {
        fmt::print("!! Error: {}\n", e.what());
       }

    return 2;
}
