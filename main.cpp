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
        make_table(
            "Author",
            make_column("id", &Author::id, primary_key()),
            make_column("name", &Author::name)
        ),
        make_table(
            "Book",
            make_column("id", &Book::id, primary_key()),
            make_column("author_id", &Book::author_id),
            make_column("title", &Book::title),
            make_column("genre", &Book::genre),
            make_column("is_borrowed", &Book::is_borrowed),
            foreign_key(&Book::author_id)
            .references(&Author::id)
            .on_delete.cascade() //Enables casacde delete
        ),
        make_table(
        "Borrower",
        make_column("id", &Borrower::id, primary_key()),
        make_column("name", &Borrower::name),
        make_column("email", &Borrower::email)
        ),
        make_table(
        "BorrowRecord",
        make_column("id", &BorrowRecord::id, primary_key()),
        make_column("book_id", &BorrowRecord::book_id),
        make_column("borrower_id", &BorrowRecord::borrower_id),
        make_column("borrow_date", &BorrowRecord::borrow_date),
        make_column("return_date", &BorrowRecord::return_date),
        foreign_key(&BorrowRecord::book_id)
        .references(&Book::id)
        .on_delete.cascade(), //enables cascade delete
        foreign_key(&BorrowRecord::borrower_id)
        .references(&Borrower::id)
        .on_delete.cascade() //enabbles cascade delete
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
void AddBook(auto& storage)
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
    book.is_borrowed = false;
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

    if (auto book = storage.template get_pointer<Book>(id))
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
}
void deleteBook(auto& storage)
{
    int id;
    cout << "Enter the ID of the book to delete: ";
    cin >> id;

    if (storage.template remove<Book>(id))
    {
        cout << "Book deleted successfully!\n";
    }
    else
    {
        cout << "Book not found!\n";
    }
}

int main() {

    enable_foreign_keys();
    auto storage = setup_database();

    // Insert sample data
    auto author_id = storage.insert(Author{0, "George Orwell"});
    storage.insert(Book{0, author_id, "1984", "Dystopian", false});
    storage.insert(Book{0, author_id, "Animal Farm", "Satire", false});

    std::cout << "Before deleting author:\n";
    for (const auto& book : storage.get_all<Book>())
    {
        std::cout << "Book: " << book.title << "\n";
    }
    for (const auto& author : storage.get_all<Author>())
    {
        std::cout << "Author: " << author.name << " | " << author.id << "\n";
    }
    cout << "------------------------------\n";
    storage.remove<Author>(1);
    cout << "------------------------------\n";
    auto remaining_books = storage.get_all<Book>();
    if (remaining_books.empty())
    {
        std::cout << "All books by the deleted author were also deleted (ON DELETE CASCADE).\n";
    }
    else
    {
        for (const auto& book : remaining_books)
        {
            std::cout << "Book: " << book.title << "\n";
        }
    }
    return 0;
}