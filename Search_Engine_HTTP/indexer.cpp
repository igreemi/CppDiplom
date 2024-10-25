#include "indexer.h"

Indexer::Indexer()
{

}

Indexer::~Indexer()
{
    delete txn;
    delete connect_db;
}

void Indexer::ConnectDB(SpiderReadConfig config)
{
    connect_db = new pqxx::connection
    ("host = " + config.db_host +
        " port = " + config.db_port +
        " dbname = " + config.db_name +
        " user = " + config.db_user +
        " password = " + config.db_password);

    txn = new pqxx::work(*connect_db);

    txn->exec("CREATE TABLE IF NOT EXISTS pages (id SERIAL PRIMARY KEY, url TEXT UNIQUE NOT NULL);");
    txn->exec("CREATE TABLE IF NOT EXISTS words (id SERIAL PRIMARY KEY, word TEXT NOT NULL);");
    txn->exec("CREATE TABLE IF NOT EXISTS page_words (page_id INTEGER REFERENCES pages(id), word_id INTEGER REFERENCES words(id), count INTEGER DEFAULT 0, PRIMARY KEY (page_id, word_id));");

    connect_db->prepare("insert_page", "INSERT INTO pages (url) VALUES ($1) ON CONFLICT (url) DO NOTHING RETURNING id");
    connect_db->prepare("insert_word", "INSERT INTO words (word) VALUES ($1) RETURNING id");
    connect_db->prepare("insert_page_word", "INSERT INTO page_words (page_id, word_id, count) VALUES ($1, $2, $3)");

    txn->commit();
}

std::string Indexer::CleanText(std::string& text)
{
    boost::regex regex;

    regex.assign("<[^>]*>");
    std::string tmp = boost::regex_replace(text, regex, " ");

    return tmp;
}

std::string Indexer::LowerCases(const std::string& word, const std::string locale_name)
{
    boost::locale::generator gen;
    std::locale loc = gen(locale_name);
    return boost::locale::to_lower(word, loc);
}

std::map<std::string, int> Indexer::AnalyzeWords(std::string text)
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

void Indexer::IndexingPages(std::string url, std::string words)
{
    std::map tmp_vocabulary = AnalyzeWords(words);
    pqxx::result page_res = txn->exec_prepared("insert_page", url);

    int page_id = -1;
    if (!page_res.empty())
    {
        page_id = page_res[0]["id"].as<int>();

        for (auto& [word, count] : tmp_vocabulary)
        {
            pqxx::result word_res = txn->exec_prepared("insert_word", word);
            if(!word_res.empty())
            {
                int word_id = word_res[0]["id"].as<int>();
                txn->exec_prepared("insert_page_word", page_id, word_id, count);
            }
        }
        
    }

    connect_db->close();
}
