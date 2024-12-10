#include <iostream>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <cstdlib>
#include <iomanip>
#include <chrono>
#include <optional>
#include <string>
#include <ncurses.h>

using namespace std;
using namespace sqlite_orm;

//global variable (for now)
int chosenBookID =0;

//structures
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
struct BorrowRecord {
    int id;
    int book_id;
    int borrower_id;  // id of the borrower
    string borrow_date;  // timestamps  of when the book was borrowed
     std::optional<std::string> return_date;  // Nullable return date
};


//sqlite-database
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
void clear_screen()
{
#ifdef _WIN32
    system("cls");  // For Windows
#elif __linux__ || __APPLE__ || __MACH__
    system("clear");  // For Linux and macOS
#elif __unix__ || __posix__
    system("clear");  // For other Unix-like systems
#else
    std::cout << "\033[2J\033[1;1H";  // ANSI escape sequence for most terminals
#endif
}

//displays
void display_main_menu() {
    cout << "\n===================================";
    cout << "\n      LIBRARY MANAGEMENT SYSTEM    ";
    cout << "\n===================================";
    cout << "\n[1] Enter as Employee";
    cout << "\n[2] Enter as Borrower";
    cout << "\n[3] Exit";
    cout << "\n>> ";
}
void display_employee_menu() {
    cout << "\n===================================";
    cout << "\n          EMPLOYEE MENU            ";
    cout << "\n===================================";
    cout << "\n[1] Manage Authors";
    cout << "\n[2] Manage Books";
    cout << "\n[3] Manage Borrowers";
    cout << "\n[4] Return to Main Menu";
    cout << "\n>> ";
}
void display_author_management_menu() {
    cout << "\n===================================";
    cout << "\n        AUTHOR MANAGEMENT          ";
    cout << "\n===================================";
    cout << "\n[1] List All Authors";
    cout << "\n[2] List Author and Their Books";
    cout << "\n[3] Return to Employee Menu";
    cout << "\n>> ";
}
void display_borrower_management_menu() {
    cout << "\n===================================";
    cout << "\n       BORROWER MANAGEMENT         ";
    cout << "\n===================================";
    cout << "\n[1] List Borrowers";
    cout << "\n[2] Add Borrower";
    cout << "\n[3] Delete Borrower";
    cout << "\n[4] Return to Employee Menu";
    cout << "\n>> ";
}
void display_borrower_menu() {
    cout << "\n===================================";
    cout << "\n          BORROWER MENU            ";
    cout << "\n===================================";
    cout << "\n[1] Borrow a Book";
    cout << "\n[2] Return a Book";
    cout << "\n[3] View Borrowing History";
    cout << "\n[4] Return to Main Menu";
    cout << "\n>> ";
}

//switches
void Employee_switch(auto&storage) {
    int choice;
    while (true) {
        display_employee_menu();
        cin >> choice;
        switch (choice) {
            case 1:
                authorEmployee_switch(storage);
            break;
            case 2:
                listBooks(storage);
            break;
            case 3:
                listBorrowers(storage);
            break;
            case 4:
                display_main_menu();
            default:
                cout << "\nInvalid Choice, try again" << endl;
        }
    }
}
void authorEmployee_switch(auto&storage) {
    int choice;
    while (true) {
        display_author_management_menu();
        cin >> choice;
        switch (choice) {
            case 1:
                listAuthors(storage);
            break;
            case 2:
                listAuthor_their_books(storage);
            int choice2;
            while (true) {
                cout << "\n[1] Go Back to Author Management Menu";
                cout <<"\n>> ";
                cin >> choice2;
                switch (choice2) {
                    case 1:
                        authorEmployee_switch(storage);
                    break;
                    default:
                        cout << "\nInvalid Choice, try again" << endl;
                }
            }
            case 3:
                return;
            default:
                cout << "\nInvalid Choice, try again" << endl;
        }
    }
}
void bookEmployee_switch(auto&storage) {
    listspecificBook(storage);
    bookActions_switch(storage);
}
void bookActions_switch(auto& storage)
{
    int choice;
    while (true)
    {
        cout << "\n[1] Update Book";
        cout << "\n[2] Delete Book";
        cout << "\n[3] Back";
        cout << "\n>> ";

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
                    << "\nInvalid Choice, try again" << endl;
        }
    }
}
void Borrower_switch(auto& storage, int id_choice) {
    int choice;
    while (true) {
        display_borrower_menu();
        cin >> choice;
        switch (choice) {
            case 1:
                borrowBook(storage, id_choice);
            break;
            case 2:
                returnBook(storage, id_choice);
            break;
            case 3:
                showbookrecordforuser(storage, id_choice);
            break;
            case 4:
                return;
            default:
                cout << "\nInvalid Choice, try again" << endl;
            break;
        }
    }
}
void enterBorrower_switch(auto& storage) {
    cout << "\n===================================";
    cout << "\n          BORROWER MENU            ";
    cout << "\n===================================";
    cout << "\n[1] Add Borrower";
    cout << "\n[2] Enter as Existing Borrower";
    cout << "\n[3] Return to Main Menu";
    cout << "\n>> ";
    int choice;
    while (true) {
        cin >> choice;
        switch (choice) {
            case 1:
                addBorrower(storage);
                enter_as_Borrower(storage);
            break;
            case 2:
                enter_as_Borrower(storage);
            break;
            case 3:
                return;
            default:
                cout << "\nInvalid Choice, try again" << endl;
        }
    }
}

//Actions with authors
void listAuthors(auto& storage) {
        const int authors_per_page = 5;
        auto authors = storage.template get_all<Author>();
        int total_authors = authors.size();
        int total_pages = (total_authors + authors_per_page - 1) / authors_per_page;
        int current_page = 1;

        if (total_authors == 0) {
            cout << "\nNo Authors Found in the Library" << endl;
            addAuthor(storage);
            return;
        }

        while (true) {
            cout << "\n===================================";
            cout << "\n       AUTHOR LIST (PAGE " << current_page << "/" << total_pages << ")";
            cout << "\n===================================";
            cout << "\nID\t| Name\n";

            int start_index = (current_page - 1) * authors_per_page;
            int end_index = min(start_index + authors_per_page, total_authors);

            for (int i = start_index; i < end_index; ++i) {
                const auto& author = authors[i];
                cout << author.id << "\t| " << author.name << "\n";
            }
            cout << "===================================";
            cout << "\n[N] Next Page | [P] Previous Page "
                    "\n[1] Delete Author "
                    "\n[2] Add Author "
                    "\n[3] Exit";
            cout << "\n>> ";

            char choice;
            cin >> choice;

            if (tolower(choice) == 'n' && current_page < total_pages) {
                current_page++;
            } else if (tolower(choice) == 'p' && current_page > 1) {
                current_page--;
            } else if (tolower(choice) == '1' && current_page > 0) {
                deleteAuthor(storage);
            } else if (tolower(choice) == '2' && current_page > 0) {
                addAuthor(storage);
            } else if (tolower(choice) == '3') {
                break;
            } else {
                cout << "\nInvalid choice. Please try again.\n";
            }
        }
    }
void listAuthor_their_books(auto& storage)
{
    int s_author_id;
    cout << "\nEnter the Author ID: " << endl;
    cin >> s_author_id;
    try
    {
        // Get the author information
        cout << "ID" << setw(5) << " | " << "  Name" << endl;
        cout << "---------------------------" << endl;
        auto author_info = storage.template get<Author>(s_author_id);
        cout << author_info.id << setw(6) << " | " << setw(6) << author_info.name << endl;

        // Fetch all books by the author
        auto books = storage.template get_all<Book>(where(c(&Book::author_id) == s_author_id));
        if (books.empty())
        {
            cout << "No books found for this author.\n";
        }
        else
        {
            cout << "\nBooks: " << endl;
            cout << "Book ID" << " | " << "Borrowed" << " | " << "  Genre  " << " | "<< "    Title" << endl;
            cout << "-----------------------------------------------------------------------------" << endl;
            for (const auto& book : books)
            {
                cout << book.id << setw(9) << " | " << setw(5) << (book.is_borrowed ? "Yes" : "No") << setw(6)
                << " | " << setw(1) << "" << book.genre << setw(4) <<
                " | " << setw(2) << "" << book.title << endl;
            }
        }
    }
    catch (const std::system_error& e)
    {
        cout << "Error: " << e.what() << ". Author with ID " << s_author_id << " not found.\n";
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

}
void deleteAuthor(auto& storage)
{
    int choice_for_deletion;
    cout << "Enter Author ID:" << endl;
    cin >> choice_for_deletion;
    storage.template remove<Author>(choice_for_deletion);
    if (!storage.template count<Author>(where(c(&Author::id) == choice_for_deletion)))
    {
        cout << "The Author with ID (" << choice_for_deletion << ") was Deleted Successfully" << endl;
    }
    else
    {
        cout << "Deletion was Unsuccessful" << endl;
    }
}

//Actions with books
void listBooks(auto& storage) {
    const int books_per_page = 5;
    ;
    auto books = storage.template get_all<Book>();
    int total_books = books.size();
    int total_pages = (total_books + books_per_page - 1) / books_per_page;
    int current_page = 1;

    if (total_books == 0) {
        cout << "\nNo Books Found in the Library" << endl;
        addBook(storage);
        return;
    }
    while (true) {
        cout << "\n===================================";
        cout << "\n         BOOK LIST (PAGE " << current_page << "/" << total_pages << ")";
        cout << "\n===================================";
        cout << "\nID\t| Title\n";

        int start_index = (current_page - 1) * books_per_page;
        int end_index = min(start_index + books_per_page, total_books);

        for (int i = start_index; i < end_index; ++i) {
            const auto& book = books[i];
            cout << book.id << "\t| " << book.title << "\n";
        }
        cout << "===================================";
        cout << "\n[N] Next Page | [P] Previous Page "
                "\n[1] Pick Book By ID"
                "\n[2] Add Book"
                "\n[3] Exit";
        cout << "\n>> ";

        char choice;
        cin >> choice;

        if (tolower(choice) == 'n' && current_page < total_pages) {
            current_page++;
        } else if (tolower(choice) == 'p' && current_page > 1) {
            current_page--;
        } else if (tolower(choice) == '1' && current_page > 0) {
            bookEmployee_switch(storage);
        }else if (tolower(choice) == '2' && current_page > 0) {
            addBook(storage);
        } else if (tolower(choice) == '3') {
            break;
        } else {
            cout << "\nInvalid choice. Please try again.\n";
        }
    }
}
void listspecificBook(auto& storage) {
    cout << "Enter the Book ID to View Details (and Delete or Update)" << "\n>> ";
    cin >> chosenBookID;

    if (auto book = storage.template get_pointer<Book>(chosenBookID)) {
        cout << "\n===================================";
        cout << "\n           BOOK DETAILS            ";
        cout << "\n===================================";
        cout << "\nID: " << book->id;
        cout << "\nTitle: " << book->title;
        cout << "\nGenre: " << book->genre;
        cout << "\nAuthor ID: " << book->author_id;
        cout << "\nBorrowed: " << (book->is_borrowed ? "Yes" : "No");

        // Check if the author exists for this book
        if (auto author = storage.template get_pointer<Author>(book->author_id)) {
            cout << "\nAuthor Name: " << author->name << "\n";
        } else {
            cout << "\nAuthor: Not found";
        }
    } else {
        cout << "Error: Book not found!\n";
    }
}
void addBook(auto& storage)
{
    Book book;
    book.id = 1 + storage.template count<Book>();
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
void updateBook(auto& storage)
{
    //int id;
    //cout << "Enter the ID of the book to update: ";
    //cin >> id;

    if (auto book = storage.template get_pointer<Book>(chosenBookID))
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
    //int id;
    //cout << "Enter the ID of the book to delete: ";
    //cin >> id;

    try
    {
        // First check if the book exists
        storage.template get<Book>(chosenBookID);

        // If no exception, proceed to delete
        storage.template remove<Book>(chosenBookID);

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
}

//Actions with borrowers
void addBorrower(auto& storage)
{
    Borrower borrower;
    borrower.id = 1 + storage.template count<Borrower>();
    cout << "Enter your name:" << endl;
    cin.ignore();
    getline(cin, borrower.name);
    cout << "Enter your email:" << endl;
    getline(cin, borrower.email);
    if (borrower.email.find("@") != std::string::npos)
    {
        storage.template insert<Borrower>(borrower);
    }
    else
    {
        cout << "Input a correct email address" << endl;
        addBorrower(storage);
    }
}
void listBorrowers(auto& storage)
{
    if (storage.template count<Borrower>() == 0)
    {
        cout << "No Borrowers recorded" << endl;
        addBorrower(storage);
    }
    else
    {
        cout << "\nList of Borrowers:" << endl;
        cout << "---------------------------\n";
        cout << "ID\t| Title\t| Email\n";
        for (const auto& borrower : storage.template get_all<Borrower>())
        {
            cout << borrower.id << "\t| " << borrower.name << " | " << borrower.email << "\n";
        }
    }
}
void deleteBorrower(auto& storage)
{
    listBorrowers(storage);
    int choice_for_deletion;
    cout << "Enter the ID of the borrower to delete: ";
    cin >> choice_for_deletion;
    storage.template remove<Borrower>(choice_for_deletion);
    if (!storage.template count<Borrower>(where(c(&Borrower::id) == choice_for_deletion)))
    {
        cout << "The Borrower with ID(" << choice_for_deletion << ") was deleted successful." << endl;
    }
    else
    {
        cout << "Deletion was unsuccessful" << endl;
    }
}
void enter_as_Borrower(auto& storage)
{
    int id_choice;
    listBorrowers(storage);
    cout << "Choose the ID of the borrower:" << endl;
    cin >> id_choice;
    if (storage.template count<Borrower>(where(c(&Borrower::id) == id_choice)))
    {
        cout << "Valid ID";
        Borrower_switch(storage, id_choice);
    }
    else
    {
        int choice;
        cout << "Invalid ID" << endl;
        cout << endl << "Would you like to choose from the list given before [1] or create a new borrower [2]" << endl;
        cin >> choice;
        if (choice == 1)
        {
            enter_as_Borrower(storage);
        }
        else if (choice == 2)
        {
            addBorrower(storage);
            enter_as_Borrower(storage);
        }
        else
        {
            cout << "Invalid choice" << endl;
        }
    }
}

//borrower actions
void borrowBook(auto& storage, int borrower_id_choice) {
    auto books = storage.template get_all<Book>();
    bool anyAvailable = false;
    for (const auto& book : books) {
        if (!book.is_borrowed) {
            anyAvailable = true;
            break;
        }
    }
    if (!anyAvailable) {
        cout << "\nNo Available Books to Borrow" << endl;
        return;
    }

    listavailablebooks(storage);
    int chosenBookID;
    cout << "\nInput ID of the Book you Want to Borrow";
    cout <<"\n>> ";
    cin >> chosenBookID;

    // Check if the book exists
    auto book_ptr = storage.template get_pointer<Book>(chosenBookID);
    if (!book_ptr) {
        cout << "\nInvalid Book ID. Please try again";
        return;
    }

    //pointer to access the book object
    auto& book = *book_ptr;

    // Record the borrow action
    auto now = std::chrono::system_clock::now();
    auto now_in_time_t = std::chrono::system_clock::to_time_t(now);  // Convert to time_t
    // Convert to tm struct to extract date components
    std::tm* local_time = std::localtime(&now_in_time_t);

    // Format the time as a date-only string (YYYY-MM-DD)
    char borrow_date[11];  // Size for "YYYY-MM-DD" format
    std::strftime(borrow_date, sizeof(borrow_date), "%Y-%m-%d", local_time);

    //Create new borrow record
    BorrowRecord newRecord;
    newRecord.book_id = chosenBookID;
    newRecord.borrow_date = borrow_date;  // Store the formatted date as a string
    newRecord.return_date = std::nullopt;  //Set return date to nullopt (indicating not returned)
    newRecord.id = 1 + storage.template count<BorrowRecord>();
    newRecord.borrower_id = borrower_id_choice;
    storage.insert(newRecord);

    //Update the borrowed status
    book.is_borrowed = true;
    storage.template update<Book>(book);

    // Get borrower info
    auto borrower = storage.template get_pointer<Borrower>(borrower_id_choice);
    if (borrower) {
        cout << "The book '" << book.title << "' was successfully borrowed by "
             << borrower->name << " on " << borrow_date << endl;
    }
}
void listavailablebooks(auto& storage) {

    cout << "\n===================================\n";
    cout << "      AVAILABLE BOOKS             \n";
    cout << "===================================\n";

    auto books = storage.template get_all<Book>();
    for (const auto& book : books) {
        if (book.is_borrowed == false)
        {
            cout << "Book ID: " << book.id << ", Title: " << book.title << "\n";
        }
    }
}
void listborrowedbooks(auto& storage, int borrower_id_choice)
{
    cout << "\n===================================\n";
    cout << "      BORROWED BOOKS             \n";
    cout << "===================================\n";

    // Get all borrow records and filter by borrower_id
    auto borrowRecords = storage.template get_all<BorrowRecord>();
    bool found = false;

    for (const auto& record : borrowRecords) {
        if (record.borrower_id == borrower_id_choice && !record.return_date.has_value()) {
            // Get the book associated with this borrow record
            auto book_ptr = storage.template get_pointer<Book>(record.book_id);
            if (book_ptr) {
                cout << "Book ID: " << book_ptr->id << ", Title: " << book_ptr->title << "\n";
                found = true;
            }
        }
    }

    if (!found) {
        cout << "No borrowed books for this borrower." << endl;
    }
}
void returnBook(auto& storage, int borrower_id_choice)
{
    auto books = storage.template get_all<Book>();
    bool anyBorrowed = false;
    for (const auto& book : books) {
        if (book.is_borrowed) {
            anyBorrowed = true;
            break;
        }
    }
    if (!anyBorrowed) {
        cout << "\nNo Books Borrowed" << endl;
        return;
    }

    listborrowedbooks(storage, borrower_id_choice);
    int chosenBookID;
    cout << "\nInput the ID of Book you Want to Return";
    cout << "\n>> ";
    cin >> chosenBookID;

    auto book_ptr = storage.template get_pointer<Book>(chosenBookID);
    if (!book_ptr) {
        cout << "\nInvalid Book ID. Please try again" << endl;
        return;
    }

    //pointer to access the book object
    auto& book = *book_ptr;

    //Find the associated borrow record with return_date is null
    auto borrowRecords = storage.template get_all<BorrowRecord>();
    BorrowRecord* borrowRecord = nullptr;

    //find the matching one
    for (auto& record : borrowRecords) {
        if (record.book_id == chosenBookID && !record.return_date.has_value()) {
            borrowRecord = &record;
            break;
        }
    }

    //If no borrow record is found
    if (!borrowRecord) {
        cout << "\nNo Active Borrow Record Found for This Book" << endl;
        return;
    }

    // Record the return action
    auto now = std::chrono::system_clock::now();
    auto now_in_time_t = std::chrono::system_clock::to_time_t(now);  //Convert to time_t
    std::tm* local_time = std::localtime(&now_in_time_t);  //Convert to tm struct to extract date components

    //Format the time as a date-only string (YYYY-MM-DD)
    std::stringstream return_date_stream;
    return_date_stream << std::put_time(local_time, "%Y-%m-%d");  //Format as YYYY-MM-DD

    std::string return_date = return_date_stream.str();  //Store the formatted date as a string

    //Update the borrow record with the return date
    borrowRecord->return_date = return_date;

    //Update the borrow record in the storage
    storage.update(*borrowRecord);

    //update book's status
    book.is_borrowed = false;
    storage.template update<Book>(book);

    cout << "\nBook returned successfully on " << return_date << endl;
}
void showbookrecordforuser(auto& storage, int borrower_id_choice) {
    cout << "\n===================================\n";
    cout << "     BOOK RECORD FOR A USER        \n";
    cout << "===================================\n";

    auto borrowRecords = storage.template get_all<BorrowRecord>(
        where(c(&BorrowRecord::borrower_id) == borrower_id_choice));

    if (borrowRecords.empty()) {
        cout << "No records found for the borrower with ID " << borrower_id_choice << ".\n";
        return;
    }

    cout << "Borrow Records for Borrower ID " << borrower_id_choice << ":\n";
    for (const auto& record : borrowRecords) {
        cout << "Record ID: " << record.id
             << ", Book ID: " << record.book_id
             << ", Borrow Date: " << record.borrow_date
             << ", Return Date: "
             << (record.return_date ? *record.return_date : "Not returned yet")
             << "\n";
    }
}

int main()
    {
        enable_foreign_keys();
        auto storage = setup_database();

        int choice;
        while (true) {
            display_main_menu();
            cin >> choice;
            switch (choice) {
                case 1:
                    Employee_switch(storage);
                break;
                case 2:
                    enterBorrower_switch(storage);
                break;
                case 3:
                    cout << "\nGoodbye!";
                return 0;
                default:
                    cout
                << "\nInvalid Choice, Enter Value From 1-3" << endl;
            }
        }
    }
