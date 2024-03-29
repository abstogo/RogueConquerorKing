// Copyright 2015 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CSV_CSV_PARSER_HPP
#define JSONCONS_CSV_CSV_PARSER_HPP

#include <memory> // std::allocator
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <cctype>
#include <jsoncons/json_exception.hpp>
#include <jsoncons/json_content_handler.hpp>
#include <jsoncons/json_reader.hpp>
#include <jsoncons/json_content_filter.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons/detail/parse_number.hpp>
#include <jsoncons_ext/csv/csv_error.hpp>
#include <jsoncons_ext/csv/csv_options.hpp>

namespace jsoncons { namespace csv {

enum class csv_mode 
{
    initial,
    header,
    data,
    subfields
};

enum class csv_parse_state 
{
    start,
    cr, 
    column_labels,
    expect_comment_or_record,
    expect_record,
    end_record,
    no_more_records,
    comment,
    between_values,
    quoted_string,
    unquoted_string,
    before_unquoted_string,
    escaped_value,
    minus, 
    zero,  
    integer,
    fraction,
    exp1,
    exp2,
    exp3,
    before_done,
    before_unquoted_field,
    before_unquoted_field_tail, 
    before_unquoted_field_tail1,
    before_last_unquoted_field,
    before_last_unquoted_field_tail,
    before_unquoted_subfield,
    before_unquoted_subfield_tail,
    before_quoted_subfield,
    before_quoted_subfield_tail,
    before_quoted_field,
    before_quoted_field_tail,
    before_last_quoted_field,
    before_last_quoted_field_tail,
    done
};

enum class cached_state
{
    begin_object,
    end_object,
    begin_array,
    end_array,
    name,
    item,
    done
};

struct default_csv_parsing
{
    bool operator()(csv_errc, const ser_context&) noexcept
    {
        return false;
    }
};

namespace detail {

    template <class CharT,class WorkAllocator>
    class parse_event
    {
        typedef WorkAllocator work_allocator_type;
        typedef typename basic_json_content_handler<CharT>::string_view_type string_view_type;
        typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<CharT> char_allocator_type;
        typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<uint8_t> byte_allocator_type;
        typedef std::basic_string<CharT,std::char_traits<CharT>,char_allocator_type> string_type;
        typedef basic_byte_string<byte_allocator_type> byte_string_type;

        staj_event_type event_type;
        string_type string_value;
        byte_string_type byte_string_value;
        union
        {
            bool bool_value;
            int64_t int64_value;
            uint64_t uint64_value;
            double double_value;
        };
        semantic_tag tag;
    public:
        parse_event(staj_event_type event_type, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(event_type), 
              string_value(allocator),
              byte_string_value(allocator),
              tag(tag)
        {
        }

        parse_event(const string_view_type& value, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(staj_event_type::string_value), 
              string_value(value.data(),value.length(),allocator), 
              byte_string_value(allocator),
              tag(tag)
        {
        }

        parse_event(const byte_string_view& value, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(staj_event_type::byte_string_value), 
              string_value(allocator),
              byte_string_value(value.data(),value.size(),allocator), 
              tag(tag)
        {
        }

        parse_event(bool value, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(staj_event_type::bool_value), 
              string_value(allocator),
              byte_string_value(allocator),
              bool_value(value), 
              tag(tag)
        {
        }

        parse_event(int64_t value, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(staj_event_type::int64_value), 
              string_value(allocator),
              byte_string_value(allocator),
              int64_value(value), 
              tag(tag)
        {
        }

        parse_event(uint64_t value, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(staj_event_type::uint64_value), 
              string_value(allocator),
              byte_string_value(allocator),
              uint64_value(value), 
              tag(tag)
        {
        }

        parse_event(double value, semantic_tag tag, const WorkAllocator& allocator)
            : event_type(staj_event_type::double_value), 
              string_value(allocator),
              byte_string_value(allocator),
              double_value(value), 
              tag(tag)
        {
        }

        parse_event(const parse_event&) = default;
        parse_event(parse_event&&) = default;
        parse_event& operator=(const parse_event&) = default;
        parse_event& operator=(parse_event&&) = default;

        bool replay(basic_json_content_handler<CharT>& handler) const
        {
            switch (event_type)
            {
                case staj_event_type::begin_array:
                    return handler.begin_array(tag, null_ser_context());
                case staj_event_type::end_array:
                    return handler.end_array(null_ser_context());
                case staj_event_type::string_value:
                    return handler.string_value(string_value, tag, null_ser_context());
                case staj_event_type::byte_string_value:
                case staj_event_type::null_value:
                    return handler.null_value(tag, null_ser_context());
                case staj_event_type::bool_value:
                    return handler.bool_value(bool_value, tag, null_ser_context());
                case staj_event_type::int64_value:
                    return handler.int64_value(int64_value, tag, null_ser_context());
                case staj_event_type::uint64_value:
                    return handler.uint64_value(uint64_value, tag, null_ser_context());
                case staj_event_type::double_value:
                    return handler.double_value(double_value, tag, null_ser_context());
                default:
                    return false;
            }
        }
    };

    template <class CharT, class WorkAllocator>
    class m_columns_filter : public basic_json_content_handler<CharT>
    {
    public:
        typedef typename basic_json_content_handler<CharT>::string_view_type string_view_type;
        typedef CharT char_type;
        typedef WorkAllocator work_allocator_type;

        typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<CharT> char_allocator_type;
        typedef std::basic_string<CharT,std::char_traits<CharT>,char_allocator_type> string_type;

        typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<string_type> string_allocator_type;
        typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<parse_event<CharT,WorkAllocator>> parse_event_allocator_type;
        typedef std::vector<parse_event<CharT,WorkAllocator>, parse_event_allocator_type> parse_event_vector_type;
        typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<parse_event_vector_type> parse_event_vector_allocator_type;
    private:
        WorkAllocator allocator_;
        size_t name_index_;
        int level_;
        cached_state state_;
        size_t column_index_;
        size_t row_index_;

        std::vector<string_type, string_allocator_type> column_names_;
        std::vector<parse_event_vector_type,parse_event_vector_allocator_type> cached_events_;
    public:

        m_columns_filter(const WorkAllocator& allocator)
            : allocator_(allocator),
              name_index_(0), 
              level_(0), 
              state_(cached_state::begin_object), 
              column_index_(0), 
              row_index_(0),
              column_names_(allocator),
              cached_events_(allocator)
        {
        }

        bool done() const
        {
            return state_ == cached_state::done;
        }

        void initialize(const std::vector<string_type, string_allocator_type>& column_names)
        {
            for (const auto& name : column_names)
            {
                column_names_.push_back(name);
                cached_events_.emplace_back(allocator_);
            }
            name_index_ = 0;
            level_ = 0;
            column_index_ = 0;
            row_index_ = 0;
            state_ = cached_state::begin_object;
        }

        void skip_column()
        {
            ++name_index_;
        }

        bool replay_parse_events(basic_json_content_handler<CharT>& handler)
        {
            bool more = true;
            while (more)
            {
                switch (state_)
                {
                    case cached_state::begin_object:
                        more = handler.begin_object(semantic_tag::none, null_ser_context());
                        column_index_ = 0;
                        state_ = cached_state::name;
                        break;
                    case cached_state::end_object:
                        more = handler.end_object(null_ser_context());
                        state_ = cached_state::done;
                        break;
                    case cached_state::name:
                        if (column_index_ < column_names_.size())
                        {
                            more = handler.name(column_names_[column_index_], null_ser_context());
                            state_ = cached_state::begin_array;
                        }
                        else
                        {
                            state_ = cached_state::end_object;
                        }
                        break;
                    case cached_state::begin_array:
                        more = handler.begin_array(semantic_tag::none, null_ser_context());
                        row_index_ = 0;
                        state_ = cached_state::item;
                        break;
                    case cached_state::end_array:
                        more = handler.end_array(null_ser_context());
                        ++column_index_;
                        state_ = cached_state::name;
                        break;
                    case cached_state::item:
                        if (row_index_ < cached_events_[column_index_].size())
                        {
                            more = cached_events_[column_index_][row_index_].replay(handler);
                            ++row_index_;
                        }
                        else
                        {
                            state_ = cached_state::end_array;
                        }
                        break;
                    default:
                        more = false;
                        break;
                }
            }
            return more;
        }

        void do_flush() override
        {
        }

        bool do_begin_object(semantic_tag, const ser_context&, std::error_code& ec) override
        {
            ec = csv_errc::invalid_parse_state;
            return false;
        }

        bool do_end_object(const ser_context&, std::error_code& ec) override
        {
            ec = csv_errc::invalid_parse_state;
            return false;
        }

        bool do_begin_array(semantic_tag tag, const ser_context&, std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(staj_event_type::begin_array, tag, allocator_);
                
                ++level_;
            }
            return true;
        }

        bool do_end_array(const ser_context&, std::error_code&) override
        {
            if (level_ > 0)
            {
                cached_events_[name_index_].emplace_back(staj_event_type::end_array, semantic_tag::none, allocator_);
                ++name_index_;
                --level_;
            }
            else
            {
                name_index_ = 0;
            }
            return true;
        }

        bool do_name(const string_view_type&, const ser_context&, std::error_code& ec) override
        {
            ec = csv_errc::invalid_parse_state;
            return false;
        }

        bool do_null_value(semantic_tag tag, const ser_context&, std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(staj_event_type::null_value, tag, allocator_);
                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }

        bool do_string_value(const string_view_type& value, semantic_tag tag, const ser_context&, std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(value, tag, allocator_);

                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }

        bool do_byte_string_value(const byte_string_view& value,
                                  semantic_tag tag,
                                  const ser_context&,
                                  std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(value, tag, allocator_);
                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }

        bool do_double_value(double value,
                             semantic_tag tag, 
                             const ser_context&,
                             std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(value, tag, allocator_);
                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }

        bool do_int64_value(int64_t value,
                            semantic_tag tag,
                            const ser_context&,
                            std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(value, tag, allocator_);
                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }

        bool do_uint64_value(uint64_t value,
                             semantic_tag tag,
                             const ser_context&,
                             std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(value, tag, allocator_);
                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }

        bool do_bool_value(bool value, semantic_tag tag, const ser_context&, std::error_code&) override
        {
            if (name_index_ < column_names_.size())
            {
                cached_events_[name_index_].emplace_back(value, tag, allocator_);
                if (level_ == 0)
                {
                    ++name_index_;
                }
            }
            return true;
        }
    };

} // namespace detail

template<class CharT,class WorkAllocator=std::allocator<char>>
class basic_csv_parser : public ser_context
{
public:
    typedef basic_string_view<CharT> string_view_type;
    typedef CharT char_type;
private:
    struct string_maps_to_double
    {
        string_view_type s;

        bool operator()(const std::pair<string_view_type,double>& val) const
        {
            return val.first == s;
        }
    };

    typedef WorkAllocator work_allocator_type;
    typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<CharT> char_allocator_type;
    typedef std::basic_string<CharT,std::char_traits<CharT>,char_allocator_type> string_type;
    typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<string_type> string_allocator_type;
    typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<csv_mode> csv_mode_allocator_type;
    typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<csv_type_info> csv_type_info_allocator_type;
    typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<std::vector<string_type,string_allocator_type>> string_vector_allocator_type;
    typedef typename std::allocator_traits<work_allocator_type>:: template rebind_alloc<csv_parse_state> csv_parse_state_allocator_type;

    static const int default_depth = 3;

    work_allocator_type allocator_;
    csv_parse_state state_;
    basic_json_content_handler<CharT>* handler_;
    std::function<bool(csv_errc,const ser_context&)> err_handler_;
    size_t column_;
    size_t line_;
    int depth_;
    const basic_csv_decode_options<CharT> options_;
    size_t column_index_;
    size_t level_;
    size_t offset_;
    jsoncons::detail::string_to_double to_double_; 
    const CharT* begin_input_;
    const CharT* input_end_;
    const CharT* input_ptr_;
    bool more_;
    size_t header_line_;

    detail::m_columns_filter<CharT,WorkAllocator> m_columns_filter_;
    std::vector<csv_mode,csv_mode_allocator_type> stack_;
    std::vector<string_type,string_allocator_type> column_names_;
    std::vector<csv_type_info,csv_type_info_allocator_type> column_types_;
    std::vector<string_type,string_allocator_type> column_defaults_;
    std::vector<csv_parse_state,csv_parse_state_allocator_type> state_stack_;
    string_type buffer_;
    std::vector<std::pair<string_view_type,double>> string_double_map_;

public:
    basic_csv_parser(const WorkAllocator& allocator = WorkAllocator())
       : basic_csv_parser(basic_csv_decode_options<CharT>(), 
                          default_csv_parsing(),
                          allocator)
    {
    }

    basic_csv_parser(const basic_csv_decode_options<CharT>& options,
                     const WorkAllocator& allocator = WorkAllocator())
        : basic_csv_parser(options, 
                           default_csv_parsing(),
                           allocator)
    {
    }

    basic_csv_parser(std::function<bool(csv_errc,const ser_context&)> err_handler,
                     const WorkAllocator& allocator = WorkAllocator())
        : basic_csv_parser(basic_csv_decode_options<CharT>(), 
                           err_handler,
                           allocator)
    {
    }

    basic_csv_parser(const basic_csv_decode_options<CharT>& options,
                     std::function<bool(csv_errc,const ser_context&)> err_handler,
                     const WorkAllocator& allocator = WorkAllocator())
       : allocator_(allocator),
         handler_(nullptr),
         err_handler_(err_handler),
         options_(options),
         level_(0),
         offset_(0),
         begin_input_(nullptr),
         input_end_(nullptr),
         input_ptr_(nullptr),
         more_(true),
         header_line_(1),
         m_columns_filter_(allocator),
         stack_(allocator),
         column_names_(allocator),
         column_types_(allocator),
         column_defaults_(allocator),
         state_stack_(allocator),
         buffer_(allocator)
    {
        depth_ = default_depth;
        state_ = csv_parse_state::start;
        line_ = 1;
        column_ = 1;
        column_index_ = 0;
        stack_.reserve(default_depth);
        if (options_.enable_str_to_nan())
        {
            string_double_map_.emplace_back(options_.nan_to_str(),std::nan(""));
        }
        if (options_.enable_str_to_inf())
        {
            string_double_map_.emplace_back(options_.inf_to_str(),std::numeric_limits<double>::infinity());
        }
        if (options_.enable_str_to_neginf())
        {
            string_double_map_.emplace_back(options_.neginf_to_str(),-std::numeric_limits<double>::infinity());
        }
        stack_.push_back(csv_mode::initial);

        jsoncons::csv::detail::parse_column_names(options.column_names(), column_names_);
        jsoncons::csv::detail::parse_column_types(options.column_types(), column_types_);
        jsoncons::csv::detail::parse_column_names(options.column_defaults(), column_defaults_);

        if (options_.header_lines() > 0)
        {
            stack_.push_back(csv_mode::header);
        }
        else
        {
            stack_.push_back(csv_mode::data);
        }
        state_ = csv_parse_state::start;
        column_index_ = 0;
        column_ = 1;
        level_ = 0;
    }

    ~basic_csv_parser()
    {
    }

    bool done() const
    {
        return state_ == csv_parse_state::done;
    }

    bool stopped() const
    {
        return !more_;
    }

    bool finished() const
    {
        return !more_;
        //return !more_ && (state_ != csv_parse_state::no_more_records || state_ != csv_parse_state::before_done);
    }

    bool source_exhausted() const
    {
        return input_ptr_ == input_end_;
    }

    const std::vector<string_type,string_allocator_type>& column_labels() const
    {
        return column_names_;
    }

    void restart()
    {
        more_ = true;
    }

    void parse_some(basic_json_content_handler<CharT>& handler)
    {
        std::error_code ec;
        parse_some(handler,ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec,line_,column_));
        }
    }

    void parse_some(basic_json_content_handler<CharT>& handler, std::error_code& ec)
    {
        switch (options_.mapping())
        {
            case mapping_kind::m_columns:
                handler_ = &m_columns_filter_;
                break;
            default:
                handler_ = std::addressof(handler);
                break;
        } 

        const CharT* local_input_end = input_end_;

        if (input_ptr_ == local_input_end && more_)
        {
            switch (state_)
            {
                case csv_parse_state::before_unquoted_field:
                case csv_parse_state::before_last_unquoted_field:
                    end_unquoted_string_value(ec);
                    state_ = csv_parse_state::before_last_unquoted_field_tail;
                    break;
                case csv_parse_state::before_last_unquoted_field_tail:
                    if (stack_.back() == csv_mode::subfields)
                    {
                        stack_.pop_back();
                        more_ = handler_->end_array(*this, ec);
                    }
                    ++column_index_;
                    state_ = csv_parse_state::end_record;
                    break;
                case csv_parse_state::before_unquoted_string: 
                    buffer_.clear();
                    JSONCONS_FALLTHROUGH;
                case csv_parse_state::unquoted_string: 
                    if (options_.trim_leading() || options_.trim_trailing())
                    {
                        trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                    }
                    if (!(options_.ignore_empty_values() && buffer_.empty()))
                    {
                        before_value(ec);
                        state_ = csv_parse_state::before_unquoted_field;
                    }
                    else
                    {
                        if (options_.trim_leading() || options_.trim_trailing())
                        {
                            trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                        }
                        if (!(options_.ignore_empty_values() && buffer_.empty()))
                        {
                            before_value(ec);
                            state_ = csv_parse_state::before_last_quoted_field;
                        }
                        else
                        {
                            state_ = csv_parse_state::end_record;
                        }
                        state_ = csv_parse_state::end_record;
                    }
                    break;
                case csv_parse_state::before_last_quoted_field:
                    end_quoted_string_value(ec);
                    ++column_index_;
                    state_ = csv_parse_state::end_record;
                    break;
                case csv_parse_state::escaped_value:
                    if (options_.quote_escape_char() == options_.quote_char())
                    {
                        if (!(options_.ignore_empty_values() && buffer_.empty()))
                        {
                            before_value(ec);
                            ++column_;
                            state_ = csv_parse_state::before_last_quoted_field;
                        }
                        else
                        {
                            state_ = csv_parse_state::end_record;
                        }
                    }
                    else
                    {
                        ec = csv_errc::invalid_escaped_char;
                        more_ = false;
                        return;
                    }
                    break;
                case csv_parse_state::end_record:
                    if (column_index_ > 0)
                    {
                        after_record(ec);
                    }
                    state_ = csv_parse_state::no_more_records;
                    break;
                case csv_parse_state::no_more_records: 
                    switch (stack_.back()) 
                    {
                        case csv_mode::header:
                            stack_.pop_back();
                            break;
                        case csv_mode::data:
                            stack_.pop_back();
                            break;
                        default:
                            break;
                    }
                    more_ = handler_->end_array(*this, ec);
                    if (options_.mapping() == mapping_kind::m_columns)
                    {
                        if (!m_columns_filter_.done())
                        {
                            more_ = m_columns_filter_.replay_parse_events(handler);
                        }
                        else
                        {
                            state_ = csv_parse_state::before_done;
                        }
                    }
                    else
                    {
                        state_ = csv_parse_state::before_done;
                    }
                    break;
                case csv_parse_state::before_done:
                    if (!(stack_.size() == 1 && stack_.back() == csv_mode::initial))
                    {
                        err_handler_(csv_errc::unexpected_eof, *this);
                        ec = csv_errc::unexpected_eof;
                        more_ = false;
                        return;
                    }
                    stack_.pop_back();
                    handler_->flush();
                    state_ = csv_parse_state::done;
                    more_ = false;
                    return;
                default:
                    state_ = csv_parse_state::end_record;
                    break;
            }
        }

        for (; (input_ptr_ < local_input_end) && more_;)
        {
            CharT curr_char = *input_ptr_;

            switch (state_) 
            {
                case csv_parse_state::cr:
                    ++line_;
                    column_ = 1;
                    switch (*input_ptr_)
                    {
                        case '\n':
                            ++input_ptr_;
                            state_ = pop_state();
                            break;
                        default:
                            state_ = pop_state();
                            break;
                    }
                    break;
                case csv_parse_state::start:
                    if (options_.mapping() != mapping_kind::m_columns)
                    {
                        more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                    }
                    if (!options_.assume_header() && options_.mapping() == mapping_kind::n_rows && options_.column_names().size() > 0)
                    {
                        column_index_ = 0;
                        state_ = csv_parse_state::column_labels;
                        more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                    }
                    else
                    {
                        state_ = csv_parse_state::expect_comment_or_record;
                    }
                    break;
                case csv_parse_state::column_labels:
                    if (column_index_ < column_names_.size())
                    {
                        more_ = handler_->string_value(column_names_[column_index_], semantic_tag::none, *this, ec);
                        ++column_index_;
                    }
                    else
                    {
                        more_ = handler_->end_array(*this, ec);
                        state_ = csv_parse_state::expect_comment_or_record; 
                        //stack_.back() = csv_mode::data;
                        column_index_ = 0;
                    }
                    break;
                case csv_parse_state::comment: 
                    switch (curr_char)
                    {
                        case '\n':
                        {
                            ++line_;
                            if (stack_.back() == csv_mode::header)
                            {
                                ++header_line_;
                            }
                            column_ = 1;
                            state_ = csv_parse_state::expect_comment_or_record;
                            break;
                        }
                        case '\r':
                            ++line_;
                            if (stack_.back() == csv_mode::header)
                            {
                                ++header_line_;
                            }
                            column_ = 1;
                            state_ = csv_parse_state::expect_comment_or_record;
                            push_state(state_);
                            state_ = csv_parse_state::cr;
                            break;
                        default:
                            ++column_;
                            break;
                    }
                    ++input_ptr_;
                    break;
                
                case csv_parse_state::expect_comment_or_record:
                    buffer_.clear();
                    if (curr_char == options_.comment_starter())
                    {
                        state_ = csv_parse_state::comment;
                        ++column_;
                        ++input_ptr_;
                    }
                    else
                    {
                        state_ = csv_parse_state::expect_record;
                    }
                    break;
                case csv_parse_state::quoted_string: 
                    {
                        if (curr_char == options_.quote_escape_char())
                        {
                            state_ = csv_parse_state::escaped_value;
                        }
                        else if (curr_char == options_.quote_char())
                        {
                            state_ = csv_parse_state::between_values;
                        }
                        else
                        {
                            buffer_.push_back(static_cast<CharT>(curr_char));
                        }
                    }
                    ++column_;
                    ++input_ptr_;
                    break;
                case csv_parse_state::escaped_value: 
                    {
                        if (curr_char == options_.quote_char())
                        {
                            buffer_.push_back(static_cast<CharT>(curr_char));
                            state_ = csv_parse_state::quoted_string;
                            ++column_;
                            ++input_ptr_;
                        }
                        else if (options_.quote_escape_char() == options_.quote_char())
                        {
                            state_ = csv_parse_state::between_values;
                        }
                        else
                        {
                            ec = csv_errc::invalid_escaped_char;
                            more_ = false;
                            return;
                        }
                    }
                    break;
                case csv_parse_state::between_values:
                    switch (curr_char)
                    {
                        case '\r':
                        case '\n':
                        {
                            if (options_.trim_leading() || options_.trim_trailing())
                            {
                                trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                            }
                            if (!(options_.ignore_empty_values() && buffer_.empty()))
                            {
                                before_value(ec);
                                state_ = csv_parse_state::before_last_quoted_field;
                            }
                            else
                            {
                                state_ = csv_parse_state::end_record;
                            }
                            break;
                        }
                        default:
                            if (curr_char == options_.field_delimiter())
                            {
                                if (options_.trim_leading() || options_.trim_trailing())
                                {
                                    trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                                }
                                before_value(ec);
                                state_ = csv_parse_state::before_quoted_field;
                            }
                            else if (options_.subfield_delimiter() != char_type() && curr_char == options_.subfield_delimiter())
                            {
                                if (options_.trim_leading() || options_.trim_trailing())
                                {
                                    trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                                }
                                before_value(ec);
                                state_ = csv_parse_state::before_quoted_subfield;
                            }
                            break;
                    }
                    break;
                case csv_parse_state::before_unquoted_string: 
                {
                    buffer_.clear();
                    state_ = csv_parse_state::unquoted_string;
                    break;
                }
                case csv_parse_state::before_unquoted_field:
                    end_unquoted_string_value(ec);
                    state_ = csv_parse_state::before_unquoted_field_tail;
                    break;
                case csv_parse_state::before_unquoted_field_tail:
                {
                    if (stack_.back() == csv_mode::subfields)
                    {
                        stack_.pop_back();
                        more_ = handler_->end_array(*this, ec);
                    }
                    ++column_index_;
                    state_ = csv_parse_state::before_unquoted_string;
                    ++column_;
                    ++input_ptr_;
                    break;
                }
                case csv_parse_state::before_unquoted_field_tail1:
                {
                    if (stack_.back() == csv_mode::subfields)
                    {
                        stack_.pop_back();
                        more_ = handler_->end_array(*this, ec);
                    }
                    state_ = csv_parse_state::end_record;
                    ++column_;
                    ++input_ptr_;
                    break;
                }

                case csv_parse_state::before_last_unquoted_field:
                    end_unquoted_string_value(ec);
                    state_ = csv_parse_state::before_last_unquoted_field_tail;
                    break;

                case csv_parse_state::before_last_unquoted_field_tail:
                    if (stack_.back() == csv_mode::subfields)
                    {
                        stack_.pop_back();
                        more_ = handler_->end_array(*this, ec);
                    }
                    ++column_index_;
                    state_ = csv_parse_state::end_record;
                    break;

                case csv_parse_state::before_unquoted_subfield:
                    if (stack_.back() == csv_mode::data)
                    {
                        stack_.push_back(csv_mode::subfields);
                        more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                    }
                    state_ = csv_parse_state::before_unquoted_subfield_tail;
                    break; 
                case csv_parse_state::before_unquoted_subfield_tail:
                    end_unquoted_string_value(ec);
                    state_ = csv_parse_state::before_unquoted_string;
                    ++column_;
                    ++input_ptr_;
                    break;
                case csv_parse_state::before_quoted_field:
                    end_quoted_string_value(ec);
                    state_ = csv_parse_state::before_unquoted_field_tail; // return to unquoted
                    break;
                case csv_parse_state::before_quoted_subfield:
                    if (stack_.back() == csv_mode::data)
                    {
                        stack_.push_back(csv_mode::subfields);
                        more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                    }
                    state_ = csv_parse_state::before_quoted_subfield_tail;
                    break; 
                case csv_parse_state::before_quoted_subfield_tail:
                    end_quoted_string_value(ec);
                    state_ = csv_parse_state::before_unquoted_string;
                    ++column_;
                    ++input_ptr_;
                    break;
                case csv_parse_state::before_last_quoted_field:
                    end_quoted_string_value(ec);
                    state_ = csv_parse_state::before_last_quoted_field_tail;
                    break;
                case csv_parse_state::before_last_quoted_field_tail:
                    if (stack_.back() == csv_mode::subfields)
                    {
                        stack_.pop_back();
                        more_ = handler_->end_array(*this, ec);
                    }
                    ++column_index_;
                    state_ = csv_parse_state::end_record;
                    break;
                case csv_parse_state::unquoted_string: 
                {
                    switch (curr_char)
                    {
                        case '\n':
                        case '\r':
                        {
                            if (options_.trim_leading() || options_.trim_trailing())
                            {
                                trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                            }
                            if (!(options_.ignore_empty_values() && buffer_.empty()))
                            {
                                before_value(ec);
                                state_ = csv_parse_state::before_last_unquoted_field;
                            }
                            else
                            {
                                state_ = csv_parse_state::end_record;
                            }
                            break;
                        }
                        default:
                            if (curr_char == options_.field_delimiter())
                            {
                                if (options_.trim_leading() || options_.trim_trailing())
                                {
                                    trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                                }
                                before_value(ec);
                                state_ = csv_parse_state::before_unquoted_field;
                            }
                            else if (options_.subfield_delimiter() != char_type() && curr_char == options_.subfield_delimiter())
                            {
                                if (options_.trim_leading() || options_.trim_trailing())
                                {
                                    trim_string_buffer(options_.trim_leading(),options_.trim_trailing());
                                }
                                before_value(ec);
                                state_ = csv_parse_state::before_unquoted_subfield;
                            }
                            else if (curr_char == options_.quote_char())
                            {
                                buffer_.clear();
                                state_ = csv_parse_state::quoted_string;
                                ++column_;
                                ++input_ptr_;
                            }
                            else
                            {
                                buffer_.push_back(static_cast<CharT>(curr_char));
                                ++column_;
                                ++input_ptr_;
                            }
                            break;
                    }
                    break;
                }
                case csv_parse_state::expect_record: 
                {
                    switch (curr_char)
                    {
                        case '\n':
                        {
                            if (!options_.ignore_empty_lines())
                            {
                                before_record(ec);
                                state_ = csv_parse_state::end_record;
                            }
                            else
                            {
                                ++line_;
                                column_ = 1;
                                state_ = csv_parse_state::expect_comment_or_record;
                                ++input_ptr_;
                            }
                            break;
                        }
                        case '\r':
                            if (!options_.ignore_empty_lines())
                            {
                                before_record(ec);
                                state_ = csv_parse_state::end_record;
                            }
                            else
                            {
                                ++line_;
                                column_ = 1;
                                state_ = csv_parse_state::expect_comment_or_record;
                                ++input_ptr_;
                                push_state(state_);
                                state_ = csv_parse_state::cr;
                            }
                            break;
                        case ' ':
                        case '\t':
                            if (!options_.trim_leading())
                            {
                                buffer_.push_back(static_cast<CharT>(curr_char));
                                before_record(ec);
                                state_ = csv_parse_state::unquoted_string;
                            }
                            ++column_;
                            ++input_ptr_;
                            break;
                        default:
                            before_record(ec);
                            if (curr_char == options_.quote_char())
                            {
                                buffer_.clear();
                                state_ = csv_parse_state::quoted_string;
                                ++column_;
                                ++input_ptr_;
                            }
                            else
                            {
                                state_ = csv_parse_state::unquoted_string;
                            }
                            break;
                        }
                    break;
                    }
                case csv_parse_state::end_record: 
                {
                    switch (curr_char)
                    {
                        case '\n':
                        {
                            ++line_;
                            column_ = 1;
                            state_ = csv_parse_state::expect_comment_or_record;
                            after_record(ec);
                            ++input_ptr_;
                            break;
                        }
                        case '\r':
                            ++line_;
                            column_ = 1;
                            state_ = csv_parse_state::expect_comment_or_record;
                            after_record(ec);
                            push_state(state_);
                            state_ = csv_parse_state::cr;
                            ++input_ptr_;
                            break;
                        case ' ':
                        case '\t':
                            ++column_;
                            ++input_ptr_;
                            break;
                        default:
                            err_handler_(csv_errc::invalid_csv_text, *this);
                            ec = csv_errc::invalid_csv_text;
                            more_ = false;
                            return;
                        }
                    break;
                }
                default:
                    err_handler_(csv_errc::invalid_parse_state, *this);
                    ec = csv_errc::invalid_parse_state;
                    more_ = false;
                    return;
            }
            if (line_ > options_.max_lines())
            {
                state_ = csv_parse_state::done;
                more_ = false;
            }
        }
    }

    void finish_parse()
    {
        std::error_code ec;
        finish_parse(ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec,line_,column_));
        }
    }

    void finish_parse(std::error_code& ec)
    {
        while (more_)
        {
            parse_some(ec);
        }
    }

    csv_parse_state state() const
    {
        return state_;
    }

    void update(const string_view_type sv)
    {
        update(sv.data(),sv.length());
    }

    void update(const CharT* data, size_t length)
    {
        begin_input_ = data;
        input_end_ = data + length;
        input_ptr_ = begin_input_;
    }

    size_t line() const override
    {
        return line_;
    }

    size_t column() const override
    {
        return column_;
    }
private:
    // name
    void before_value(std::error_code& ec)
    {
        switch (stack_.back())
        {
            case csv_mode::header:
                if (options_.trim_leading_inside_quotes() | options_.trim_trailing_inside_quotes())
                {
                    trim_string_buffer(options_.trim_leading_inside_quotes(),options_.trim_trailing_inside_quotes());
                }
                if (options_.assume_header() && line_ == header_line_)
                {
                    column_names_.push_back(buffer_);
                    if (options_.mapping() == mapping_kind::n_rows)
                    {
                        more_ = handler_->string_value(buffer_, semantic_tag::none, *this, ec);
                    }
                }
                break;
            case csv_mode::data:
                if (options_.mapping() == mapping_kind::n_objects)
                {
                    if (!(options_.ignore_empty_values() && buffer_.empty()))
                    {
                        if (column_index_ < column_names_.size() + offset_)
                        {
                            more_ = handler_->name(column_names_[column_index_ - offset_], *this, ec);
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

    // begin_array or begin_record
    void before_record(std::error_code& ec)
    {
        offset_ = 0;

        switch (stack_.back())
        {
            case csv_mode::header:
                if (options_.assume_header() && line_ == header_line_)
                {
                    if (options_.mapping() == mapping_kind::n_rows)
                    {
                        more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                    }
                }
                break;
            case csv_mode::data:
                switch (options_.mapping())
                {
                    case mapping_kind::n_rows:
                        more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                        break;
                    case mapping_kind::n_objects:
                        more_ = handler_->begin_object(semantic_tag::none, *this, ec);
                        break;
                    case mapping_kind::m_columns:
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    // end_array, begin_array, string_value (headers)
    void after_record(std::error_code& ec)
    {
        if (column_types_.size() > 0)
        {
            if (level_ > 0)
            {
                more_ = handler_->end_array(*this, ec);
                level_ = 0;
            }
        }
        switch (stack_.back())
        {
            case csv_mode::header:
                if (line_ >= options_.header_lines())
                {
                    stack_.back() = csv_mode::data;
                }
                switch (options_.mapping())
                {
                    case mapping_kind::n_rows:
                        if (options_.assume_header())
                        {
                            more_ = handler_->end_array(*this, ec);
                        }
                        break;
                    case mapping_kind::m_columns:
                        m_columns_filter_.initialize(column_names_);
                        break;
                    default:
                        break;
                }
                break;
            case csv_mode::data:
            case csv_mode::subfields:
            {
                switch (options_.mapping())
                {
                    case mapping_kind::n_rows:
                        more_ = handler_->end_array(*this, ec);
                        break;
                    case mapping_kind::n_objects:
                        more_ = handler_->end_object(*this, ec);
                        break;
                    case mapping_kind::m_columns:
                        more_ = handler_->end_array(*this, ec);
                        break;
                }
                break;
            }
            default:
                break;
        }
        column_index_ = 0;
    }

    void trim_string_buffer(bool trim_leading, bool trim_trailing)
    {
        size_t start = 0;
        size_t length = buffer_.length();
        if (trim_leading)
        {
            bool done = false;
            while (!done && start < buffer_.length())
            {
                if ((buffer_[start] < 256) && std::isspace(buffer_[start]))
                {
                    ++start;
                }
                else
                {
                    done = true;
                }
            }
        }
        if (trim_trailing)
        {
            bool done = false;
            while (!done && length > 0)
            {
                if ((buffer_[length-1] < 256) && std::isspace(buffer_[length-1]))
                {
                    --length;
                }
                else
                {
                    done = true;
                }
            }
        }
        if (start != 0 || length != buffer_.size())
        {
            buffer_ = buffer_.substr(start,length-start);
        }
    }

    /*
        end_array, begin_array, xxx_value (end_value)
    */
    void end_unquoted_string_value(std::error_code& ec) 
    {
        switch (stack_.back())
        {
            case csv_mode::data:
            case csv_mode::subfields:
                switch (options_.mapping())
                {
                case mapping_kind::n_rows:
                    if (options_.unquoted_empty_value_is_null() && buffer_.length() == 0)
                    {
                        more_ = handler_->null_value(semantic_tag::none, *this, ec);
                    }
                    else
                    {
                        end_value(options_.infer_types(), ec);
                    }
                    break;
                case mapping_kind::n_objects:
                    if (!(options_.ignore_empty_values() && buffer_.empty()))
                    {
                        if (column_index_ < column_names_.size() + offset_)
                        {
                            if (options_.unquoted_empty_value_is_null() && buffer_.length() == 0)
                            {
                                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                            }
                            else
                            {
                                end_value(options_.infer_types(), ec);
                            }
                        }
                        else if (level_ > 0)
                        {
                            if (options_.unquoted_empty_value_is_null() && buffer_.length() == 0)
                            {
                                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                            }
                            else
                            {
                                end_value(options_.infer_types(), ec);
                            }
                        }
                    }
                    break;
                case mapping_kind::m_columns:
                    if (!(options_.ignore_empty_values() && buffer_.empty()))
                    {
                        end_value(options_.infer_types(), ec);
                    }
                    else
                    {
                        m_columns_filter_.skip_column();
                    }
                    break;
                }
                break;
            default:
                break;
        }
    }

    void end_quoted_string_value(std::error_code& ec) 
    {
        switch (stack_.back())
        {
            case csv_mode::data:
            case csv_mode::subfields:
                if (options_.trim_leading_inside_quotes() | options_.trim_trailing_inside_quotes())
                {
                    trim_string_buffer(options_.trim_leading_inside_quotes(),options_.trim_trailing_inside_quotes());
                }
                switch (options_.mapping())
                {
                case mapping_kind::n_rows:
                    end_value(false, ec);
                    break;
                case mapping_kind::n_objects:
                    if (!(options_.ignore_empty_values() && buffer_.empty()))
                    {
                        if (column_index_ < column_names_.size() + offset_)
                        {
                            if (options_.unquoted_empty_value_is_null() && buffer_.length() == 0)
                            {
                                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                            }
                            else 
                            {
                                end_value(false, ec);
                            }
                        }
                        else if (level_ > 0)
                        {
                            if (options_.unquoted_empty_value_is_null() && buffer_.length() == 0)
                            {
                                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                            }
                            else
                            {
                                end_value(false, ec);
                            }
                        }
                    }
                    break;
                case mapping_kind::m_columns:
                    if (!(options_.ignore_empty_values() && buffer_.empty()))
                    {
                        end_value(false, ec);
                    }
                    else
                    {
                        m_columns_filter_.skip_column();
                    }
                    break;
                }
                break;
            default:
                break;
        }
    }

    void end_value(bool infer_types, std::error_code&  ec)
    {
        auto it = std::find_if(string_double_map_.begin(), string_double_map_.end(), string_maps_to_double{ buffer_ });
        if (it != string_double_map_.end())
        {
            more_ = handler_->double_value(it->second, semantic_tag::none, *this, ec);
        }
        else if (column_index_ < column_types_.size() + offset_)
        {
            if (column_types_[column_index_ - offset_].col_type == csv_column_type::repeat_t)
            {
                offset_ = offset_ + column_types_[column_index_ - offset_].rep_count;
                if (column_index_ - offset_ + 1 < column_types_.size())
                {
                    if (column_index_ == offset_ || level_ > column_types_[column_index_-offset_].level)
                    {
                        more_ = handler_->end_array(*this, ec);
                    }
                    level_ = column_index_ == offset_ ? 0 : column_types_[column_index_ - offset_].level;
                }
            }
            if (level_ < column_types_[column_index_ - offset_].level)
            {
                more_ = handler_->begin_array(semantic_tag::none, *this, ec);
                level_ = column_types_[column_index_ - offset_].level;
            }
            else if (level_ > column_types_[column_index_ - offset_].level)
            {
                more_ = handler_->end_array(*this, ec);
                level_ = column_types_[column_index_ - offset_].level;
            }
            switch (column_types_[column_index_ - offset_].col_type)
            {
                case csv_column_type::integer_t:
                    {
                        std::basic_istringstream<CharT,std::char_traits<CharT>,char_allocator_type> iss{buffer_};
                        int64_t val;
                        iss >> val;
                        if (!iss.fail())
                        {
                            more_ = handler_->int64_value(val, semantic_tag::none, *this, ec);
                        }
                        else
                        {
                            if (column_index_ - offset_ < column_defaults_.size() && column_defaults_[column_index_ - offset_].length() > 0)
                            {
                                basic_json_parser<CharT,work_allocator_type> parser(allocator_);
                                parser.update(column_defaults_[column_index_ - offset_].data(),column_defaults_[column_index_ - offset_].length());
                                parser.parse_some(*handler_);
                                parser.finish_parse(*handler_);
                            }
                            else
                            {
                                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                            }
                        }
                    }
                    break;
                case csv_column_type::float_t:
                    {
                        if (options_.lossless_number())
                        {
                            more_ = handler_->string_value(buffer_,semantic_tag::bigdec, *this, ec);
                        }
                        else
                        {
                            std::basic_istringstream<CharT, std::char_traits<CharT>, char_allocator_type> iss{ buffer_ };
                            double val;
                            iss >> val;
                            if (!iss.fail())
                            {
                                more_ = handler_->double_value(val, semantic_tag::none, *this, ec);
                            }
                            else
                            {
                                if (column_index_ - offset_ < column_defaults_.size() && column_defaults_[column_index_ - offset_].length() > 0)
                                {
                                    basic_json_parser<CharT,work_allocator_type> parser(allocator_);
                                    parser.update(column_defaults_[column_index_ - offset_].data(),column_defaults_[column_index_ - offset_].length());
                                    parser.parse_some(*handler_);
                                    parser.finish_parse(*handler_);
                                }
                                else
                                {
                                    more_ = handler_->null_value(semantic_tag::none, *this, ec);
                                }
                            }
                        }
                    }
                    break;
                case csv_column_type::boolean_t:
                    {
                        if (buffer_.length() == 1 && buffer_[0] == '0')
                        {
                            more_ = handler_->bool_value(false, semantic_tag::none, *this, ec);
                        }
                        else if (buffer_.length() == 1 && buffer_[0] == '1')
                        {
                            more_ = handler_->bool_value(true, semantic_tag::none, *this, ec);
                        }
                        else if (buffer_.length() == 5 && ((buffer_[0] == 'f' || buffer_[0] == 'F') && (buffer_[1] == 'a' || buffer_[1] == 'A') && (buffer_[2] == 'l' || buffer_[2] == 'L') && (buffer_[3] == 's' || buffer_[3] == 'S') && (buffer_[4] == 'e' || buffer_[4] == 'E')))
                        {
                            more_ = handler_->bool_value(false, semantic_tag::none, *this, ec);
                        }
                        else if (buffer_.length() == 4 && ((buffer_[0] == 't' || buffer_[0] == 'T') && (buffer_[1] == 'r' || buffer_[1] == 'R') && (buffer_[2] == 'u' || buffer_[2] == 'U') && (buffer_[3] == 'e' || buffer_[3] == 'E')))
                        {
                            more_ = handler_->bool_value(true, semantic_tag::none, *this, ec);
                        }
                        else
                        {
                            if (column_index_ - offset_ < column_defaults_.size() && column_defaults_[column_index_ - offset_].length() > 0)
                            {
                                basic_json_parser<CharT,work_allocator_type> parser(allocator_);
                                parser.update(column_defaults_[column_index_ - offset_].data(),column_defaults_[column_index_ - offset_].length());
                                parser.parse_some(*handler_);
                                parser.finish_parse(*handler_);
                            }
                            else
                            {
                                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                            }
                        }
                    }
                    break;
                default:
                    if (buffer_.length() > 0)
                    {
                        more_ = handler_->string_value(buffer_, semantic_tag::none, *this, ec);
                    }
                    else
                    {
                        if (column_index_ < column_defaults_.size() + offset_ && column_defaults_[column_index_ - offset_].length() > 0)
                        {
                            basic_json_parser<CharT,work_allocator_type> parser(allocator_);
                            parser.update(column_defaults_[column_index_ - offset_].data(),column_defaults_[column_index_ - offset_].length());
                            parser.parse_some(*handler_);
                            parser.finish_parse(*handler_);
                        }
                        else
                        {
                            more_ = handler_->string_value(string_view_type(), semantic_tag::none, *this, ec);
                        }
                    }
                    break;  
            }
        }
        else
        {
            if (infer_types)
            {
                end_value_with_numeric_check(ec);
            }
            else
            {
                more_ = handler_->string_value(buffer_, semantic_tag::none, *this, ec);
            }
        }
    }

    enum class numeric_check_state 
    {
        initial,
        null,
        boolean_true,
        boolean_false,
        minus,
        zero,
        integer,
        fraction1,
        fraction,
        exp1,
        exp,
        done
    };

    /*
        xxx_value 
    */
    void end_value_with_numeric_check(std::error_code& ec)
    {
        numeric_check_state state = numeric_check_state::initial;
        bool is_negative = false;
        int precision = 0;
        uint8_t decimal_places = 0;

        auto last = buffer_.end();

        std::string buffer;
        for (auto p = buffer_.begin(); state != numeric_check_state::done && p != last; ++p)
        {
            switch (state)
            {
                case numeric_check_state::initial:
                {
                    switch (*p)
                    {
                    case 'n':case 'N':
                        if ((last-p) == 4 && (p[1] == 'u' || p[1] == 'U') && (p[2] == 'l' || p[2] == 'L') && (p[3] == 'l' || p[3] == 'L'))
                        {
                            state = numeric_check_state::null;
                        }
                        else
                        {
                            state = numeric_check_state::done;
                        }
                        break;
                    case 't':case 'T':
                        if ((last-p) == 4 && (p[1] == 'r' || p[1] == 'R') && (p[2] == 'u' || p[2] == 'U') && (p[3] == 'e' || p[3] == 'U'))
                        {
                            state = numeric_check_state::boolean_true;
                        }
                        else
                        {
                            state = numeric_check_state::done;
                        }
                        break;
                    case 'f':case 'F':
                        if ((last-p) == 5 && (p[1] == 'a' || p[1] == 'A') && (p[2] == 'l' || p[2] == 'L') && (p[3] == 's' || p[3] == 'S') && (p[4] == 'e' || p[4] == 'E'))
                        {
                            state = numeric_check_state::boolean_false;
                        }
                        else
                        {
                            state = numeric_check_state::done;
                        }
                        break;
                    case '-':
                        is_negative = true;
                        buffer.push_back(*p);
                        state = numeric_check_state::minus;
                        break;
                    case '0':
                        ++precision;
                        buffer.push_back(*p);
                        state = numeric_check_state::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        ++precision;
                        buffer.push_back(*p);
                        state = numeric_check_state::integer;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::zero:
                {
                    switch (*p)
                    {
                    case '.':
                        buffer.push_back(to_double_.get_decimal_point());
                        state = numeric_check_state::fraction1;
                        break;
                    case 'e':case 'E':
                        buffer.push_back(*p);
                        state = numeric_check_state::exp1;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::integer:
                {
                    switch (*p)
                    {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        ++precision;
                        buffer.push_back(*p);
                        break;
                    case '.':
                        buffer.push_back(to_double_.get_decimal_point());
                        state = numeric_check_state::fraction1;
                        break;
                    case 'e':case 'E':
                        buffer.push_back(*p);
                        state = numeric_check_state::exp1;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::minus:
                {
                    switch (*p)
                    {
                    case '0':
                        ++precision;
                        buffer.push_back(*p);
                        state = numeric_check_state::zero;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        ++precision;
                        buffer.push_back(*p);
                        state = numeric_check_state::integer;
                        break;
                    case 'e':case 'E':
                        buffer.push_back(*p);
                        state = numeric_check_state::exp1;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::fraction1:
                {
                    switch (*p)
                    {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        ++precision;
                        ++decimal_places;
                        buffer.push_back(*p);
                        state = numeric_check_state::fraction;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::fraction:
                {
                    switch (*p)
                    {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        ++precision;
                        ++decimal_places;
                        buffer.push_back(*p);
                        break;
                    case 'e':case 'E':
                        buffer.push_back(*p);
                        state = numeric_check_state::exp1;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::exp1:
                {
                    switch (*p)
                    {
                    case '-':
                        buffer.push_back(*p);
                        state = numeric_check_state::exp;
                        break;
                    case '+':
                        state = numeric_check_state::exp;
                        break;
                    case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        buffer.push_back(*p);
                        state = numeric_check_state::integer;
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                case numeric_check_state::exp:
                {
                    switch (*p)
                    {
                    case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                        buffer.push_back(*p);
                        break;
                    default:
                        state = numeric_check_state::done;
                        break;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        switch (state)
        {
            case numeric_check_state::null:
                more_ = handler_->null_value(semantic_tag::none, *this, ec);
                break;
            case numeric_check_state::boolean_true:
                more_ = handler_->bool_value(true, semantic_tag::none, *this, ec);
                break;
            case numeric_check_state::boolean_false:
                more_ = handler_->bool_value(false, semantic_tag::none, *this, ec);
                break;
            case numeric_check_state::zero:
            case numeric_check_state::integer:
            {
                if (is_negative)
                {
                    auto result = jsoncons::detail::to_integer<int64_t>(buffer_.data(), buffer_.length());
                    if (result.ec == jsoncons::detail::to_integer_errc())
                    {
                        more_ = handler_->int64_value(result.value, semantic_tag::none, *this, ec);
                    }
                    else // Must be overflow
                    {
                        more_ = handler_->string_value(buffer_, semantic_tag::bigint, *this, ec);
                    }
                }
                else
                {
                    auto result = jsoncons::detail::to_integer<uint64_t>(buffer_.data(), buffer_.length());
                    if (result.ec == jsoncons::detail::to_integer_errc())
                    {
                        more_ = handler_->uint64_value(result.value, semantic_tag::none, *this, ec);
                    }
                    else if (result.ec == jsoncons::detail::to_integer_errc::overflow)
                    {
                        more_ = handler_->string_value(buffer_, semantic_tag::bigint, *this, ec);
                    }
                    else
                    {
                        JSONCONS_THROW(json_runtime_error<std::invalid_argument>(make_error_code(result.ec).message()));
                    }
                }
                break;
            }
            case numeric_check_state::fraction:
            case numeric_check_state::exp:
            {
                if (options_.lossless_number())
                {
                    more_ = handler_->string_value(buffer_,semantic_tag::bigdec, *this, ec);
                }
                else
                {
                    double d = to_double_(buffer.c_str(), buffer.length());
                    more_ = handler_->double_value(d, semantic_tag::none, *this, ec);
                }
                break;
            }
            default:
            {
                more_ = handler_->string_value(buffer_, semantic_tag::none, *this, ec);
                break;
            }
        }
    } 

    void push_state(csv_parse_state state)
    {
        state_stack_.push_back(state);
    }

    csv_parse_state pop_state()
    {
        JSONCONS_ASSERT(!state_stack_.empty())
        csv_parse_state state = state_stack_.back();
        state_stack_.pop_back();
        return state;
    }
};

typedef basic_csv_parser<char> csv_parser;
typedef basic_csv_parser<wchar_t> wcsv_parser;

}}

#endif

