/*
The MIT License (MIT)

Copyright (c) 2015 Julius Ikkala

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/* This program searches for all possible combinations of two words that can
 * be subjected to "sananmuunnos" https://en.wikipedia.org/wiki/Sananmuunnos
 * You can optionally define a word or a regex to search for. Requires C++11.
 * */
#include <cstdio>
#include <string>
#include <iostream>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <unordered_set>
#include <regex>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <thread>
#define MAX_THREADS  64
#ifdef NDEBUG
    #define STATIC static
#else
    #define STATIC
#endif
static const char vowels[][4]={"a","e","i","o","u","y","å","ä","ö","é"};
struct word_data
{
    size_t mora_len;
    bool has_long_vowel;
    const char* vowel;
};
struct dictionary_entry
{
    dictionary_entry(const std::string& word, const word_data& data)
    : word(word), data(data)
    {}
    std::string word;
    word_data data;
};
//Used for checking if the formed string is a proper word (O(1) lookup)
typedef std::unordered_set<std::string> search_dictionary;
//Used for actually iterating through words. (simple iteration, random access
//iterator, sorted)
typedef std::vector<dictionary_entry> dictionary;

STATIC bool word_in_dictionary(
    const std::string& word,
    const search_dictionary& sdict
){
    return sdict.count(word);
}
STATIC search_dictionary search_dictionary_from_dictionary(
    const dictionary& dict
){
    search_dictionary sdict;
    for(const dictionary_entry& entry: dict)
    {
        sdict.insert(entry.word);
    }
    return sdict;
}
STATIC word_data analyze_word(const std::string& word)
{
    const char* word_str=nullptr;
    word_data data;
    data.mora_len=0;
    data.has_long_vowel=false;
    data.vowel=nullptr;
    for(word_str=word.c_str();!data.vowel&&*word_str!=0;word_str++)
    {
        // Check if the character is a vowel
        for(size_t i=0;i<sizeof(vowels)/sizeof(const char[4]);++i)
        {
            size_t vlen=strlen(vowels[i]);
            if(strncmp(word_str, vowels[i], vlen)==0)
            {
                data.vowel=vowels[i];
                word_str+=vlen;
                if(strncmp(word_str, data.vowel, vlen)==0)
                {
                    data.has_long_vowel=true;
                    word_str+=vlen;
                }
                break;
            }
        }
    }
    data.mora_len=word_str-word.c_str()-1;
    return data;
}
STATIC void dictionary_add(
    dictionary& dict,
    const std::string& word
){
    dict.push_back(dictionary_entry(word, analyze_word(word)));
}
STATIC dictionary read_dictionary(const char* path)
{
    dictionary dict;
    std::ifstream file;
    file.open(path, std::ios::in|std::ios::binary);
    if(!file)
    {
        throw std::runtime_error(std::string(path)+": Unable to read file");
    }
    std::string str;
    std::set<std::string> unique_lines;
    while(std::getline(file, str, '\n'))
    {
        if(str.size()!=0)
        {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            unique_lines.insert(str);
        }
    }
    dict.reserve(unique_lines.size());
    for(const std::string& line: unique_lines)
    {
        dictionary_add(dict, line);
    }
    file.close();
    return dict;
}
STATIC bool sananmuunnos(
    const std::string& word1,
    const word_data& word1_data,
    const std::string& word2,
    const word_data& word2_data,
    std::string& out_word1,
    std::string& out_word2
){
    out_word1.clear();
    out_word2.clear();

    size_t shortest_len=
        word1_data.mora_len<word2_data.mora_len?
        word1_data.mora_len:word2_data.mora_len;
    if(word1_data.vowel==nullptr||word2_data.vowel==nullptr
       ||strncmp(word1.c_str(), word2.c_str(), shortest_len)==0
       ||strcmp(
            word1.c_str()+word1_data.mora_len,
            word2.c_str()+word2_data.mora_len)==0
    ){
        return false;
    }
    if(word1_data.has_long_vowel==word2_data.has_long_vowel){
        out_word1.append(word2, 0, word2_data.mora_len);
        out_word2.append(word1, 0, word1_data.mora_len);
    }
    else if(word1_data.has_long_vowel==false&&word2_data.has_long_vowel==true)
    {
        out_word1.append(
            word2,
            0,
            word2_data.mora_len-strlen(word2_data.vowel)
        );
        out_word2.append(word1, 0, word1_data.mora_len);
        out_word2.append(word1_data.vowel);
    }
    else
    {
        out_word2.append(
            word1,
            0,
            word1_data.mora_len-strlen(word1_data.vowel)
        );
        out_word1.append(word2, 0, word2_data.mora_len);
        out_word1.append(word2_data.vowel);
    }
    out_word1.append(word1, word1_data.mora_len, std::string::npos);
    out_word2.append(word2, word2_data.mora_len, std::string::npos);
    return true;
}
STATIC void search_and_print_worker(
    dictionary::const_iterator dict1_begin,
    dictionary::const_iterator dict1_end,
    const dictionary& dict2,
    const search_dictionary& sdict,
    std::vector<std::string>& formatted
){
    std::string out_word1, out_word2;
    for(dictionary::const_iterator it=dict1_begin;it!=dict1_end;++it)
    {
        const dictionary_entry& entry1=*it;
        for(const dictionary_entry& entry2: dict2)
        {
            sananmuunnos(
                entry1.word,
                entry1.data,
                entry2.word,
                entry2.data,
                out_word1,
                out_word2
            );
            if(word_in_dictionary(out_word1, sdict)&&
               word_in_dictionary(out_word2, sdict))
            {
                std::stringstream ss;
                ss<<entry1.word<<" "<<entry2.word<<" => "<<out_word1<<" "
                  <<out_word2;
                formatted.push_back(ss.str());
            }
        }
    }
}
STATIC void print_vector(const std::vector<std::string>& v)
{
    for(const std::string& str: v)
    {
        std::cout<<str<<std::endl;
    }
}
STATIC size_t search_and_print(
    const dictionary& dict1,
    const dictionary& dict2,
    const search_dictionary& sdict,
    unsigned threads
){
    size_t n=0;
    //Ensure that all there aren't unnecessarily many threads.
    while(threads!=0&&dict1.size()/threads==0)
    {
        threads/=2;
    }
    if(threads>1)
    {
        std::vector<std::thread> pool;
        std::vector<std::vector<std::string> > output(threads);

        size_t sector_length=dict1.size()/threads;
        dictionary::const_iterator begin=dict1.begin()+sector_length;
        dictionary::const_iterator end=begin+sector_length;
        //spawn worker threads
        for(unsigned i=1;i<threads;++i)
        {
            pool.push_back(
                std::thread(
                    search_and_print_worker,
                    begin,
                    (i==threads-1?dict1.end():end),
                    std::cref(dict2),
                    std::cref(sdict),
                    std::ref(output[i])
                )
            );
            begin=end;
            end+=sector_length;
        }
        //Start working on the first sector
        search_and_print_worker(
            dict1.begin(),
            dict1.begin()+sector_length,
            dict2,
            sdict,
            output[0]
        );
        n+=output[0].size();
        print_vector(output[0]);
        for(unsigned i=1;i<threads;++i)
        {
            pool[i-1].join();
            n+=output[i].size();
            print_vector(output[i]);
        }
    }
    else
    {
        std::vector<std::string> formatted;
        search_and_print_worker(
            dict1.begin(),
            dict1.end(),
            dict2,
            sdict,
            formatted
        );
        n=formatted.size();
        print_vector(formatted);
    }
    return n;
}
enum search_type
{
    SEARCH_ALL,
    SEARCH_WORD,
    SEARCH_REGEX
};
struct arguments {
    std::string dictionary_path;
    search_type search;
    std::string search_word;
    unsigned threads;
};
bool parse_arguments(int argc, char** argv, arguments& args)
{
    args.search=SEARCH_ALL;
    args.search_word="";
    args.threads=0;
    if(argc<2)
    {
        return false;
    }
    args.dictionary_path=argv[1];
    for(int i=2;i<argc;++i)
    {
        if(strcmp("-r", argv[i])==0)
        {
            i++;
            if(i>=argc) return false;
            args.search=SEARCH_REGEX;
            args.search_word=argv[i];
        }
        else if(strcmp("-t", argv[i])==0)
        {
            i++;
            if(i>=argc) return false;
            char* endptr=nullptr;
            args.threads=strtoul(argv[i], &endptr, 10);
            if(endptr!=argv[i]+strlen(argv[i]))
            {
                return false;
            }
        }
        else
        {
            args.search=SEARCH_WORD;
            args.search_word=argv[i];
        }
    }
    return true;
}
int main(int argc, char** argv)
{
    arguments args;
    if(!parse_arguments(argc, argv, args))
    {
        std::cerr<<"Usage: "<<argv[0]
                 <<" dictionary [word|-r regex] [-t threads]\n";
        return 1;
    }
    if(args.threads>MAX_THREADS)
    {
        std::cerr<<"Too many threads specified! The maximum is "<<MAX_THREADS
                 <<"."<<std::endl;
        return 2;
    }
    try {
        dictionary dict(read_dictionary(args.dictionary_path.c_str()));
        search_dictionary sdict(search_dictionary_from_dictionary(dict));
        size_t n=0;
        if(args.search==SEARCH_ALL)
        {
            n=search_and_print(dict, dict, sdict, args.threads);
        }
        else
        {
            dictionary from;
            if(args.search==SEARCH_WORD)
            {
                dictionary_add(from, args.search_word);
            }
            else if(args.search==SEARCH_REGEX)
            {
                std::regex regex(
                    args.search_word,
                    std::regex_constants::grep|std::regex_constants::icase
                );
                std::cout<<"Searching for following regex matches: "
                         <<std::endl;
                for(const dictionary_entry& entry: dict)
                {
                    if(std::regex_match(entry.word, regex))
                    {
                        dictionary_add(from, entry.word);
                        std::cout<<"\t"<<entry.word<<std::endl;
                    }
                }
            }
            else
            {
                assert(false);
            }
            n=search_and_print(from, dict, sdict, args.threads);
        }
        std::cout<<n<<" found "<<std::endl;
    }
    catch(std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}
