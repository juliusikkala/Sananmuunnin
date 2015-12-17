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
#include <map>
#include <regex>
#include <algorithm>
#include <cstring>
#include <thread>
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
typedef std::map<std::string, word_data> dictionary;
STATIC bool word_in_dictionary(
    const std::string& word,
    const dictionary& dict
){
    return dict.count(word);
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
    dict.insert(std::pair<std::string, word_data>(word, analyze_word(word)));
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
    while(std::getline(file, str, '\n'))
    {
        if(str.size()!=0)
        {
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            dictionary_add(dict, str);
        }
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
STATIC size_t search_and_print(
    const dictionary& search_dict,
    const dictionary& dict
){
    std::string out_word1, out_word2;
    size_t n=0;
    for(const auto& search_it: search_dict)
    {
        for(const auto& it: dict)
        {
            sananmuunnos(
                search_it.first,
                search_it.second,
                it.first,
                it.second,
                out_word1,
                out_word2
            );
            if(word_in_dictionary(out_word1, dict)&&
               word_in_dictionary(out_word2, dict))
            {
                n++;
                std::cout<<search_it.first<<" "<<it.first<<" => "<<out_word1
                         <<" "<<out_word2<<std::endl;
            }
        }
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
    try {
        dictionary dict(read_dictionary(args.dictionary_path.c_str()));
        size_t n=0;
        if(args.search==SEARCH_ALL)
        {
            n=search_and_print(dict, dict);
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
                for(const auto& it: dict)
                {
                    if(std::regex_match(it.first, regex))
                    {
                        dictionary_add(from, it.first);
                        std::cout<<"\t"<<it.first<<std::endl;
                    }
                }
            }
            else
            {
                assert(false);
            }
            n=search_and_print(from, dict);
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
