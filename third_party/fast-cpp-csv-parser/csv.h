// Copyright: (2012-2014) Ben Strasser <code@ben-strasser.net>
// License: BSD-3
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
//2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
//3. Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef CSV_H
#define CSV_H

#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <utility>
#include <cstdio>
#include <exception>
#ifndef CSV_IO_NO_THREAD
#include <future>
#endif
#include <cassert>
#include <cerrno>

namespace io{
        ////////////////////////////////////////////////////////////////////////////
        //                                 LineReader                             //
        ////////////////////////////////////////////////////////////////////////////

        namespace error{
                struct base : std::exception{
                        virtual void format_error_message()const = 0;                          
                       
                        const char*what()const throw(){
                                format_error_message();
                                return error_message_buffer;
                        }

                        mutable char error_message_buffer[256];
                };

                const int max_file_name_length = 255;

                struct with_file_name{
                        with_file_name(){
                                std::memset(file_name, 0, max_file_name_length+1);
                        }
                       
                        void set_file_name(const char*file_name){
                                std::strncpy(this->file_name, file_name, max_file_name_length);
                                this->file_name[max_file_name_length] = '\0';
                        }

                        char file_name[max_file_name_length+1];
                };

                struct with_file_line{
                        with_file_line(){
                                file_line = -1;
                        }
                       
                        void set_file_line(int file_line){
                                this->file_line = file_line;
                        }

                        int file_line;
                };

                struct with_errno{
                        with_errno(){
                                errno_value = 0;
                        }
                       
                        void set_errno(int errno_value){
                                this->errno_value = errno_value;
                        }

                        int errno_value;
                };

                struct can_not_open_file :
                        base,
                        with_file_name,
                        with_errno{
                        void format_error_message()const{
                                if(errno_value != 0)
                                        std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                                "Can not open file \"%s\" because \"%s\"."
                                                , file_name, std::strerror(errno_value));
                                else
                                        std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                                "Can not open file \"%s\"."
                                                , file_name);
                        }
                };

                struct line_length_limit_exceeded :
                        base,
                        with_file_name,
                        with_file_line{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Line number %d in file \"%s\" exceeds the maximum length of 2^24-1."
                                        , file_line, file_name);
                        }
                };
        }

        class LineReader{
        private:
                static const int block_len = 1<<24;
                #ifndef CSV_IO_NO_THREAD
                std::future<int>bytes_read;
                #endif
                FILE*file;
                char*buffer;
                int data_begin;
                int data_end;

                char file_name[error::max_file_name_length+1];
                unsigned file_line;

                void open_file(const char*file_name){
                        // We open the file in binary mode as it makes no difference under *nix
                        // and under Windows we handle \r\n newlines ourself.
                        file = std::fopen(file_name, "rb");
                        if(file == 0){
                                int x = errno; // store errno as soon as possible, doing it after constructor call can fail.
                                error::can_not_open_file err;
                                err.set_errno(x);
                                err.set_file_name(file_name);
                                throw err;
                        }
                }

                void init(){
                        file_line = 0;

                        // Tell the std library that we want to do the buffering ourself.
                        std::setvbuf(file, 0, _IONBF, 0);

                        try{
                                buffer = new char[3*block_len];
                        }catch(...){
                                std::fclose(file);
                                throw;
                        }

                        data_begin = 0;
                        data_end = std::fread(buffer, 1, 2*block_len, file);

                        // Ignore UTF-8 BOM
                        if(data_end >= 3 && buffer[0] == '\xEF' && buffer[1] == '\xBB' && buffer[2] == '\xBF')
                                data_begin = 3;

                        #ifndef CSV_IO_NO_THREAD
                        if(data_end == 2*block_len){
                                bytes_read = std::async(std::launch::async, [=]()->int{
                                        return std::fread(buffer + 2*block_len, 1, block_len, file);
                                });
                        }
                        #endif
                }

        public:
                LineReader() = delete;
                LineReader(const LineReader&) = delete;
                LineReader&operator=(const LineReader&) = delete;

                LineReader(const char*file_name, FILE*file):
                        file(file){
                        set_file_name(file_name);
                        init();
                }

                LineReader(const std::string&file_name, FILE*file):
                        file(file){
                        set_file_name(file_name.c_str());
                        init();
                }

                explicit LineReader(const char*file_name){
                        set_file_name(file_name);
                        open_file(file_name);
                        init();
                }

                explicit LineReader(const std::string&file_name){
                        set_file_name(file_name.c_str());
                        open_file(file_name.c_str());
                        init();
                }

                void set_file_name(const std::string&file_name){
                        set_file_name(file_name.c_str());
                }

                void set_file_name(const char*file_name){
                        strncpy(this->file_name, file_name, error::max_file_name_length);
                        this->file_name[error::max_file_name_length] = '\0';
                }

                const char*get_truncated_file_name()const{
                        return file_name;
                }

                void set_file_line(unsigned file_line){
                        this->file_line = file_line;
                }

                unsigned get_file_line()const{
                        return file_line;
                }

                char*next_line(){
                        if(data_begin == data_end)
                                return 0;

                        ++file_line;

                        assert(data_begin < data_end);
                        assert(data_end <= block_len*2);

                        if(data_begin >= block_len){
                                std::memcpy(buffer, buffer+block_len, block_len);
                                data_begin -= block_len;
                                data_end -= block_len;
                                #ifndef CSV_IO_NO_THREAD
                                if(bytes_read.valid())
                                #endif
                                {
                                        #ifndef CSV_IO_NO_THREAD
                                        data_end += bytes_read.get();
                                        #else
                                        data_end += std::fread(buffer + 2*block_len, 1, block_len, file);
                                        #endif
                                        std::memcpy(buffer+block_len, buffer+2*block_len, block_len);

                                        #ifndef CSV_IO_NO_THREAD
                                        bytes_read = std::async(std::launch::async, [=]()->int{
                                                return std::fread(buffer + 2*block_len, 1, block_len, file);
                                        });
                                        #endif
                                }
                        }

                        int line_end = data_begin;
                        while(buffer[line_end] != '\n' && line_end != data_end){
                                ++line_end;
                        }

                        if(line_end - data_begin + 1 > block_len){
                                error::line_length_limit_exceeded err;
                                err.set_file_name(file_name);
                                err.set_file_line(file_line);
                                throw err;
                        }

                        if(buffer[line_end] == '\n'){
                                buffer[line_end] = '\0';
                        }else{
                                // some files are missing the newline at the end of the
                                // last line
                                ++data_end;
                                buffer[line_end] = '\0';
                        }

                        // handle windows \r\n-line breaks
                        if(line_end != data_begin && buffer[line_end-1] == '\r')
                                buffer[line_end-1] = '\0';

                        char*ret = buffer + data_begin;
                        data_begin = line_end+1;
                        return ret;
                }

                ~LineReader(){
                        #ifndef CSV_IO_NO_THREAD
                        // GCC needs this or it will crash.
                        if(bytes_read.valid())
                                bytes_read.get();
                        #endif

                        delete[] buffer;
                        std::fclose(file);
                }
        };

        ////////////////////////////////////////////////////////////////////////////
        //                                 CSV                                    //
        ////////////////////////////////////////////////////////////////////////////

        namespace error{
                const int max_column_name_length = 63;
                struct with_column_name{
                        with_column_name(){
                                std::memset(column_name, 0, max_column_name_length+1);
                        }
                       
                        void set_column_name(const char*column_name){
                                std::strncpy(this->column_name, column_name, max_column_name_length);
                                this->column_name[max_column_name_length] = '\0';
                        }

                        char column_name[max_column_name_length+1];
                };


                const int max_column_content_length = 63;

                struct with_column_content{
                        with_column_content(){
                                std::memset(column_content, 0, max_column_content_length+1);
                        }
                       
                        void set_column_content(const char*column_content){
                                std::strncpy(this->column_content, column_content, max_column_content_length);
                                this->column_content[max_column_content_length] = '\0';
                        }

                        char column_content[max_column_content_length+1];
                };


                struct extra_column_in_header :
                        base,
                        with_file_name,
                        with_column_name{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Extra column \"%s\" in header of file \"%s\"."
                                        , column_name, file_name);
                        }
                };

                struct missing_column_in_header :
                        base,
                        with_file_name,
                        with_column_name{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Missing column \"%s\" in header of file \"%s\"."
                                        , column_name, file_name);
                        }
                };

                struct duplicated_column_in_header :
                        base,
                        with_file_name,
                        with_column_name{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Duplicated column \"%s\" in header of file \"%s\"."
                                        , column_name, file_name);
                        }
                };

                struct header_missing :
                        base,
                        with_file_name{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Header missing in file \"%s\"."
                                        , file_name);
                        }
                };

                struct too_few_columns :
                        base,
                        with_file_name,
                        with_file_line{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Too few columns in line %d in file \"%s\"."
                                        , file_line, file_name);
                        }
                };

                struct too_many_columns :
                        base,
                        with_file_name,
                        with_file_line{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Too many columns in line %d in file \"%s\"."
                                        , file_line, file_name);
                        }
                };

                struct escaped_string_not_closed :
                        base,
                        with_file_name,
                        with_file_line{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "Escaped string was not closed in line %d in file \"%s\"."
                                        , file_line, file_name);
                        }
                };

                struct integer_must_be_positive :
                        base,
                        with_file_name,
                        with_file_line,
                        with_column_name,
                        with_column_content{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "The integer \"%s\" must be positive or 0 in column \"%s\" in file \"%s\" in line \"%d\"."
                                        , column_content, column_name, file_name, file_line);
                        }
                };

                struct no_digit :
                        base,
                        with_file_name,
                        with_file_line,
                        with_column_name,
                        with_column_content{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "The integer \"%s\" contains an invalid digit in column \"%s\" in file \"%s\" in line \"%d\"."
                                        , column_content, column_name, file_name, file_line);
                        }
                };

                struct integer_overflow :
                        base,
                        with_file_name,
                        with_file_line,
                        with_column_name,
                        with_column_content{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "The integer \"%s\" overflows in column \"%s\" in file \"%s\" in line \"%d\"."
                                        , column_content, column_name, file_name, file_line);
                        }
                };

                struct integer_underflow :
                        base,
                        with_file_name,
                        with_file_line,
                        with_column_name,
                        with_column_content{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "The integer \"%s\" underflows in column \"%s\" in file \"%s\" in line \"%d\"."
                                        , column_content, column_name, file_name, file_line);
                        }
                };

                struct invalid_single_character :
                        base,
                        with_file_name,
                        with_file_line,
                        with_column_name,
                        with_column_content{
                        void format_error_message()const{
                                std::snprintf(error_message_buffer, sizeof(error_message_buffer),
                                        "The content \"%s\" of column \"%s\" in file \"%s\" in line \"%d\" is not a single character."
                                        , column_content, column_name, file_name, file_line);
                        }
                };
        }

        typedef unsigned ignore_column;
        static const ignore_column ignore_no_column = 0;
        static const ignore_column ignore_extra_column = 1;
        static const ignore_column ignore_missing_column = 2;

        template<char ... trim_char_list>
        struct trim_chars{
        private:
                constexpr static bool is_trim_char(char c){
                        return false;
                }
       
                template<class ...OtherTrimChars>
                constexpr static bool is_trim_char(char c, char trim_char, OtherTrimChars...other_trim_chars){
                        return c == trim_char || is_trim_char(c, other_trim_chars...);
                }

        public:
                static void trim(char*&str_begin, char*&str_end){
                        while(is_trim_char(*str_begin, trim_char_list...) && str_begin != str_end)
                                ++str_begin;
                        while(is_trim_char(*(str_end-1), trim_char_list...) && str_begin != str_end)
                                --str_end;
                        *str_end = '\0';
                }
        };


        struct no_comment{
                static bool is_comment(const char*line){
                        return false;
                }
        };

        template<char ... comment_start_char_list>
        struct single_line_comment{
        private:
                constexpr static bool is_comment_start_char(char c){
                        return false;
                }
       
                template<class ...OtherCommentStartChars>
                constexpr static bool is_comment_start_char(char c, char comment_start_char, OtherCommentStartChars...other_comment_start_chars){
                        return c == comment_start_char || is_comment_start_char(c, other_comment_start_chars...);
                }

        public:

                static bool is_comment(const char*line){
                        return is_comment_start_char(*line, comment_start_char_list...);
                }
        };

        struct empty_line_comment{
                static bool is_comment(const char*line){
                        if(*line == '\0')
                                return true;
                        while(*line == ' ' || *line == '\t'){
                                ++line;
                                if(*line == 0)
                                        return true;
                        }
                        return false;
                }
        };

        template<char ... comment_start_char_list>
        struct single_and_empty_line_comment{
                static bool is_comment(const char*line){
                        return single_line_comment<comment_start_char_list...>::is_comment(line) || empty_line_comment::is_comment(line);
                }
        };

        template<char sep>
        struct no_quote_escape{
                static const char*find_next_column_end(const char*col_begin){
                        while(*col_begin != sep && *col_begin != '\0')
                                ++col_begin;
                        return col_begin;
                }

                static void unescape(char*&col_begin, char*&col_end){

                }
        };

        template<char sep, char quote>
        struct double_quote_escape{
                static const char*find_next_column_end(const char*col_begin){
                        while(*col_begin != sep && *col_begin != '\0')
                                if(*col_begin != quote)
                                        ++col_begin;
                                else{
                                        do{
                                                ++col_begin;
                                                while(*col_begin != quote){
                                                        if(*col_begin == '\0')
                                                                throw error::escaped_string_not_closed();
                                                        ++col_begin;
                                                }
                                                ++col_begin;
                                        }while(*col_begin == quote);
                                }      
                        return col_begin;      
                }

                static void unescape(char*&col_begin, char*&col_end){
                        if(col_end - col_begin >= 2){
                                if(*col_begin == quote && *(col_end-1) == quote){
                                        ++col_begin;
                                        --col_end;
                                        char*out = col_begin;
                                        for(char*in = col_begin; in!=col_end; ++in){
                                                if(*in == quote && *(in+1) == quote){
                                                         ++in;
                                                }
                                                *out = *in;
                                                ++out;
                                        }
                                        col_end = out;
                                        *col_end = '\0';
                                }
                        }
                       
                }
        };

        struct throw_on_overflow{
                template<class T>
                static void on_overflow(T&){
                        throw error::integer_overflow();
                }
               
                template<class T>
                static void on_underflow(T&){
                        throw error::integer_underflow();
                }
        };

        struct ignore_overflow{
                template<class T>
                static void on_overflow(T&){}
               
                template<class T>
                static void on_underflow(T&){}
        };

        struct set_to_max_on_overflow{
                template<class T>
                static void on_overflow(T&x){
                        x = std::numeric_limits<T>::max();
                }
               
                template<class T>
                static void on_underflow(T&x){
                        x = std::numeric_limits<T>::min();
                }
        };


        namespace detail{
                template<class quote_policy>
                void chop_next_column(
                        char*&line, char*&col_begin, char*&col_end
                ){
                        assert(line != nullptr);

                        col_begin = line;
                        // the col_begin + (... - col_begin) removes the constness
                        col_end = col_begin + (quote_policy::find_next_column_end(col_begin) - col_begin);
                       
                        if(*col_end == '\0'){
                                line = nullptr;
                        }else{
                                *col_end = '\0';
                                line = col_end + 1;    
                        }
                }

                template<class trim_policy, class quote_policy>
                void parse_line(
                        char*line,
                        char**sorted_col,
                        const std::vector<int>&col_order
                ){
                        for(std::size_t i=0; i<col_order.size(); ++i){
                                if(line == nullptr)
                                        throw ::io::error::too_few_columns();
                                char*col_begin, *col_end;
                                chop_next_column<quote_policy>(line, col_begin, col_end);

                                if(col_order[i] != -1){
                                        trim_policy::trim(col_begin, col_end);
                                        quote_policy::unescape(col_begin, col_end);
                                                               
                                        sorted_col[col_order[i]] = col_begin;
                                }
                        }
                        if(line != nullptr)
                                throw ::io::error::too_many_columns();
                }

                template<unsigned column_count, class trim_policy, class quote_policy>
                void parse_header_line(
                        char*line,
                        std::vector<int>&col_order,
                        const std::string*col_name,
                        ignore_column ignore_policy
                ){
                        col_order.clear();

                        bool found[column_count];
                        std::fill(found, found + column_count, false);
                        while(line){
                                char*col_begin,*col_end;
                                chop_next_column<quote_policy>(line, col_begin, col_end);

                                trim_policy::trim(col_begin, col_end);
                                quote_policy::unescape(col_begin, col_end);
                               
                                for(unsigned i=0; i<column_count; ++i)
                                        if(col_begin == col_name[i]){
                                                if(found[i]){
                                                        error::duplicated_column_in_header err;
                                                        err.set_column_name(col_begin);
                                                        throw err;
                                                }
                                                found[i] = true;
                                                col_order.push_back(i);
                                                col_begin = 0;
                                                break;
                                        }
                                if(col_begin){
                                        if(ignore_policy & ::io::ignore_extra_column)
                                                col_order.push_back(-1);
                                        else{
                                                error::extra_column_in_header err;
                                                err.set_column_name(col_begin);
                                                throw err;
                                        }
                                }
                        }
                        if(!(ignore_policy & ::io::ignore_missing_column)){
                                for(unsigned i=0; i<column_count; ++i){
                                        if(!found[i]){
                                                error::missing_column_in_header err;
                                                err.set_column_name(col_name[i].c_str());
                                                throw err;
                                        }
                                }
                        }
                }

                template<class overflow_policy>
                void parse(char*col, char &x){
                        if(!*col)
                                throw error::invalid_single_character();
                        x = *col;
                        ++col;
                        if(*col)
                                throw error::invalid_single_character();
                }
               
                template<class overflow_policy>
                void parse(char*col, std::string&x){
                        x = col;
                }

                template<class overflow_policy>
                void parse(char*col, const char*&x){
                        x = col;
                }

                template<class overflow_policy>
                void parse(char*col, char*&x){
                        x = col;
                }

                template<class overflow_policy, class T>
                void parse_unsigned_integer(const char*col, T&x){
                        x = 0;
                        while(*col != '\0'){
                                if('0' <= *col && *col <= '9'){
                                        T y = *col - '0';
                                        if(x > (std::numeric_limits<T>::max()-y)/10){
                                                overflow_policy::on_overflow(x);
                                                return;
                                        }
                                        x = 10*x+y;
                                }else
                                        throw error::no_digit();
                                ++col;
                        }
                }

                template<class overflow_policy>void parse(char*col, unsigned char &x)
                        {parse_unsigned_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, unsigned short &x)
                        {parse_unsigned_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, unsigned int &x)
                        {parse_unsigned_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, unsigned long &x)
                        {parse_unsigned_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, unsigned long long &x)
                        {parse_unsigned_integer<overflow_policy>(col, x);}
               
                template<class overflow_policy, class T>
                void parse_signed_integer(const char*col, T&x){
                        if(*col == '-'){
                                ++col;

                                x = 0;
                                while(*col != '\0'){
                                        if('0' <= *col && *col <= '9'){
                                                T y = *col - '0';
                                                if(x < (std::numeric_limits<T>::min()+y)/10){
                                                        overflow_policy::on_underflow(x);
                                                        return;
                                                }
                                                x = 10*x-y;
                                        }else
                                                throw error::no_digit();
                                        ++col;
                                }
                                return;
                        }else if(*col == '+')
                                ++col;
                        parse_unsigned_integer<overflow_policy>(col, x);
                }      

                template<class overflow_policy>void parse(char*col, signed char &x)
                        {parse_signed_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, signed short &x)
                        {parse_signed_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, signed int &x)
                        {parse_signed_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, signed long &x)
                        {parse_signed_integer<overflow_policy>(col, x);}
                template<class overflow_policy>void parse(char*col, signed long long &x)
                        {parse_signed_integer<overflow_policy>(col, x);}

                template<class T>
                void parse_float(const char*col, T&x){
                        bool is_neg = false;
                        if(*col == '-'){
                                is_neg = true;
                                ++col;
                        }else if(*col == '+')
                                ++col;

                        x = 0;
                        while('0' <= *col && *col <= '9'){
                                int y = *col - '0';
                                x *= 10;
                                x += y;
                                ++col;
                        }
                       
                        if(*col == '.'|| *col == ','){
                                ++col;
                                T pos = 1;
                                while('0' <= *col && *col <= '9'){
                                        pos /= 10;
                                        int y = *col - '0';
                                        ++col;
                                        x += y*pos;
                                }
                        }

                        if(*col == 'e' || *col == 'E'){
                                ++col;
                                int e;

                                parse_signed_integer<set_to_max_on_overflow>(col, e);
                               
                                if(e != 0){
                                        T base;
                                        if(e < 0){
                                                base = 0.1;
                                                e = -e;
                                        }else{
                                                base = 10;
                                        }
       
                                        while(e != 1){
                                                if((e & 1) == 0){
                                                        base = base*base;
                                                        e >>= 1;
                                                }else{
                                                        x *= base;
                                                        --e;
                                                }
                                        }
                                        x *= base;
                                }
                        }else{
                                if(*col != '\0')
                                        throw error::no_digit();
                        }

                        if(is_neg)
                                x = -x;
                }

                template<class overflow_policy> void parse(char*col, float&x) { parse_float(col, x); }
                template<class overflow_policy> void parse(char*col, double&x) { parse_float(col, x); }
                template<class overflow_policy> void parse(char*col, long double&x) { parse_float(col, x); }

                template<class overflow_policy, class T>
                void parse(char*col, T&x){
                        // GCC evalutes "false" when reading the template and
                        // "sizeof(T)!=sizeof(T)" only when instantiating it. This is why
                        // this strange construct is used.
                        static_assert(sizeof(T)!=sizeof(T),
                                "Can not parse this type. Only buildin integrals, floats, char, char*, const char* and std::string are supported");
                }

        }

        template<unsigned column_count,
                class trim_policy = trim_chars<' ', '\t'>,
                class quote_policy = no_quote_escape<','>,
                class overflow_policy = throw_on_overflow,
                class comment_policy = no_comment
        >
        class CSVReader{
        private:
                LineReader in;

                char*(row[column_count]);
                std::string column_names[column_count];

                std::vector<int>col_order;

                template<class ...ColNames>
                void set_column_names(std::string s, ColNames...cols){
                        column_names[column_count-sizeof...(ColNames)-1] = std::move(s);
                        set_column_names(std::forward<ColNames>(cols)...);
                }

                void set_column_names(){}


        public:
                CSVReader() = delete;
                CSVReader(const CSVReader&) = delete;
                CSVReader&operator=(const CSVReader&);

                template<class ...Args>
                explicit CSVReader(Args...args):in(std::forward<Args>(args)...){
                        std::fill(row, row+column_count, nullptr);
                        col_order.resize(column_count);
                        for(unsigned i=0; i<column_count; ++i)
                                col_order[i] = i;
                        for(unsigned i=1; i<=column_count; ++i)
                                column_names[i-1] = "col"+std::to_string(i);
                }

                template<class ...ColNames>
                void read_header(ignore_column ignore_policy, ColNames...cols){
                        static_assert(sizeof...(ColNames)>=column_count, "not enough column names specified");
                        static_assert(sizeof...(ColNames)<=column_count, "too many column names specified");
                        try{
                                set_column_names(std::forward<ColNames>(cols)...);

                                char*line;
                                do{
                                        line = in.next_line();
                                        if(!line)
                                                throw error::header_missing();
                                }while(comment_policy::is_comment(line));

                                detail::parse_header_line
                                        <column_count, trim_policy, quote_policy>
                                        (line, col_order, column_names, ignore_policy);
                        }catch(error::with_file_name&err){
                                err.set_file_name(in.get_truncated_file_name());
                                throw;
                        }
                }

                template<class ...ColNames>
                void set_header(ColNames...cols){
                        static_assert(sizeof...(ColNames)>=column_count,
                                "not enough column names specified");
                        static_assert(sizeof...(ColNames)<=column_count,
                                "too many column names specified");
                        set_column_names(std::forward<ColNames>(cols)...);
                        std::fill(row, row+column_count, nullptr);
                        col_order.resize(column_count);
                        for(unsigned i=0; i<column_count; ++i)
                                col_order[i] = i;
                }

                bool has_column(const std::string&name) const {
                        return col_order.end() != std::find(
                                col_order.begin(), col_order.end(),
                                        std::find(std::begin(column_names), std::end(column_names), name)
                                - std::begin(column_names));
                }

                void set_file_name(const std::string&file_name){
                        in.set_file_name(file_name);
                }

                void set_file_name(const char*file_name){
                        in.set_file_name(file_name);
                }

                const char*get_truncated_file_name()const{
                        return in.get_truncated_file_name();
                }

                void set_file_line(unsigned file_line){
                        in.set_file_line(file_line);
                }

                unsigned get_file_line()const{
                        return in.get_file_line();
                }

        private:
                void parse_helper(std::size_t r){}

                template<class T, class ...ColType>
                void parse_helper(std::size_t r, T&t, ColType&...cols){                        
                        if(row[r]){
                                try{
                                        try{
                                                ::io::detail::parse<overflow_policy>(row[r], t);
                                        }catch(error::with_column_content&err){
                                                err.set_column_content(row[r]);
                                                throw;
                                        }
                                }catch(error::with_column_name&err){
                                        err.set_column_name(column_names[r].c_str());
                                        throw;
                                }
                        }
                        parse_helper(r+1, cols...);
                }

       
        public:
                template<class ...ColType>
                bool read_row(ColType& ...cols){
                        static_assert(sizeof...(ColType)>=column_count,
                                "not enough columns specified");
                        static_assert(sizeof...(ColType)<=column_count,
                                "too many columns specified");
                        try{
                                try{
       
                                        char*line;
                                        do{
                                                line = in.next_line();
                                                if(!line)
                                                        return false;
                                        }while(comment_policy::is_comment(line));
                                       
                                        detail::parse_line<trim_policy, quote_policy>
                                                (line, row, col_order);
               
                                        parse_helper(0, cols...);
                                }catch(error::with_file_name&err){
                                        err.set_file_name(in.get_truncated_file_name());
                                        throw;
                                }
                        }catch(error::with_file_line&err){
                                err.set_file_line(in.get_file_line());
                                throw;
                        }

                        return true;
                }
        };
}
#endif

