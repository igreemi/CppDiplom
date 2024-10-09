#pragma once

#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

struct SpiderReadConfig 
{
    std::string db_host;
    std::string db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;

    std::string start_url;
    std::string client_port;
    std::string server_port;
    int max_depth;

    void LoadConfig(std::string const& filename) 
    {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(filename, pt);

        db_host = pt.get<std::string>("Database.host");
        db_port = pt.get<std::string>("Database.port");
        db_name = pt.get<std::string>("Database.name");
        db_user = pt.get<std::string>("Database.user");
        db_password = pt.get<std::string>("Database.pass");

        start_url = pt.get<std::string>("Url.start_url");
        client_port = pt.get<std::string>("Url.client_port");
        server_port = pt.get<std::string>("Url.server_port");
        max_depth = pt.get<int>("Url.max_depth");
    }

    void PrintConfig() const
    {
        std::cout << "db_host = " << db_host << std::endl;
        std::cout << "db_port = " << db_port << std::endl;
        std::cout << "db_name = " << db_name << std::endl;
        std::cout << "db_user = " << db_user << std::endl;
        std::cout << "db_password = " << db_password << std::endl;
        std::cout << "start_url = " << start_url << std::endl;
        std::cout << "client_port = " << client_port << std::endl;
        std::cout << "server_port = " << server_port << std::endl;
        std::cout << "max_depth = " << max_depth << std::endl;
    }
};