#pragma once
//  ---------------------------------------------
//  Program arguments
//  ---------------------------------------------
//  #include "arguments.hpp" // app::Arguments
//  ---------------------------------------------
#include <string_view>
#include <filesystem> // std::filesystem
#include <format>
#include <print>
#include <stdexcept> // std::runtime_error, std::invalid_argument

#include "string_utilities.hpp" // str::to_lower
#include "args_extractor.hpp" // MG::args_extractor
#include "job_unit.hpp" // app::JobUnit
#include "options_set.hpp" // MG::options_set
#include "app_data.hpp" // app::name, app::descr

namespace fs = std::filesystem;
using namespace std::literals; // "..."sv

namespace app //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class Arguments final
{
 private:
    JobUnit m_job;
    MG::options_set m_options;
    std::string m_outpath;
    bool m_verbose = false; // More info to stdout
    bool m_quiet = false; // No user interaction

 public:
    [[nodiscard]] const auto& job() const noexcept { return m_job; }
    [[nodiscard]] const auto& options() const noexcept { return m_options; }
    [[nodiscard]] bool verbose() const noexcept { return m_verbose; }
    [[nodiscard]] bool quiet() const noexcept { return m_quiet; }

 public:
    //-----------------------------------------------------------------------
    void parse(const int argc, const char* const argv[])
       {
        try{
            MG::args_extractor args(argc, argv);
            args.apply_switch_by_name_or_char = [this](const std::string_view full_name, const char brief_name) { apply_switch(full_name,brief_name); };

            while( args.has_data() )
               {
                std::string_view arg = args.current();
                if( args.is_switch(arg) )
                   {
                    if( arg=="--to"sv or arg=="--out"sv or arg=="-o"sv or arg=="-out"sv )
                       {
                        const std::string_view str = args.get_next_value_of(arg);
                        if( not m_outpath.empty() )
                           {
                            throw std::invalid_argument( std::format("Output was already set to {}", m_outpath) );
                           }
                        m_outpath = str;
                       }
                    else if( arg=="--target"sv or arg=="--tgt"sv or arg=="-tgt"sv )
                       {
                        const std::string_view str = args.get_next_value_of(arg);
                        if( m_job.target_file() )
                           {
                            throw std::invalid_argument( std::format("Target file was already set to {}", m_job.target_file().path().string()) );
                           }
                        m_job.set_target_file(str);
                       }
                    else if( arg=="--db"sv or arg=="-db"sv )
                       {
                        const std::string_view str = args.get_next_value_of(arg);
                        if( m_job.db_file() )
                           {
                            throw std::invalid_argument( std::format("DB file was already set to {}", m_job.db_file().path().string()) );
                           }
                        m_job.set_db_file(str);
                       }
                    else if( arg=="--machine"sv or arg=="--mach"sv or arg=="-m"sv or arg=="-mach"sv )
                       {
                        const std::string_view str = args.get_next_value_of(arg);
                        if( m_job.mach_data() )
                           {
                            throw std::invalid_argument( std::format("Machine type was already set to {}", m_job.mach_data().string()) );
                           }
                        m_job.set_mach_data(str);
                       }
                    else if( arg=="--options"sv or arg=="-p"sv )
                       {
                        const std::string_view str = args.get_next_value_of(arg);
                        if( not options().is_empty() )
                           {
                            throw std::invalid_argument("Options already set");
                           }
                        m_options.assign(str);
                       }
                    else
                       {
                        args.apply_switch(arg);
                       }
                   }
                else
                   {
                    throw std::invalid_argument( std::format("Unexpected argument: {}",arg) );
                   }
                args.next();
               }

            check_and_postprocess();
           }
        catch( std::invalid_argument& )
           {
            throw;
           }
        catch( std::exception& e )
           {
            throw std::invalid_argument( e.what() );
           }
       }

    //-----------------------------------------------------------------------
    void check_and_postprocess()
       {
        m_job.detect_task();
        m_job.ensure_out_path(m_outpath);
       }

    //-----------------------------------------------------------------------
    static void print_help_and_exit()
       {
        std::print( "\n{} (build " __DATE__ ")\n"
                    "{}\n"
                    "\n", app::name, app::descr );
        throw std::invalid_argument{"Exiting after printing help"}; // Triggers print_usage()
       }

    //-----------------------------------------------------------------------
    static void print_usage()
       {
        std::print( "\nUsage:\n"
                    "   {0} --tgt path/to/MachSettings.udt --db path/to/msetts_pars.txt --mach ActiveW-4.9/4.6-(no-buf,opp)\n"
                    "   {0} --db path/to/old.udt --tgt path/to/new.udt\n"
                    "       --db <path> (Specify parameters database json file or original file)\n"
                    "       --help/-h (Print help info and abort)\n"
                    "       --machine/--mach/-m (Specify machine type string)\n"
                    "       --options/-p (Specify comma separated options: no-timestamp)\n"
                    "       --quiet/-q (No user interaction)\n"
                    "       --target/-tgt (Specify file to adapt or template)\n"
                    "       --to/--out/-o (Specify output file)\n"
                    "       --verbose/-v (Print more info on stdout)\n"
                    "\n", app::name );
       }

 private:
    //-----------------------------------------------------------------------
    void apply_switch(const std::string_view full_name, const char brief_name)
       {
        if( full_name=="verbose"sv or brief_name=='v' )
           {
            m_verbose = true;
           }
        else if( full_name=="quiet"sv or brief_name=='q' )
           {
            m_quiet = true;
           }
        else if( full_name=="help"sv or brief_name=='h' )
           {
            print_help_and_exit();
           }
        else
           {
            if( full_name.empty() ) throw std::invalid_argument{ std::format("Unknown switch: '{}'", brief_name) };
            else                    throw std::invalid_argument{ std::format("Unknown switch: \"{}\"", full_name) };
           }
       }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
