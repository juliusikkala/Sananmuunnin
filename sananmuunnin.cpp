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
#include <regex>
#include <algorithm>
#include <cstring>
#include <thread>
#ifdef NDEBUG
    #define STATIC static
#else
    #define STATIC
#endif
STATIC std::vector<std::string> read_lines_tolower(const char* path)
{
    std::vector<std::string> res;
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
            res.push_back(str);
        }
    }
    file.close();
    return res;
}
STATIC void analyze_initial(
    const std::string& word,
    size_t& len,
    bool& has_long_vowel,
    const char*& vowel
){
    static const char vowels[][4]={"a","e","i","o","u","y","å","ä","ö","é"};
    const char* word_str=nullptr;
    vowel=nullptr;
    has_long_vowel=false;
    len=0;
    for(word_str=word.c_str();!vowel&&*word_str!=0;word_str++)
    {
        // Check if the character is a vowel
        for(size_t i=0;i<sizeof(vowels)/sizeof(const char[4]);++i)
        {
            size_t vlen=strlen(vowels[i]);
            if(strncmp(word_str, vowels[i], vlen)==0)
            {
                vowel=vowels[i];
                word_str+=vlen;
                if(strncmp(word_str, vowel, vlen)==0)
                {
                    has_long_vowel=true;
                    word_str+=vlen;
                }
                break;
            }
        }
    }
    len=word_str-word.c_str()-1;
}
STATIC bool sananmuunnos(
    const std::string& word1,
    const std::string& word2,
    std::string& out_word1,
    std::string& out_word2
){
    out_word1.clear();
    out_word2.clear();
    
    size_t word1_len=0;
    bool word1_has_long_vowel=false;
    const char* word1_vowel=nullptr;
    analyze_initial(word1, word1_len, word1_has_long_vowel, word1_vowel);
    
    size_t word2_len=0;
    bool word2_has_long_vowel=false;
    const char* word2_vowel=nullptr;
    analyze_initial(word2, word2_len, word2_has_long_vowel, word2_vowel);

    size_t shortest_len=word1_len<word2_len?word1_len:word2_len;
    if(word1_vowel==nullptr||word2_vowel==nullptr
       ||strncmp(word1.c_str(), word2.c_str(), shortest_len)==0
       ||strcmp(word1.c_str()+word1_len, word2.c_str()+word2_len)==0)
    {
        return false;
    }
    if(word1_has_long_vowel==word2_has_long_vowel){
        out_word1.append(word2, 0, word2_len);
        out_word2.append(word1, 0, word1_len);
    }
    else if(word1_has_long_vowel==false&&word2_has_long_vowel==true)
    {
        out_word1.append(word2, 0, word2_len-strlen(word2_vowel));
        out_word2.append(word1, 0, word1_len);
        out_word2.append(word1_vowel);
    }
    else
    {
        out_word2.append(word1, 0, word1_len-strlen(word1_vowel));
        out_word1.append(word2, 0, word2_len);
        out_word1.append(word2_vowel);
    }
    out_word1.append(word1, word1_len, std::string::npos);
    out_word2.append(word2, word2_len, std::string::npos);
    return true;
}
template<typename T>
STATIC bool is_in_sorted_vector(const T& t, const std::vector<T>& vec)
{
    auto it=std::lower_bound(vec.begin(), vec.end(), t);
    return it!=vec.end()&&*it==t;
}
STATIC size_t search_and_print(
    const std::vector<std::string>& search_words,
    const std::vector<std::string>& all_words
){
    std::string out_word1, out_word2;
    size_t n=0;
    for(const std::string& word1: search_words)
    {
        for(const std::string& word2: all_words)
        {
            sananmuunnos(word1, word2, out_word1, out_word2);
            if(is_in_sorted_vector(out_word1, all_words)&&
               is_in_sorted_vector(out_word2, all_words))
            {
                n++;
                std::cout<<word1<<" "<<word2<<" => "<<out_word1<<" "
                         <<out_word2<<std::endl;
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
        std::vector<std::string> words(
            read_lines_tolower(args.dictionary_path.c_str())
        );
        /* The vector must be sorted so that it can be used with lower_bound*/
        std::sort(words.begin(), words.end());
        size_t n=0;
        if(args.search==SEARCH_ALL)
        {
            n=search_and_print(words, words);
        }
        else
        {
            std::vector<std::string> from;
            if(args.search==SEARCH_WORD)
            {
                from.push_back(args.search_word);
            }
            else if(args.search==SEARCH_REGEX)
            {
                std::regex regex(
                    args.search_word,
                    std::regex_constants::grep|std::regex_constants::icase
                );
                std::cout<<"Searching for following regex matches: "
                         <<std::endl;
                for(const std::string& word: words)
                {
                    if(std::regex_match(word, regex))
                    {
                        from.push_back(word);
                        std::cout<<"\t"<<word<<std::endl;
                    }
                }
            }
            else
            {
                assert(false);
            }
            n=search_and_print(from, words);
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
