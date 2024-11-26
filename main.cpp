#include <iostream>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

using namespace std;
using namespace sqlite_orm;

struct Book
{
    int id, author_id;
    string title, genre;
    bool is_borrowed{};
};
struct Author
{
    int id;
    string name;
};
struct Borrower
{
    int id;
    string name;
    string email;
};
struct BorrowRecord
{
    int id{}, book_id{}, borrower_id{};
    string borrow_date, return_date;
};

auto setup_database()
{
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
            .on_delete.cascade()
            //enables CASCADE delete, when a book is deleted, the borrow record for that book will be deleted as well
            .on_update.cascade(),
            //enables CASCADE update, when book ID is updated, the book ID in borrow records will be updated as well
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
void enable_foreign_keys()
{
    // Enable foreign keys using raw SQLite API
    sqlite3* db = nullptr;
    sqlite3_open("library.db", &db);

    if (db)
    {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK)
        {
            cerr << "Error enabling foreign keys: " << (errMsg ? errMsg : "unknown error") << endl;
            sqlite3_free(errMsg);
        }
        else
        {
            cout << "Foreign key constraints enabled." << endl;
        }

        // Check if foreign keys are enabled
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "PRAGMA foreign_keys;", -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            bool foreign_keys_enabled = sqlite3_column_int(stmt, 0) == 1;
            cout << "Foreign keys enabled: " << (foreign_keys_enabled ? "Yes" : "No") << endl;
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else
    {
        cerr << "Failed to open database." << endl;
    }
}

void addAuthor(auto& storage)
{
    Author author;
    author.id = 1 + storage.template count<Author>();
    cout << "Enter new Author Name:" << endl;
    cin.ignore();
    getline(cin, author.name);
    storage.template insert(author);
    listAuthors(storage);
}
void listAuthors_n_Books(auto& storage)
{
    cout << "List of All Authors and their Books:" << endl;
    for (const auto& author : storage.template get_all<Author>())
    {
        cout << "Author: " << author.id << " | " << author.name << endl;
        // Query books written by this author

        auto books = storage.template get_all<Book>(sqlite_orm::where(sqlite_orm::c(&Book::author_id) == author.id));

        // Check if the author has books
        if (books.empty())
        {
            cout << "  No books found for this author." << endl;
        }
        else
        {
            cout << "Books: " << endl;
            for (const auto& book : books)
            {
                cout << "Title: " << book.title << "; Book ID: " << book.id << "; Book Author Id: " << book.author_id <<
                    "; Genre: " << book.genre
                    << "; Borrowed: " << (book.is_borrowed ? "Yes" : "No") << endl;
            }
        }
        cout << endl; // Blank line after each author
    }
}
void listAuthors(auto& storage)
{
    if (storage.template count<Author>() == 0)
    {
        cout << "No authors found." << endl;
        addAuthor(storage);
    }
    else
    {
        cout << "List of All Authors" << endl;
        for (const auto& author : storage.template get_all<Author>())
        {
            cout << "Author: " << author.id << " | " << author.name << endl;
        }
    }
}
void deleteAuthor(auto& storage)
{
    int choice_for_deletion;
    listAuthors(storage);
    cout << "Choose the Author for deletion:" << endl;
    cin >> choice_for_deletion;
    storage.template remove<Author>(choice_for_deletion);
    if (!storage.template count<Author>(where(c(&Author::id) == choice_for_deletion)))
    {
        cout << "The Author with ID(" << choice_for_deletion << ") was deleted successfully" << endl;
    }
    else
    {
        cout << "Deletion was unsuccessful" << endl;
    }
}

void addBook(auto& storage)
{
    Book book;
    book.id = 1 + storage.template count<Book>();
    listAuthors(storage);
    cout << "Enter the Author ID: " << endl;
    cin >> book.author_id;
    // Check if the author exists
    if (!storage.template count<Author>(where(c(&Author::id) == book.author_id)))
    {
        cout << "Error: Author with ID " << book.author_id << " does not exist. Please add the author first.\n";
        addAuthor(storage);
    }
    else
    {
        cout << "Enter the Book Title: " << endl;
        cin.ignore();
        getline(cin, book.title);
        cout << "Enter the Book Genre: " << endl;
        getline(cin, book.genre);
        book.is_borrowed = false;
        storage.insert(book); // Saving the book to the database
        cout << "Book added successfully!" << endl;
        listBooks(storage);
    }
}
void listBooks(auto& storage)
{
    cout << "\nList of Books:\n";
    cout << "--------------------------------\n";
    cout << "ID\t| Title\n";
    cout << "--------------------------------\n";

    for (const auto& book : storage.template get_all<Book>())
    {
        cout << book.id << "\t| " << book.title << "\n";
    }

    cout << "--------------------------------\n";
}
//int listspecificBook(auto& storage){} // this function has to show the specific information about the book !!!!!!!
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
} //we do not have to make the user enter the ID again, this has to be edited
void deleteBook(auto& storage)
{
    int id;
    cout << "Enter the ID of the book to delete: ";
    cin >> id;

    try
    {
        // First check if the book exists
        storage.template get<Book>(id);

        // If no exception, proceed to delete
        storage.template remove<Book>(id);

        cout << "Book deleted successfully!" << endl;
    }
    catch (const std::system_error& e)
    {
        // Handle the case where the book does not exist
        if (std::string(e.what()).find("not found") != std::string::npos)
        {
            cout << "The book was not found." << endl;
        }
        else
        {
            throw; // Re-throw unexpected exceptions
        }
    }
} //same here, no need to call for the ID

void Employee_switch(auto&storage) {
    int choice;
    while (true) {
        cout << "\n1. List all books: ";
        cout << "\n2. List all authors: ";
        cout << "\n3. Back: " <<endl;
        cin >> choice;
        switch (choice) {
            case 1:
                bookEmployee_switch(storage);
            break;
            case 2:
                authorEmployee_switch(storage);
                break;
            case 3:
                return;
            default:
                cout << "Invalid Choice!";
        }
    }
}

void bookEmployee_switch(auto&storage) {
    listBooks(storage);
    int choice;
    while (true) {
        cout << "\n1. Pick book by ID: ";
        cout << "\n2. Add book: ";
        cout << "\n3. Back: " << endl;
        cin >> choice;

        switch (choice) {
            case 1:
                //listspecificBooks(storage);
                bookActions_switch(storage);
                break;
            case 2:
                addBook(storage);
                break;
            case 3:
                return;
            default:
                cout << "Invalid Choice!";
        }
    }
}
void bookActions_switch(auto& storage)
{

    int choice;
    while (true)
    {
        cout << "1. Update Book" << endl;
        cout << "2. Delete Book" << endl;
        cout << "3. Back" << endl;
        cin >> choice;
        switch (choice)
        {
            case 1:
                updateBook(storage);
            break;
            case 2:
                deleteBook(storage);
            break;
            case 3:
                return;
            default:
                cout
                    << " Invalid Choice!";
        }
    }
}

void authorEmployee_switch(auto&storage) {
    listAuthors(storage);
    int choice;
    while (true) {
        cout << "\n1. See books by Author: ";
        cout << "\n2. Add Author: ";
        cout << "\n3. Delete Author: ";
        cout << "\n4. Back: ";
        cin >> choice;
        switch (choice) {
            case 1:
                break;
            case 2:
                addAuthor(storage);
            break;
            case 3:
                deleteAuthor(storage);
            break;
            case 4:
                return;
            default:
                cout << "Invalid Choice!";
        }
    }
}

int main()
{
    enable_foreign_keys();
    auto storage = setup_database();

    int choice;
    while (true) {
        cout << "\nVirtual Library: ";
        cout << " \n1. Enter as Employee: ";
        cout << " \n2. Enter as Borrower: ";
        cout << " \n3. Exit: " << endl;
        cin >> choice;
        switch (choice) {
            case 1:
                Employee_switch(storage);
            break;
            case 2:
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