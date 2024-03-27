#include <stdexcept> // std::exception, std::invalid_argument
#include <print>

#include "arguments.hpp" // app::Arguments
#include "issues_collector.hpp" // MG::issues
#include "edit_text_file.hpp" // sys::edit_text_file()

#include "adapt_udt_file.hpp" // app::adapt_udt()
#include "adapt_parax_file.hpp" // app::adapt_parax()
#include "handle_output_file.hpp" // app::handle_output_file()


//---------------------------------------------------------------------------
int main( const int argc, const char* const argv[] )
{
    app::Arguments args;
    try{
        args.parse(argc, argv);

        const auto verbose_print = [verb=args.verbose()](const std::string_view msg, const auto&... args){ if(verb) std::vprint_unicode(msg, std::make_format_args(args...)); };

        verbose_print("---- {} (build " __DATE__ ") ----\n", app::name);

        MG::issues issues;

        if( args.job().is_update_udt() )
           {
            verbose_print("Updating {} using {}\n", args.job().db_file().path().string(), args.job().target_file().path().string());
            const bool same_mach =
            app::update_udt( args.job().target_file().path().string(),
                             args.job().db_file().path().string(),
                             args.job().out_path().string(),
                             args.options(),
                             verbose_print,
                             std::ref(issues) );
            const auto template_file = app::empty_if_or(not same_mach, args.job().target_file().path());
            app::handle_output_file( args.quiet(), args.job().out_path(), args.job().db_file().path(), template_file );
           }
        else if( args.job().is_adapt_udt() )
           {
            verbose_print("Adapting {} for {} basing on DB {}\n", args.job().target_file().path().filename().string(), args.job().mach_data().string(), args.job().db_file().path().filename().string());
            app::adapt_udt( args.job().target_file().path().string(),
                            args.job().db_file().path().string(),
                            args.job().out_path().string(),
                            args.job().mach_data(),
                            args.options(),
                            verbose_print,
                            std::ref(issues) );
            app::handle_output_file( args.quiet(), args.job().out_path(), args.job().target_file().path() );
           }
        else if( args.job().is_adapt_parax() )
           {
            verbose_print("Adapting {} for {} basing on DB {}\n", args.job().target_file().path().filename().string(), args.job().mach_data().string(), args.job().db_file().path().filename().string());
            app::adapt_parax( args.job().target_file().path().string(),
                              args.job().db_file().path().string(),
                              args.job().out_path().string(),
                              args.job().mach_data(),
                              args.options(),
                              verbose_print,
                              std::ref(issues) );
            app::handle_output_file( args.quiet(), args.job().out_path(), args.job().target_file().path() );
           }
        else
           {
            issues("Nothing to do");
           }

        if( issues.size()>0 )
           {
            for( const auto& issue : issues )
               {
                std::print("! {}\n", issue);
               }
            return 1;
           }

        return 0;
       }

    catch( std::invalid_argument& e )
       {
        std::print("!! {}\n", e.what());
        if( not args.quiet() )
           {
            args.print_usage();
           }
       }

    catch( parse::error& e)
       {
        std::print("!! [{}:{}] {}\n", e.file(), e.line(), e.what());
        if( not args.quiet() )
           {
            sys::edit_text_file( e.file(), e.line() );
           }
       }

    catch( std::exception& e )
       {
        std::print("!! {}\n", e.what());
       }

    return 2;
}
