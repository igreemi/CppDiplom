#include "indexer.h"
#pragma execution_character_set("utf-8")

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
    txn->exec("CREATE TABLE IF NOT EXISTS words (id SERIAL PRIMARY KEY, word TEXT UNIQUE NOT NULL);");
    txn->exec("CREATE TABLE IF NOT EXISTS page_words (page_id INTEGER REFERENCES pages(id), word_id INTEGER REFERENCES words(id), count INTEGER DEFAULT 0, PRIMARY KEY (page_id, word_id));");

    connect_db->prepare("insert_page", "INSERT INTO pages (url) VALUES ($1) ON CONFLICT (url) DO NOTHING RETURNING id");
    connect_db->prepare("insert_word", "INSERT INTO words (word) VALUES ($1) ON CONFLICT (word) DO NOTHING RETURNING id");
    connect_db->prepare("insert_page_word", "INSERT INTO page_words (page_id, word_id, count) VALUES ($1, $2, $3) ON CONFLICT (page_id, word_id) DO UPDATE SET count = $3");

    txn->commit();

    //std::cout << "ТАБЛИЦЫ СОЗДАНЫ" << std::endl;
}

std::string Indexer::CleanText(std::string& text)
{
    boost::locale::generator gen;
    std::locale loc = gen("ru_RU.UTF-8");

    boost::regex regex;

    //regex.assign("(<script.*>.*?</script>)");
    regex.assign("<[^>]*>");
    std::string tmp = boost::regex_replace(text, regex, " ");

    //при повторной фильтрации начинает тормозить и слова на кирилице разбивает
    //regex.assign("[^а-яА-ЯёЁa-zA-Z0-9]");
    //regex.assign("<(style|link)[^>]*>.*?</(style|link)>");
    //tmp = boost::regex_replace(tmp, regex, " ");

    return tmp;
}

std::string Indexer::LowerCases(const std::string& word, const std::string& locale_name)
{
    boost::locale::generator gen;
    std::locale loc = gen(locale_name);
    return boost::locale::to_lower(word, loc);
}

std::map<std::string, int> Indexer::AnalyzeWords(std::string &text)
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

void Indexer::PrintVocabulary(std::map<std::string, int>& vocabulary)
{
    std::cout << "WORDS-|-QUANTITY" << std::endl;
    for (auto& [word, count] : vocabulary)
    {
        std::cout << word << " | " << count << std::endl;
    }
}

void Indexer::IndexingPages(std::string url, std::string words) //std::map<std::string, int>& vocabulary)
{
    std::map tmp_vocabulary = AnalyzeWords(words);
    pqxx::result page_res = txn->exec_prepared("insert_page", url);
    //std::cout << "URL " << url << " dobavlen" << " thread: " << std::this_thread::get_id() << std::endl;

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
    else
    {
        //std::cout << "URL " << url << " sushestvuet" << " thread: " << std::this_thread::get_id() << std::endl;
        
    }
    connect_db->close();
}