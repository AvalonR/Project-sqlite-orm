//
// Created by AdMin on 2024-11-21.
//

#ifndef MYSQL_CONNECTOR_BOOK_H
#define MYSQL_CONNECTOR_BOOK_H
#include <string>
#include <sqlite_orm/sqlite_orm.h>
using namespace std;
class book {

public:
    int id , author_id;
    string title, genre ;
    bool is_borrowed;
    book

};


#endif //MYSQL_CONNECTOR_BOOK_H
