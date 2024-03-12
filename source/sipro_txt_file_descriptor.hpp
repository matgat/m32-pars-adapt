#pragma once
//  ---------------------------------------------
//  Sipro text file descriptor base classes
//  ---------------------------------------------
//  #include "sipro_txt_file_descriptor.hpp" // sipro::TxtFile
//  ---------------------------------------------
#include <vector>
#include <string>
#include <string_view>

#include "memory_mapped_file.hpp" // sys::memory_mapped_file
#include "output_streamable_concept.hpp" // MG::OutputStreamable
#include "file_write.hpp" // sys::file_write()
#include "timestamp.hpp" // MG::get_human_readable_timestamp()
#include "app_data.hpp" // app::name
#include "options_set.hpp" // MG::options_set


namespace sipro //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
class TxtField final
{
 private:
    std::string_view m_name;
    std::string_view m_value;
    std::string m_mod_val;
    std::string_view m_comment;
    std::string_view m_label;
    std::size_t m_line_idx;

 public:
    explicit TxtField( const std::string_view nam,
                       const std::string_view val,
                       const std::string_view cmt,
                       const std::string_view lbl,
                       const std::size_t ln_idx )
      : m_name(nam)
      , m_value(val)
      , m_comment(cmt)
      , m_label(lbl)
      , m_line_idx(ln_idx)
       {}

    [[nodiscard]] std::string_view var_name() const noexcept { return m_name; }

    [[nodiscard]] std::string_view value() const noexcept { return m_mod_val.empty() ? m_value : m_mod_val; }
    [[nodiscard]] bool is_value_modified() const noexcept { return not m_mod_val.empty(); }
    void modify_value(const std::string_view new_val) { m_mod_val = new_val; }

    [[nodiscard]] constexpr bool has_comment() const noexcept { return not m_comment.empty(); }
    [[nodiscard]] constexpr std::string_view comment() const noexcept { return m_comment; }

    [[nodiscard]] constexpr bool has_label() const noexcept { return not m_label.empty(); }
    [[nodiscard]] constexpr std::string_view label() const noexcept { return m_label; }

    [[nodiscard]] std::size_t line_index() const noexcept { return m_line_idx; }
};


/////////////////////////////////////////////////////////////////////////////
class TxtLine final
{
 private:
    std::string_view m_content;
    TxtField* m_associated_field;

 public:
    explicit TxtLine(const std::string_view l, TxtField* const p =nullptr)
      : m_content(l)
      , m_associated_field(p)
       {}

    [[nodiscard]] std::string_view content() const noexcept { return m_content; }

    [[nodiscard]] const TxtField* associated_field() const noexcept { return m_associated_field; }
    [[nodiscard]] TxtField* associated_field() noexcept { return m_associated_field; }
};


/////////////////////////////////////////////////////////////////////////////
class TxtFile
{
 private:
    const std::string m_path;
    const sys::memory_mapped_file m_file_buf;
    std::vector<std::string> m_mod_issues; // Modifications problems
 protected:
    std::vector<TxtLine> m_lines;

 public:
    explicit TxtFile(const std::string& pth)
      : m_path{pth}
      , m_file_buf{m_path.c_str()}
       {}

    [[nodiscard]] std::string_view buf() const noexcept { return m_file_buf.as_string_view(); }
    [[nodiscard]] const std::string& path() const noexcept { return m_path; }

    [[nodiscard]] const std::vector<std::string>& mod_issues() const noexcept { return m_mod_issues; }
    void add_mod_issue(std::string&& issue) { m_mod_issues.push_back( std::move(issue) ); }

    //-----------------------------------------------------------------------
    void write_to(const std::string& pth, const MG::options_set& options, const std::string_view add_info ={})
       {
        sys::file_write fw( pth.c_str() );
        write_to(fw, options, add_info);
       }
    void write_to(MG::OutputStreamable auto& fw, const MG::options_set& options, const std::string_view add_info)
       {
        const std::string_view endline = not m_lines.empty() and
                                         m_lines.front().content().length()>1 and
                                         m_lines.front().content()[m_lines.front().content().length()-2]=='\r'
                                         ? "\r\n"sv : "\n"sv;
        bool block_comment_notyetfound = true;
        for( const auto& line : m_lines )
           {
            const TxtField* const field = line.associated_field();
            if( field and field->is_value_modified() )
               {// This line is a field with modified value, reconstructing the line
                // Detect indentation
                const std::ptrdiff_t indent_len = field->var_name().data() - line.content().data();
                assert(indent_len>=0);
                fw << line.content().substr(0u, static_cast<std::size_t>(indent_len))
                   << field->var_name()
                   << " = "sv
                   << field->value();

                if( field->has_comment() )
                   {
                    fw << " # "sv << field->comment();
                    if( field->has_label() )
                       {
                        fw << " '"sv << field->label() << '\'';
                       }
                   }

                fw << endline;
               }
            else if( block_comment_notyetfound and line.content().contains("[EndNote]") )
               {
                block_comment_notyetfound = false;
                // Adding some info on generated file
                fw << "    "sv;
                if( not options.contains("no-timestamp"sv) )
                   {
                    fw << MG::get_human_readable_timestamp() << ' ';
                   }
                fw << app::name << ", "sv << std::to_string(mod_issues().size()) << " issues"sv << endline;
                if( not add_info.empty() )
                   {
                    fw << "    "sv << add_info << endline;
                   }
                for( const auto& issue : mod_issues() )
                   {
                    fw << "    ! "sv << issue << endline;
                   }
                fw << line.content();
               }
            else
               {// Write original unmodified line
                fw << line.content();
               }
           }
       }

 protected:
    //-----------------------------------------------------------------------
    [[nodiscard]] static auto extract_comment_label(const std::string_view sv) noexcept
       {// "comment 'label'" => "comment", "label"
        std::pair<std::string_view,std::string_view> cmt_lbl = {sv, {}};
        if( sv.ends_with('\'') and sv.size()>=2u )
           {
            if( const std::size_t apos = sv.rfind('\'', sv.size()-2u);
                apos!=std::string_view::npos )
               {
                cmt_lbl.first = str::trim_right(sv.substr(0u, apos));
                cmt_lbl.second = sv.substr(apos+1u, sv.size()-2u-apos);
               }
           }
        return cmt_lbl;
       }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
