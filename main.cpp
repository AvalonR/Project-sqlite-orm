#include <iostream>
#include <sqlite_orm/sqlite_orm.h>

using namespace std;

struct Book {
    int id, author_id;
    string title, genre;
    bool is_borrowed{};
};

struct BorrowRecord
{
    int id{}, book_id{}, borrower_id{};
    string borrow_date, return_date;
};

int main() {
    int id, bookid, borrowerid;
    string borrowdate, returndate, valid;
    using namespace sqlite_orm;
    auto storage = make_storage(
                "library",

                make_table("BorrowRecord",
                    make_column("id", &BorrowRecord::id, primary_key()),
                    make_column("book id", &BorrowRecord::book_id),
                    make_column("borrower_id", &BorrowRecord::borrower_id),
                    make_column("borrow_date", &BorrowRecord::borrow_date),
                    make_column("return_date", &BorrowRecord::return_date))
                    );
    storage.vacuum();
    storage.sync_schema();
    int first = 0;
    while (true)
    {
        if (first == 0)
        {
            cout << "Please add the first Borrow Record:\n";
            valid = "Y";
        }
        else
        {
            cout << "Do you want to add another Borrow Record\n[Y] Yes\n[N] No\n";
            cin >> valid;
        }
        first++;
        if (valid == "Y")
        {
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
        }
        else
        {
            break;
        }
    }
    cout << "/ID/BOOK ID/BORROWER ID/BORROW DATE/RETURN DATE/"<<endl;
    cout << "------------------------------------------------" << endl;
    for (const auto& [id, book_id, borrower_id, borrow_date, return_date] : storage.get_all<BorrowRecord>())
    {
        cout << id << endl << book_id << endl << borrower_id << endl << borrow_date << endl << return_date << endl << "------------------------------------------------" << endl;
    }
    storage.remove_all<BorrowRecord>();
    return 0;
}
