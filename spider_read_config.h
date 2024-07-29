#pragma once

#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

struct SpiderReadConfig {
    std::string start_url;
    std::string db_host;
    std::string db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;
    int max_depth;

    void LoadConfig(std::string const& filename) 
    {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(filename, pt);

        start_url = pt.get<std::string>("url");
        db_host = pt.get<std::string>("db.host");
        db_port = pt.get<std::string>("db.port");
        db_name = pt.get<std::string>("db.name");
        db_user = pt.get<std::string>("db.user");
        db_password = pt.get<std::string>("db.password");
        max_depth = pt.get<int>("max_depth");
    }
};