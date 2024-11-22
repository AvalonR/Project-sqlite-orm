#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

using namespace std;

struct Book {
    int id, author_id;
    string title, genre;
    bool is_borrowed{};
};

struct author {
    int id;
    string name;
};

struct borrower {
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

    // Define and initialize the database storage
    auto storage = make_storage(
            "library.db",
            make_table(
                    "author",
                    make_column("id", &author::id, primary_key()),
                    make_column("name", &author::name)
            ),
            make_table(
                    "borrower",
                    make_column("id", &borrower::id, primary_key()),
                    make_column("name", &borrower::name),
                    make_column("email", &borrower::email)
            ),
            make_table(
                    "BorrowRecord",
                    make_column("id", &BorrowRecord::id, primary_key()),
                    make_column("book_id", &BorrowRecord::book_id),  // Fixed "book id" to "book_id"
                    make_column("borrower_id", &BorrowRecord::borrower_id),
                    make_column("borrow_date", &BorrowRecord::borrow_date),
                    make_column("return_date", &BorrowRecord::return_date)
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
