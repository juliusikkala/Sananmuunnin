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
#include <fstream>
#include <stdexcept>
#include <vector>
#include <set>
#include <regex>
#include <algorithm>
#include <cstring>
static std::set<std::string> read_lines_tolower(const char* path)
{
    std::set<std::string> res;
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
            res.insert(str);
        }
    }
    file.close();
    return res;
}
static void analyze_initial(
    const std::string& word,
    size_t& len,
    bool& has_long_vowel,
    const char*& vowel
){
    static const char* vowels[]={"a","e","i","o","u","y","å","ä","ö","é"};
    const char* word_str=word.c_str();
    vowel=nullptr;
    has_long_vowel=false;
    len=0;
    while(*word_str!=0)
    {
        // Check if the character is a vowel
        for(size_t i=0;i<sizeof(vowels)/sizeof(const char*);++i)
        {
            size_t vlen=strlen(vowels[i]);
            if(strncmp(word_str, vowels[i], vlen)==0)
            {
                vowel=vowels[i];
                word_str+=vlen;
                break;
            }
        }
        if(vowel)
        {
            if(strncmp(word_str, vowel, strlen(vowel))==0)
            {
                has_long_vowel=true;
                word_str+=strlen(vowel);
            }
            break;
        }
        else
        {
            word_str++;
        }
    }
    len=word_str-word.c_str();
}
static bool sananmuunnos(
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
static size_t search_and_print(
    const std::set<std::string>& search_words,
    const std::set<std::string>& all_words
){
    std::string out_word1, out_word2;
    size_t n=0;
    for(const std::string& word1: search_words)
    {
        for(const std::string& word2: all_words)
        {
            sananmuunnos(word1, word2, out_word1, out_word2);
            if(all_words.count(out_word1)&&all_words.count(out_word2))
            {
                n++;
                std::cout<<word1<<" "<<word2<<" => "<<out_word1<<" "<<out_word2<<std::endl;
            }
        }
    }
    return n;
}
int main(int argc, char** argv)
{
    if(argc<2||argc>4||(argc==4&&strcmp(argv[2], "-r")!=0))
    {
        std::cerr<<"Usage: "<<argv[0]<<" wordlist [word|-r regex]\n";
        return 1;
    }
    try {
        std::set<std::string> words(read_lines_tolower(argv[1]));
        if(argc==2)
        {
            search_and_print(words, words);
        }
        else
        {
            std::set<std::string> from;
            if(argc==3)
            {
                from.insert(argv[2]);
            }
            else if(argc==4)
            {
                std::regex regex(
                    argv[3],
                    std::regex_constants::grep|std::regex_constants::icase
                );
                std::cout<<"Searching for following regex matches: "
                         <<std::endl;
                for(const std::string& word: words)
                {
                    if(std::regex_match(word, regex))
                    {
                        from.insert(word);
                        std::cout<<"\t"<<word<<std::endl;
                    }
                }
            }
            size_t n=search_and_print(from, words);
            std::cout<<n<<" found "<<std::endl;
        }
    
    }
    catch(std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return 1;
    }
    return 0;
}
