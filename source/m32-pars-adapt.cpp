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
#include "text-files-tools.hpp" // sys::edit_text_file, sys::compare_wait

#include "machine-type.hpp" // macotec::MachineType
#include "extract-mach-db.hpp" // macotec::extract_mach_*_db
#include "pars-db.hpp" // ParsDB
#include "udt-file.hpp" // udt::File
#include "parax-file.hpp" // parax::File

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
                    throw std::invalid_argument( fmt::format("File not found: {}",s) );
                   }

                // Recognize the file type
                const std::string fnam{ str::tolower(i_path.filename().string()) };
                const std::string ext{ str::tolower(i_path.extension().string()) };
                if( ext==".udt" )
                   {
                    i_type = file_type::udt;
                   }
                else if( fnam=="par2kax.txt" )
                   {
                    i_type = file_type::parax;
                   }
                else if( ext==".txt" )
                   {
                    i_type = file_type::txt;
                   }
                else
                   {
                    throw std::invalid_argument( fmt::format("Unrecognized file: {}",s) );
                   }
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

    [[nodiscard]] const auto& out_file_name() const noexcept { return i_out_file_name; }
    void set_out_file_name(const std::string_view s) { i_out_file_name = s; }

 private:
    macotec::MachineType i_machinetype;
    file_t i_targetfile;
    file_t i_dbfile;
    std::string i_out_file_name;
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
                GET_DBFILEPATH,
                GET_OUTFILENAM
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

                    case STS::GET_OUTFILENAM :
                        if( !i_job.out_file_name().empty() )
                           {
                            throw std::invalid_argument( fmt::format("Output file name was already set to {}",i_job.out_file_name()) );
                           }
                        i_job.set_out_file_name(arg);
                        status = STS::SEE_ARG;
                        break;

                    default :
                        if( arg.length()>1 && arg[0]=='-' )
                           {// A command switch!
                            // Skip hyphen, tolerate also doubled ones
                            const std::size_t skip = arg[1]=='-' ? 2 : 1;
                            const std::string_view swtch{ arg.data()+skip, arg.length()-skip};
                            if( swtch=="machine"sv || swtch=="mach"sv || swtch=="m"sv )
                               {
                                status = STS::GET_MACHTYPE;
                               }
                            else if( swtch=="target"sv || swtch=="tgt"sv )
                               {
                                status = STS::GET_TGTFILEPATH;
                               }
                            else if( swtch=="db"sv )
                               {
                                status = STS::GET_DBFILEPATH;
                               }
                            else if( swtch=="out"sv || swtch=="o"sv )
                               {
                                status = STS::GET_OUTFILENAM;
                               }
                            else if( swtch=="quiet"sv || swtch=="q"sv )
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
                    "   m32-pars-adapt --tgt path/to/MachSettings.udt --db path/to/msetts_pars.txt --mach ActiveW-4.9/4.6-(no-buf,opp)\n"
                    "       --db <path> (Parameters database json file)\n"
                    "       --help/-h (Just print help info and abort)\n"
                    "       --machine/-mach/-m <string> (Machine type string)\n"
                    "       --out/-o <string> (Specify output file name, keep file)\n"
                    "       --quiet/-q (Substitute file without manual merge)\n"
                    "       --target/-tgt <path> (Parameter file to adapt)\n"
                    "       --verbose/-v (Print more info on stdout)\n"
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
[[nodiscard]] fs::path get_out_path(const fs::path& orig_path, const Arguments& args)
{
    fs::path out_path{ orig_path.parent_path() };
    if( args.job().out_file_name().empty() )
       {
        out_path /= fmt::format("~{}.tmp", orig_path.filename().string());
       }
    else
       {
        out_path /= args.job().out_file_name();
        if( fs::equivalent(out_path, orig_path) )
           {
            throw std::runtime_error( fmt::format("Specified output file \"{}\" collides with original file",args.job().out_file_name()) );
           }
       }
    return out_path ;
}


//---------------------------------------------------------------------------
[[nodiscard]] fs::path update_udt(const Arguments& args, std::vector<std::string>& issues)
{
    // [Machine type]
    // The machine type shouldn't be explicitly given
    if( args.job().machine_type() )
       {
        throw std::invalid_argument( "Machine type shouldn't be specified for an UDT update" );
       }

    // [The template UDT file (newest)]
    udt::File new_udt_file(args.job().target_file().path(), issues);

    // [The original UDT file to upgrade (oldest)]
    const udt::File old_udt_file(args.job().db_file().path(), issues);

    // [Summarize the job]
    if( args.verbose() )
       {
        fmt::print("Updating {} using {}\n", old_udt_file.path(), new_udt_file.path());
        fmt::print("  .Old UDT: {}\n", old_udt_file.info());
        fmt::print("  .New UDT: {}\n", new_udt_file.info());
       }

    // Overwrite values in newest file using the old as database
    new_udt_file.overwrite_values_from( old_udt_file );

    if( args.verbose() )
       {
        fmt::print("  .Modified {} values, {} issues\n", new_udt_file.modified_values_count(), new_udt_file.mod_issues_count());
       }

    // Write output file
    fs::path out_pth = get_out_path(args.job().db_file().path(), args);
    new_udt_file.write( out_pth );
    return out_pth;
}


//---------------------------------------------------------------------------
[[nodiscard]] fs::path adapt_udt(Arguments& args, std::vector<std::string>& issues)
{
    // [The UDT file to adapt]
    udt::File udt_file(args.job().target_file().path(), issues);

    // [Machine type]
    if( args.job().machine_type() )
       {// Machine type specified
        if( const auto vaMachName = udt_file.get_field("vaMachName") )
           {
            // Better check that the target file has already superimposed options
            macotec::MachineType udt_mach_type;
            try{
                udt_mach_type.assign( str::unquoted(vaMachName->value()) );
               }
            catch( std::exception& e )
               {
                issues.push_back( fmt::format("{} has an invalid vaMachName \"{}\": {}", args.job().target_file().path().filename().string(), vaMachName->value(), e.what()) );
               }
            if( !udt_mach_type.options().is_empty() )
               {
                throw std::runtime_error( fmt::format("{} had already options: {}", args.job().target_file().path().filename().string(), udt_mach_type.options().string()) );
               }
            // Overwrite the specified machine type in the file
            vaMachName->modify_value( str::quoted(args.job().machine_type().string()) );
           }
        else
           {
            issues.push_back( fmt::format("{} hasn't field vaMachName", args.job().target_file().path().filename().string()) );
           }
       }
    else
       {// Machine type not explicitly specified
        // Ask for machine type
        //args.modify_job().set_machine_type( macotec::MachineType::ask_user() );
        // Extract machine type from udt file
        if( const auto vaMachName = udt_file.get_field("vaMachName") )
           {
            args.modify_job().set_machine_type( str::unquoted(vaMachName->value()) );
           }
        else
           {
            throw std::runtime_error( fmt::format("Can't infer machine from {}", args.job().target_file().path().filename().string()) );
           }
       }

    // [Parameters DB]
    ParsDB udt_db;
    udt_db.parse( args.job().db_file().path(), issues );
    // Extract the pertinent data for this machine
    const auto mach_udt_db = macotec::extract_mach_udt_db(udt_db.root(), args.job().machine_type(), issues);

    // [Summarize the job]
    if( args.verbose() )
       {
        fmt::print("Adapting {} for {} basing on DB {}\n", args.job().target_file().path().filename().string(), args.job().machine_type().string(), args.job().db_file().path().filename().string());
        fmt::print("  .udt file: {}\n", udt_file.info());
        fmt::print("  .DB: {}\n", udt_db.info());
        //udt_db.print();
       }

    // Overwrite values from database
    for( const auto group_ref : mach_udt_db )
       {
        for( const auto& [nam, db_field] : group_ref.get().childs() )
           {
            if( !db_field.has_value() )
               {
                issues.push_back( fmt::format("Node {} hasn't a value in {}", nam, args.job().db_file().path().string()) );
               }
            else if( const auto udt_field = udt_file.get_field(nam) )
               {
                udt_field->modify_value(db_field.value());
               }
            else
               {
                udt_file.add_mod_issue( fmt::format("Not found: {}={}",nam,db_field.value()) );
               }
           }
       }

    if( args.verbose() )
       {
        fmt::print("  .Modified {} values, {} issues\n", udt_file.modified_values_count(), udt_file.mod_issues_count());
       }

    // Write output file
    fs::path out_pth = get_out_path(args.job().target_file().path(), args);
    udt_file.write( out_pth );
    return out_pth;
}


//---------------------------------------------------------------------------
[[nodiscard]] fs::path adapt_parax(const Arguments& args, std::vector<std::string>& issues)
{
    // [The parax file to adapt]
    parax::File parax_file(args.job().target_file().path(), issues);

    // [Machine type]
    if( !args.job().machine_type() )
       {// Machine type not explicitly specified
        // Ask for machine type
        //args.modify_job().set_machine_type( macotec::MachineType::ask_user() );
        throw std::invalid_argument("Machine not specified");
       }

    // [Parameters DB]
    ParsDB parax_db;
    parax_db.parse( args.job().db_file().path(), issues );
    // Extract the pertinent data for this machine
    const auto mach_parax_db = macotec::extract_mach_parax_db(parax_db.root(), args.job().machine_type(), issues);

    // [Summarize the job]
    if( args.verbose() )
       {
        fmt::print("Adapting {} for {} basing on DB {}\n", args.job().target_file().path().filename().string(), args.job().machine_type().string(), args.job().db_file().path().filename().string());
        fmt::print("  .parax file: {}\n", parax_file.info());
        fmt::print("  .DB: {}\n", parax_db.info());
        //parax_db.print();
       }

    // Overwrite values from database
    for( const auto& [axid, db_axfields] : mach_parax_db )
       {
        if( const auto par_ax_fields = parax_file.get_axfields(axid) )
           {
            for( const auto group_ref : db_axfields )
               {
                for( const auto& [nam, db_field] : group_ref.get().childs() )
                   {
                    fmt::print(">>> {}.{}={}\n",axid,nam,db_field.value());
                    if( !db_field.has_value() )
                       {
                        issues.push_back( fmt::format("Axis field {}.{} hasn't a value in {}", axid, nam, args.job().db_file().path().string()) );
                       }
                    else if( const auto par_field = parax_file.get_field(*par_ax_fields,nam) )
                       {
                        par_field->modify_value(db_field.value());
                       }
                    else
                       {
                        parax_file.add_mod_issue( fmt::format("Axis parameter not found: {}={}",nam,db_field.value()) );
                       }
                   }
               }
           }
        else
           {
            parax_file.add_mod_issue( fmt::format("Axis not found: {}",axid) );
           }
       }

    if( args.verbose() )
       {
        fmt::print("  .Modified {} values, {} issues\n", parax_file.modified_values_count(), parax_file.mod_issues_count());
       }

    // Write output file
    fs::path out_pth = get_out_path(args.job().target_file().path(), args);
    parax_file.write( out_pth, args.job().machine_type().string() );
    return out_pth;
}


//---------------------------------------------------------------------------
void handle_adapted_file(const fs::path& out_pth, const Arguments& args)
{
    if( args.quiet() )
       {// No user intervention
        if( args.job().out_file_name().empty() )
           {
            sys::backup_file_same_dir( args.job().target_file().path() );
            fs::remove( args.job().target_file().path() );
            fs::rename( out_pth, args.job().target_file().path() );
           }
       }
    else
       {// Manual merge
        sys::compare_wait(out_pth.string().c_str(), args.job().target_file().path().string().c_str());

        if( args.job().out_file_name().empty() )
           {
            fs::remove( out_pth );
           }
       }
}


//---------------------------------------------------------------------------
void handle_updated_file(const fs::path& out_pth, const Arguments& args)
{
    if( args.quiet() )
       {// No user intervention
        if( args.job().out_file_name().empty() )
           {// Output file is a temporary
            sys::backup_file_same_dir( args.job().db_file().path() );
            fs::remove( args.job().db_file().path() );
            fs::rename( out_pth, args.job().db_file().path() );
           }
       }
    else
       {// Manual merge
        // Compare updated with the original
        sys::compare_wait(  out_pth.string().c_str(),
                            args.job().db_file().path().string().c_str()  );
        // Compare the template with the merged one
        sys::compare_wait(  args.job().target_file().path().string().c_str(),
                            args.job().db_file().path().string().c_str()  );
        // Compare all three
        //sys::compare_wait(  args.job().target_file().path().string().c_str(),
        //                    out_pth.string().c_str(),
        //                    args.job().db_file().path().string().c_str()  );

        if( args.job().out_file_name().empty() )
           {// Output file is a temporary
            fs::remove( out_pth );
           }
       }
}


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

        //====================================================================
        if( args.job().target_file().is_udt() && args.job().db_file().is_udt() )
           {// Updating an old udt file (db) to a newer one (target)
            const fs::path out_pth = update_udt(args, issues);
            handle_updated_file(out_pth, args);
           }

        //====================================================================
        else if( args.job().target_file().is_udt() && args.job().db_file().is_txt() )
           {// Adapting an udt file given overlays database and machine type
            const fs::path out_pth = adapt_udt(args, issues);
            handle_adapted_file(out_pth, args);
           }

        //====================================================================
        else if( args.job().target_file().is_parax() && args.job().db_file().is_txt() )
           {// Adapting a par2kax.txt file given overlays database and machine type
            const fs::path out_pth = adapt_parax(args, issues);
            handle_adapted_file(out_pth, args);
           }

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
