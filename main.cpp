#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

using namespace std;

struct Book {
    int id, author_id;
    string title, genre;
    bool is_borrowed{};
};
struct Author {
    int id;
    string name;
};
struct Borrower {
    int id;
    string name;
    string email;
};
struct BorrowRecord {
    int id{}, book_id{}, borrower_id{};
    string borrow_date, return_date;
};

int main() {
    using namespace sqlite_orm;

    auto storage = make_storage(
            "library.db",

            make_table(
               "Author",
               make_column("id", &Author::id, primary_key()),
               make_column("name", &Author::name)
       ),
            make_table(
                    "Book",
                    make_column("id", &Book::id, primary_key()),
                    make_column("author_id", &Book::author_id), //Explicit foreign key
                    make_column("title", &Book::title),
                    make_column("genre", &Book::genre),
                    make_column("is_borrowed", &Book::is_borrowed),
                    foreign_key(&Book::author_id).references(&Author::id)
            ),

            make_table(
                    "Borrower",
                    make_column("id", &Borrower::id, primary_key()),
                    make_column("name", &Borrower::name),
                    make_column("email", &Borrower::email)
            ),
            make_table(
                //Junction table (many-to-many relationship, connects to borrower and book)
                    "BorrowRecord",
                    make_column("id", &BorrowRecord::id, primary_key()),
                    make_column("book_id", &BorrowRecord::book_id),  // Fixed "book id" to "book_id"
                    make_column("borrower_id", &BorrowRecord::borrower_id),
                    make_column("borrow_date", &BorrowRecord::borrow_date),
                    make_column("return_date", &BorrowRecord::return_date),
                    foreign_key(&BorrowRecord::book_id).references(&Book::id),
                    foreign_key(&BorrowRecord::borrower_id).references(&Borrower::id)
            )
    );

    // Sync the schema
    storage.sync_schema();

    int id, bookid, borrowerid;
    string borrowdate, returndate, valid;
    int first = 0;

    while (true) {
        if (first == 0) {
            cout << "Please add the first Borrow Record:\n";
            valid = "Y";
        } else {
            cout << "Do you want to add another Borrow Record\n[Y] Yes\n[N] No\n";
            cin >> valid;
        }
        first++;

        if (valid == "Y") {
            cout << "Enter id:\n";
            cin >> id;
            cout << "Enter book id:\n";
            cin >> bookid;
            cout << "Enter borrower id:\n";
            cin >> borrowerid;
            cout << "Enter borrow date:\n";
            cin >> borrowdate;
            cout << "Enter return date:\n";
            cin >> returndate;

            BorrowRecord ok;
            ok.id = id;
            ok.book_id = bookid;
            ok.borrower_id = borrowerid;
            ok.borrow_date = borrowdate;
            ok.return_date = returndate;

            storage.insert(ok);
        } else {
            break;
        }
    }

    // Display Borrow Records
    cout << "/ID/BOOK ID/BORROWER ID/BORROW DATE/RETURN DATE/\n";
    cout << "------------------------------------------------\n";
    for (const auto& record : storage.get_all<BorrowRecord>()) {
        cout << record.id << " | " << record.book_id << " | " << record.borrower_id
             << " | " << record.borrow_date << " | " << record.return_date << "\n";
        cout << "------------------------------------------------\n";
    }

    // Remove all records
    storage.remove_all<BorrowRecord>();

    return 0;
}
