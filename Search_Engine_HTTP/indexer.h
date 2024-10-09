#pragma once

#include <iostream>
#include <pqxx/pqxx>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <unicode/ustream.h>

#include <string>
#include <regex>
#include <vector>
#include <map>

#include "spider_read_config.h"

class Indexer {

private:
    pqxx::connection* connect_db = nullptr;
    pqxx::work *txn = nullptr;

    std::string _host = ""; // = "localhost";
    std::string _port = ""; // = "5432";
    std::string _name_db = ""; // = "test_se";
    std::string _user = ""; // = "postgres";
    std::string _pass = ""; // = "123";

    std::string CleanText(std::string& text);

public:
    boost::locale::generator gen;

    Indexer();

    ~Indexer();

    void ConnectDB(SpiderReadConfig config);

    std::string LowerCases(const std::string& word, const std::string& locale_name);

    std::map<std::string, int> AnalyzeWords(std::string &text);

    void PrintVocabulary(std::map<std::string, int>& vocabulary);

    void IndexingPages(std::string url, std::string words); // std::map<std::string, int>& vocabulary);

};