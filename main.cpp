#include <iostream>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

using namespace std;
using namespace sqlite_orm;

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

auto setup_database () {
    auto storage = make_storage(
        "library.db",
        //Parent Table
        make_table(
            "Author",
            make_column("id", &Author::id, primary_key()),
            make_column("name", &Author::name)
        ),
        //Child Table (for Author table)
        make_table(
            "Book",
            make_column("id", &Book::id, primary_key()),
            make_column("author_id", &Book::author_id), //Explicit foreign key
            make_column("title", &Book::title),
            make_column("genre", &Book::genre),
            make_column("is_borrowed", &Book::is_borrowed),
            foreign_key(&Book::author_id)
            .references(&Author::id)
            .on_delete.cascade() //Enables CASCADE delete (Author is deleted, all books will be deleted as well)
            .on_update.restrict_() //does not allow the author ID to be updated
        ),
        make_table(
        "Borrower",
        make_column("id", &Borrower::id, primary_key()),
        make_column("name", &Borrower::name),
        make_column("email", &Borrower::email)
        ),

        //Junction table (many-to-many relationship, connects to borrower and book)
        make_table(
        "BorrowRecord",
        make_column("id", &BorrowRecord::id, primary_key()),
        make_column("book_id", &BorrowRecord::book_id),
        make_column("borrower_id", &BorrowRecord::borrower_id),
        make_column("borrow_date", &BorrowRecord::borrow_date),
        make_column("return_date", &BorrowRecord::return_date),
        foreign_key(&BorrowRecord::book_id)
        .references(&Book::id)
        .on_delete.cascade() //enables CASCADE delete, when a book is deleted, the borrow record for that book will be deleted as well
        .on_update.cascade(), //enables CASCADE update, when book ID is updated, the book ID in borrow records will be updated as well
        foreign_key(&BorrowRecord::borrower_id)
        .references(&Borrower::id)
        .on_delete.cascade() //enables CASCADE delete, deleting a borrower, will delete the borrowers borrow record
        .on_update.restrict_() //does not allow the borrower ID to be updated
        )
    );
    // Sync the schema
    storage.sync_schema();
    cout << "Database schema initialized successfully!" << endl;
    return storage;
}
void enable_foreign_keys () {
    // Enable foreign keys using raw SQLite API
    sqlite3* db = nullptr;
    sqlite3_open("library.db", &db);

    if (db) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "Error enabling foreign keys: " << (errMsg ? errMsg : "unknown error") << endl;
            sqlite3_free(errMsg);
        } else {
            cout << "Foreign key constraints enabled." << endl;
        }

        // Check if foreign keys are enabled
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "PRAGMA foreign_keys;", -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            bool foreign_keys_enabled = sqlite3_column_int(stmt, 0) == 1;
            cout << "Foreign keys enabled: " << (foreign_keys_enabled ? "Yes" : "No") << endl;
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    } else {
        cerr << "Failed to open database." << endl;
    }
}

void addBook(auto& storage)
{
    Book book;
    cout << "end the the Book id : " << endl;
    cin >> book.id;
    cout << "end the the author id : " << endl;
    cin >> book.author_id;
    cout << "end the the Book title  : " << endl;
    cin.ignore();
    getline(cin, book.title);
    cout << "Enter Genre: ";
    getline(cin, book.genre);
    //book.is_borrowed = false;
}
void listBooks(auto& storage)
{
    cout << "\n/ID/TITLE/AUTHOR_ID/GENRE/BORROWED/\n";
    cout << "--------------------------------------\n";
    for (const auto& book : storage.template get_all<Book>())
    {
        cout << book.id << " | " << book.title << " | " << book.author_id
            << " | " << book.genre << " | " << (book.is_borrowed ? "Yes" : "No") << "\n";
        cout << "--------------------------------------\n";
    }
}
void updateBook(auto& storage)
{
    int id;
    cout << "Enter the ID of the book to update: ";
    cin >> id;

    /*if (auto book = storage.template get_pointer<Book>(id))
    {
        cout << "Enter new title (current: " << book->title << "): ";
        cin.ignore();
        getline(cin, book->title);

        cout << "Enter new genre (current: " << book->genre << "): ";
        getline(cin, book->genre);

        cout << "Enter new Author ID (current: " << book->author_id << "): ";
        cin >> book->author_id;

        storage.update(*book);
        cout << "Book updated successfully!\n";
    }
    else
    {
        cout << "Book not found!\n";
    }
    */
}
void deleteBook(auto& storage)
{
    int id;
    cout << "Enter the ID of the book to delete: ";
    cin >> id;

    /*if (storage.template remove<Book>(id))
    {
        cout << "Book deleted successfully!\n";
    }
    else
    {
        cout << "Book not found!\n";
    }
    */
}
void bookActions_switch(auto& storage) {
    int choice;
    while (true) {
        cout << "1. Add Book" << endl;
        cout << "2. Update Book" << endl;
        cout << "3. Delete Book" << endl;
        cout << "4. Exit" << endl;
        cin >> choice;
        switch (choice) {
            case 1:
                addBook(storage);
            break;
            case 2:
                updateBook(storage);
            break;
            case 3:
                deleteBook(storage);
            break;
            case 4:
            return;
            default:
                cout
            << " Invalid Choice!";
        }
    }
}


int main() {

    enable_foreign_keys();
    auto storage = setup_database();

    int choice;
    while (true) {
        cout << "\nVirtual Library: ";
        cout << " \n1. Enter as borrower: ";
        cout << " \n2. Enter as employee: ";
        cout << " \n3. Exit: " <<endl;
        cin >> choice;
        switch (choice) {
            case 1:
            break;
            case 2:
                listBooks(storage);
                bookActions_switch(storage);
            break;
            case 3:
                cout << " \nGoodbye!";
            return 0;
            default:
                cout
            << " Invalid Choice!";
        }
    }
}