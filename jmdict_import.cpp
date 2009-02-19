/*
jmdict, a frontend to the JMdict file. http://mandrill.fuxx0r.net/jmdict.php
Copyright (C) 2004 Florian Bluemel (florian.bluemel@uni-dortmund.de)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <cstdlib>
#include <exception>
#include <ostream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <stack>
#include "sqlite.h"
#include "xmlparser.h"
#include "kana2romaji.h"
using namespace std;

class Dictionary {
public:
    Dictionary(const string& name) : db(name) {
        db.exec("DROP TABLE kanji");
        db.exec("DROP TABLE reading");
	db.exec("DROP TABLE gloss");
        db.exec("CREATE TABLE kanji (entry INT NOT NULL, kanji TINYTEXT NOT NULL)");
        db.exec("CREATE TABLE reading (entry INT NOT NULL, kana TINYTEXT NOT NULL, romaji TINYTEXT NOT NULL)");
        db.exec("CREATE TABLE gloss (entry INT NOT NULL, sense INT NOT NULL, lang TINYTEXT NOT NULL, gloss TEXT NOT NULL)");
        db.exec("BEGIN");
    }

    ~Dictionary() {
        db.exec("COMMIT");
    }

    void createIndices() {
        db.exec("CREATE INDEX k_entry ON kanji (entry)");
        db.exec("CREATE INDEX r_entry ON reading (entry)");
        db.exec("CREATE INDEX r_kana ON reading (kana)");
        db.exec("CREATE INDEX r_romaji ON reading (romaji)");
        db.exec("CREATE INDEX g_entry ON gloss (entry)");
        db.exec("CREATE INDEX g_gloss ON gloss (gloss)");
    }
    
    void push(const xml::Tag& tag) {
        tags.push(tag);
    }

    xml::Tag& top() {
        return tags.top();
    }

    void pop() {
        xml::Tag& tag = top();
        if (tag.name() == "ent_seq") {
            entry_seq = atoi(tag.text().c_str());
            sense_seq = 1;
        }
        else if (tag.name() == "keb")
            insert_kanji(tag.text());
        else if (tag.name() == "reb")
            insert_reading(tag.text());
        else if (tag.name() == "sense")
            ++sense_seq;
        else if (tag.name() == "gloss")
            insert_gloss(tag.attribute("xml:lang"), tag.text());
        tags.pop();
    }

private:
    void insert_kanji(const string& kanji) {
        db.exec(sql::query("INSERT INTO kanji (entry, kanji) VALUES (%u, %Q)") % entry_seq % kanji);
    }

    void insert_reading(const string& reading) {
        string romaji;
        kana2romaji(reading, romaji);
        db.exec(sql::query("INSERT INTO reading (entry, kana, romaji) VALUES (%u, %Q, %Q)") % entry_seq % reading % romaji);
    }
    
    void insert_gloss(string lang, const string& text) {
        if (lang == "")
            lang = "en";
        db.exec(
            sql::query("INSERT INTO gloss (entry, sense, lang, gloss) "
                       "VALUES (%u, %u, %Q, %Q)") % entry_seq % sense_seq % lang % text);

        static unsigned seq = 0;
        if (++seq % 50000 == 0) {
            db.exec("COMMIT");
            db.exec("BEGIN");
        }
    }

    stack<xml::Tag> tags;
    unsigned entry_seq;
    unsigned sense_seq;
    sql::db db;
};

int main(int argc, char** argv)
try {
    if(argc < 2 || argc > 3) {
        cerr << "Usage: jmdict_import <dictfile> [dest_dir]\n";
	return EXIT_FAILURE;
    }
    
    const string dict_file = argv[1],
          database_name = argc == 2 ? DICTIONARY_PATH : string(argv[2]) + DICTIONARY_PATH;

    initRomaji();
    if (std::remove(database_name.c_str()) == 0)
        std::cout << "removed old dictionary database\n";

    Dictionary dict(database_name);
    xml::Parser<Dictionary> parser(dict);
    
    ifstream in(dict_file.c_str());
    if (!in.is_open()) {
        cerr << "could not open dictionary file '" << dict_file << "'\n";
        return EXIT_FAILURE;
    }
    cout << "filling database... " << flush;
    time_t start = time(0);
    parser.parse(in);
    cout << time(0) - start << "s" << endl;
    cout << "creating indices... " << flush;
    start = time(0);
    dict.createIndices();
    cout << time(0) - start << "s" << endl;

    return EXIT_SUCCESS;
}
catch (const std::exception& e) {
    cerr << e.what() << '\n';
    return EXIT_FAILURE;
}
