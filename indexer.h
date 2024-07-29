#pragma once

#include <iostream>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <unicode/ustream.h>

#include <string>
#include <regex>
#include <vector>
#include <map>

class Indexer {

public:
    boost::locale::generator gen;

    Indexer() 
    {
//     std::locale("ru_RU.UTF-8");
//     std::locale loc = gen("");
//     std::locale::global(loc);
//     std::wcout.imbue(loc);
    }

    std::string CleanText(std::string& text)
    {
        boost::locale::generator gen;
        std::locale loc = gen("ru_RU.UTF-8");

//      std::string regular("[à-ÿÀ-ß¸¨]+");
//      boost::u32regex regex = boost::make_u32regex(regular);
//      std::string tmp = boost::u32regex_replace(text, regex, " ");

        boost::regex regex;
//      regex.imbue();
        regex.assign("<[^>]*>|\n|\r");

        std::string tmp = boost::regex_replace(text, regex, " ");

        regex.assign("[^A-Za-z0-9]+");

        tmp = boost::regex_replace(tmp, regex, " ");

        return tmp;
    }

    std::string LowerCases(const std::string& word, const std::string& locale_name)
    {
        boost::locale::generator gen;
        std::locale loc = gen(locale_name);
        return boost::locale::to_lower(word, loc);
    }

    std::map<std::string, int> AnalyzeWords(std::string & text) 
    {
        std::map<std::string, int> frequencies;
        std::vector<std::string> words;

        std::string tmp = CleanText(text);

        boost::split(words, tmp, boost::is_any_of(" "));

        for (std::string const& word : words) 
        {
            std::string word_cleaned = word;

            if (word_cleaned.size() >= 3 && word_cleaned.size() <= 32) 
            {
                std::string word_lower = LowerCases(word_cleaned, "ru_RU.UTF-8");
                frequencies[word_lower]++;
            }
        }

        return frequencies;
    }

    void PrintVocabulary(std::map<std::string, int> &vocabulary)
    {
        std::cout << "WORDS-|-QUANTITY" << std::endl;
        for (auto [word, count] : vocabulary)
        {
            std::cout << word << "-|-" << count << std::endl;
        }
    }

};