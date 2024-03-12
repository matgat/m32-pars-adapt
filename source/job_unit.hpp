#pragma once
//  ---------------------------------------------
//  Program arguments
//  ---------------------------------------------
//  #include "job_unit.hpp" // app::JobUnit
//  ---------------------------------------------
#include <cstdint> // std::uint8_t
#include <string_view>
#include <filesystem> // std::filesystem
#include <stdexcept> // std::runtime_error

#include "macotec_machine_data.hpp" // macotec::MachineData

namespace fs = std::filesystem;


namespace app //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class JobUnit final
{
    /////////////////////////////////////////////////////////////////////////
    class file_t final
    {
        enum class file_type : std::uint8_t
           {
            unknown,
            udt, // MachSettings.udt, ParDefaults.udt
            parax, // par2kax.txt
            txt // generic text (could be a json db or a sipro parameter file)
           };

     private:
        fs::path m_path;
        file_type m_type = file_type::unknown;

     public:
        [[nodiscard]] explicit operator bool() const noexcept { return !m_path.empty(); }

        [[nodiscard]] const auto& path() const noexcept { return m_path; }

        void assign( const std::string_view sv )
           {
            m_path = sv;
            if( not fs::exists(m_path) )
               {
                throw std::runtime_error( std::format("File not found: {}", sv) );
               }

            // Recognize the file type
            const std::string fnam{ str::to_lower(m_path.filename().string()) };
            const std::string ext{ str::to_lower(m_path.extension().string()) };
            if( ext==".udt" )
               {
                m_type = file_type::udt;
               }
            else if( fnam=="par2kax.txt" )
               {
                m_type = file_type::parax;
               }
            else if( ext==".txt" )
               {
                m_type = file_type::txt;
               }
            else
               {
                throw std::runtime_error( std::format("Unrecognized file: {}", sv) );
               }
           }

        [[nodiscard]] bool is_udt() const noexcept { return m_type==file_type::udt; }
        [[nodiscard]] bool is_parax() const noexcept { return m_type==file_type::parax; }
        [[nodiscard]] bool is_txt() const noexcept { return m_type==file_type::txt; }
    };

    enum class task_type : std::uint8_t
       {
        unknown,
        update_udt, // Updating an old udt file (db) to a newer one (target)
        adapt_udt,  // Adapting an udt file given overlays database and machine type
        adapt_parax // Adapting a par2kax.txt file given overlays database and machine type
       };

 private:
    macotec::MachineData m_mach_data;
    file_t m_targetfile;
    file_t m_dbfile;
    fs::path m_outpath;
    task_type m_task = task_type::unknown;

 public:
    [[nodiscard]] const auto& mach_data() const noexcept { return m_mach_data; }
    void set_mach_data(const std::string_view sv) { m_mach_data.assign(sv); }

    [[nodiscard]] const auto& target_file() const noexcept { return m_targetfile; }
    void set_target_file(const std::string_view sv) { m_targetfile.assign(sv); }

    [[nodiscard]] const auto& db_file() const noexcept { return m_dbfile; }
    void set_db_file(const std::string_view sv) { m_dbfile.assign(sv); }

    [[nodiscard]] const auto& out_path() const noexcept { return m_outpath; }
    void set_out_path(const fs::path& orig_path, const std::string_view outpth)
       {
        if( outpth.empty() )
           {
            m_outpath = orig_path.parent_path();
            m_outpath /= std::format("~{}.tmp"sv, orig_path.filename().string());
           }
        else
           {
            m_outpath = outpth;
            if( fs::exists(m_outpath) and (fs::equivalent(m_outpath, target_file().path()) or fs::equivalent(m_outpath, db_file().path())) )
               {
                throw std::invalid_argument( std::format("Specified output \"{}\" collides with input file", outpth) );
               }
           }
       }

    void detect_task()
       {
        if( target_file().is_udt() and db_file().is_udt() )
           {
            m_task = task_type::update_udt;
           }
        else if( target_file().is_udt() and db_file().is_txt() )
           {
            m_task = task_type::adapt_udt;
           }
        else if( target_file().is_parax() and db_file().is_txt() )
           {
            m_task = task_type::adapt_parax;
           }
        else
           {
            throw std::invalid_argument("Don't know what to do with the given files");
           }
       }

    void ensure_out_path(const std::string_view outpth)
       {
        if( is_update_udt() )
           {
            if( mach_data() )
               {
                throw std::invalid_argument( "Machine shouldn't be specified for a UDT update" );
               }
            set_out_path(db_file().path(), outpth);
           }
        else if( is_adapt_udt() or is_adapt_parax() )
           {
            if( not mach_data() )
               {
                throw std::invalid_argument("Machine not specified");
               }
            else if( mach_data().is_incomplete() )
               {
                throw std::invalid_argument( std::format("Machine data incomplete: {}", mach_data().string()) );
               }
            set_out_path(target_file().path(), outpth);
           }
       }

    [[nodiscard]] bool is_update_udt() const noexcept { return m_task == task_type::update_udt; }
    [[nodiscard]] bool is_adapt_udt() const noexcept { return m_task == task_type::adapt_udt; }
    [[nodiscard]] bool is_adapt_parax() const noexcept { return m_task == task_type::adapt_parax; }
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
