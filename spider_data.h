#pragma once

#include <pqxx/pqxx>
#include "spider_read_config.h"

struct SpiderData {
    pqxx::connection db;
    SpiderReadConfig config;
    std::queue<std::string> links;
    std::mutex links_mutex;

    void OpenDB() {
        std::string db_string = "host=" + config.db_host + " port=" + config.db_port + " dbname=" + config.db_name + " user=" + config.db_user + " password=" + config.db_password;
        db = pqxx::connection(db_string);
    }

    void CreateTables() {
        pqxx::work w(db);
        w.exec("CREATE TABLE IF NOT EXISTS words (id SERIAL PRIMARY KEY, word TEXT, frequency INTEGER)");
        w.commit();
    }
};