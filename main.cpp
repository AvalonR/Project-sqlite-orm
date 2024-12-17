#include <iostream>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <cstdlib>
#include <iomanip>
#include <chrono>
#include <optional>
#include <string>

using namespace std;
using namespace sqlite_orm;

//global variables
int chosenBookID = 0;
auto originalCinBuf = std::cin.rdbuf();

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
    string name, email;
};
struct BorrowRecord
{
    int id, book_id, borrower_id;
    string borrow_date;
    std::optional<std::string> return_date; //nullable return date
};

//sqlite-database set up
auto setup_database(bool is_test = false) {
    string db_name = is_test ? ":memory:" : "library.db"; //Use in-memory DB for testing
    auto storage = make_storage(
        db_name,
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
    storage.sync_schema();
    cout << (is_test ? "Test" : "Production") << " database initialized successfully!" << endl;
    return storage;
}
void enable_foreign_keys()
{
    //Enable foreign keys using raw SQLite API
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
    system("cls"); // For Windows
#elif __linux__ || __APPLE__ || __MACH__
    system("clear");  // For Linux and macOS
#elif __unix__ || __posix__
    system("clear");  // For other Unix-like systems
#else
    std::cout << "\033[2J\033[1;1H";  // ANSI escape sequence for most terminals
#endif
}
void pause() {
    std::cin.rdbuf(originalCinBuf);
    std::cout << "\nPress Enter to continue..." << endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Clear any leftover input in the buffer
    std::cin.get();  //Wait for Enter key
}

//displays
void displayHeader(const string& title)
{
    //Allows the title of the header to always be centered by using padding
    const int headerWidth = 35;
    const char borderChar = '=';
    int leftPadding = (headerWidth - title.length()) / 2;
    int rightPadding = headerWidth - leftPadding - title.length();
    cout << '\n' << string(headerWidth, borderChar) << endl;
    cout << string(leftPadding, ' ') << title << string(rightPadding, ' ') << endl;
    cout << string(headerWidth, borderChar);
}
void display_main_menu()
{
    clear_screen();
    displayHeader("VIRTUAL LIBRARY");
    cout << "\n[1] Enter as Librarian";
    cout << "\n[2] Enter as Patron";
    cout << "\n[3] Exit";
    cout << "\n>> ";
}
void display_employee_menu() {
    clear_screen();
    displayHeader("LIBRARIAN MENU");
    cout << "\n[1] Manage Authors";
    cout << "\n[2] Manage Books";
    cout << "\n[3] Manage Patrons";
    cout << "\n[4] Return";
    cout << "\n>> ";
}
void display_borrower_management_menu()
{
    clear_screen();
    displayHeader("PATRON MANAGEMENT");
    cout << "\n[1] List Patrons";
    cout << "\n[2] Add Patron";
    cout << "\n[3] Delete Patron";
    cout << "\n[4] See Borrowing History";
    cout << "\n[5] Return";
    cout << "\n>> ";
}
void display_main_borrower_menu()
{
    clear_screen();
    displayHeader("PATRON MENU");
    cout << "\n[1] Register";
    cout << "\n[2] Enter as Patron";
    cout << "\n[3] Return to Main Menu";
    cout << "\n>> ";
}
void display_borrower_menu()
{
    clear_screen();
    displayHeader("PATRON MENU");
    cout << "\n[1] Borrow a Book";
    cout << "\n[2] Return a Book";
    cout << "\n[3] View Your Borrowing History";
    cout << "\n[4] Delete Your Data";
    cout << "\n[5] Return";
    cout << "\n>> ";
}

//switches
void main_menu_Switch(auto& storage, int id_choice) {
    int choice;
    while (true) {
        display_main_menu();
        cin >> choice;
        switch (choice) {
            case 1:
                Employee_switch(storage, id_choice);
            break;
            case 2:
                enterBorrower_switch(storage);
            break;
            case 3:
                cout << "\nGoodbye!";
            exit(0);
            default:
                cout << "\nInvalid Choice, Try Again" << endl;
                pause();
        }
    }
}
void Employee_switch(auto& storage, int id_choice)
{
    int choice;
    while (true)
    {
        display_employee_menu();
        cin >> choice;
        switch (choice)
        {
        case 1:
            listAuthors(storage);
            break;
        case 2:
            listBooks(storage, id_choice);
            break;
        case 3:
            borrowerManagement_switch(storage, id_choice);
            break;
        case 4:
            main_menu_Switch(storage, id_choice);
        default:
            cout << "\nInvalid Choice, Try Again" << endl;
            pause();
        }
    }
}
void bookActions_switch(auto& storage, int id_choice)
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
            pause();
            clear_screen();
            listBooks(storage, id_choice);
            break;
        case 2:
            deleteBook(storage);
            pause();
            clear_screen();
            listBooks(storage, id_choice);
            break;
        case 3:
            clear_screen();
            listBooks(storage, id_choice);
        default:
            cout
                << "\nInvalid Choice, try again" << endl;
        }
    }
}
void borrowerManagement_switch(auto& storage, int id_choice) {
    int choice;
    while (true) {
        display_borrower_management_menu();
        cin >> choice;
        switch (choice) {
            case 1:
                clear_screen();
                listBorrowers(storage);
                pause();
            break;
            case 2:
                clear_screen();
                addBorrower(storage);
                pause();
            break;
            case 3:
                clear_screen();
                listBorrowers(storage);
                deleteBorrower(storage);
                pause();
            break;
            case 4:
                clear_screen();
                choose_Borrower(storage, id_choice);
                clear_screen();
                showbookrecordforuser(storage, id_choice);
                pause();
                borrowerManagement_switch(storage, id_choice);
            case 5:
                return;
            default:
                cout << "\nInvalid Choice, Try Again" << endl;
                pause();
        }
    }

}
void Borrower_switch(auto& storage, int id_choice)
{
    int choice;
    while (true)
    {
        display_borrower_menu();
        cin >> choice;
        switch (choice)
        {
        case 1:
            listavailablebooks(storage, id_choice);
            break;
        case 2:
            listborrowedbooks(storage, id_choice);
            break;
        case 3:
            clear_screen();
            showbookrecordforuser(storage, id_choice);
            pause();
            break;
            case 4:
                deleteBorrower(storage);
                pause();
                clear_screen();
            case 5:
            return;
        default:
            cout << "\nInvalid Choice, try again" << endl;
            pause();
            break;
        }
    }
}
void enterBorrower_switch(auto& storage)
{
    int choice;
    while (true)
    {
        display_main_borrower_menu();
        cin >> choice;
        switch (choice)
        {
        case 1:
            addBorrower(storage);
            pause();
            clear_screen();
            break;
        case 2:
            clear_screen();
            enter_as_Borrower(storage);
            break;
        case 3:
            return;
        default:
            cout << "\nInvalid Choice, try again" << endl;
            pause();
        }
    }
}

//Actions with authors
void listAuthors(auto& storage)
{
    clear_screen();
    const int authors_per_page = 5;
    int current_page = 1;

    while (true)
    {
        auto authors = storage.template get_all<Author>();
        int total_authors = authors.size();
        int total_pages = (total_authors + authors_per_page - 1) / authors_per_page;

        if (total_authors == 0)
        {
            cout << "\nNo Authors Found in the Library" << endl;
            addAuthor(storage);
            pause();
            return;
        }

        string header = "AUTHOR LIST (PAGE " + to_string(current_page) + "/" + to_string(total_pages) + ")";
        displayHeader(header);
        cout << "\nID\t| Name\n";

        int start_index = (current_page - 1) * authors_per_page;
        int end_index = min(start_index + authors_per_page, total_authors);

        for (int i = start_index; i < end_index; ++i)
        {
            const auto& author = authors[i];
            cout << author.id << "\t| " << author.name << "\n";
        }
        cout << "===================================";
        cout << "\n[P] Previous Page | [N] Next Page "
            "\n[1] Delete Author"
            "\n[2] Add Author"
            "\n[3] See Authors Works"
            "\n[4] Return";
        cout << "\n>> ";

        char choice;
        cin >> choice;

        if (tolower(choice) == 'n' && current_page < total_pages)
        {
            current_page++;
            clear_screen();
        }
        else if (tolower(choice) == 'p' && current_page > 1)
        {
            current_page--;
            clear_screen();
        }
        else if (tolower(choice) == '1' && current_page > 0)
        {
            deleteAuthor(storage);
            pause();
            clear_screen();
        }
        else if (tolower(choice) == '2' && current_page > 0)
        {
            addAuthor(storage);
            pause();
            clear_screen();
        }
        else if (tolower(choice) == '3' && current_page > 0)
        {
            listAuthor_their_books(storage);
            pause();
            clear_screen();
        }
        else if (tolower(choice) == '4')
        {
            break;
        }
        else
        {
            cout << "\nInvalid choice. Please try again.\n";
            pause();
            clear_screen();
        }
    }
}
void listAuthor_their_books(auto& storage)
{
    int s_author_id;
    cout << "Enter the Author ID" << "\n>> ";
    cin >> s_author_id;
    clear_screen();
    try
    {
        //Check if the author exists
        auto author_exists = storage.template count<Author>(where(c(&Author::id) == s_author_id)) > 0;

        if (!author_exists)
        {
            cout << "Error: Author with ID " << s_author_id << " Not Found" << endl;
            return;
        }
        // Fetch all books by the author
        auto books = storage.template get_all<Book>(where(c(&Book::author_id) == s_author_id));
        if (books.empty())
        {
            cout << "\nNo Books Found for This Author" << endl;
        }
        else
        {
            displayHeader("BOOKS");
            cout << endl;
            cout << "Book ID" << " | " << "Borrowed" << " | " << "  Genre  " << " | " << "    Title" << endl;
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
        cout << "Error: " << e.what();
    }
}
void addAuthor(auto& storage)
{
    Author author;
    author.id = 1 + storage.template count<Author>();
    cout << "Enter new Author Name" << "\n>> ";
    cin.ignore();
    getline(cin, author.name);
    storage.template insert(author);
    cout << author.name << " Added Succesfully!" << endl;
}
void deleteAuthor(auto& storage) {
    int choice_for_deletion;
    cout << "Enter Author ID" << "\n>> ";
    cin >> choice_for_deletion;

    if (storage.template count<Author>(where(c(&Author::id) == choice_for_deletion)) == 0) {
        cout << "The Author with ID (" << choice_for_deletion << ") Does not Exist\n";
        return;
    }

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
void listBooks(auto& storage, int id_choice)
{
    clear_screen();
    const int books_per_page = 5;
    int current_page = 1;

    while (true)
    {
        auto books = storage.template get_all<Book>();
        int total_books = books.size();
        int total_pages = (total_books + books_per_page - 1) / books_per_page;

        if (total_books == 0)
        {
            cout << "\nNo Books Found in the Library" << endl;
            addBook(storage);
            return;
        }

        string header = "BOOKS (PAGE " + to_string(current_page) + "/" + to_string(total_pages) + ")";
        displayHeader(header);
        cout << "\nID\t| Title\n";

        int start_index = (current_page - 1) * books_per_page;
        int end_index = min(start_index + books_per_page, total_books);

        for (int i = start_index; i < end_index; ++i)
        {
            const auto& book = books[i];
            cout << book.id << "\t| " << book.title << "\n";
        }
        cout << "===================================";
        cout << "\n[P] Previous Page | [N] Next Page"
            "\n[1] Pick Book By ID"
            "\n[2] Add Book"
            "\n[3] Return";
        cout << "\n>> ";

        char choice;
        cin >> choice;

        if (tolower(choice) == 'n' && current_page < total_pages)
        {
            current_page++;
            clear_screen();
        }
        else if (tolower(choice) == 'p' && current_page > 1)
        {
            current_page--;
            clear_screen();
        }
        else if (tolower(choice) == '1' && current_page > 0)
        {
            listspecificBook(storage);
            bookActions_switch(storage, id_choice);
        }
        else if (tolower(choice) == '2' && current_page > 0)
        {
            addBook(storage);
            pause();
            clear_screen();
        }
        else if (tolower(choice) == '3')
        {
            Employee_switch(storage, id_choice);
        }
        else
        {
            cout << "\nInvalid choice, Try Again.\n";
            pause();
            clear_screen();
        }
    }
}
void listspecificBook(auto& storage)
{
    cout << "Enter the Book ID to View Details (and Delete/Update)" << "\n>> ";
    cin >> chosenBookID;
    clear_screen();
    if (auto book = storage.template get_pointer<Book>(chosenBookID))
    {
        displayHeader("BOOK DETAILS");
        cout << "\nBook ID   | " << book->id;
        cout << "\nAuthor ID | " << book->author_id;
        cout << "\nTitle     | " << book->title;
        cout << "\nGenre     | " << book->genre;
        cout << "\nStatus    | " << (book->is_borrowed ? "Borrowed" : "Not Borrowed") << endl;
    }
    else
    {
        cout << "\nBook not Found!" << endl;
    }
}
void addBook(auto& storage)
{
    Book book;
    book.id = 1 + storage.template count<Book>();
    cout << "Enter the Author ID >> ";
    cin >> book.author_id;
    // Check if the author exists
    if (!storage.template count<Author>(where(c(&Author::id) == book.author_id)))
    {
        cout << "\nError: Author with ID " << book.author_id << " Does not Exist. Please add the Author First" << endl;
        addAuthor(storage);
    }
    else
    {
        cout << "\nEnter the Book Title >> ";
        cin.ignore();
        getline(cin, book.title);
        cout << "\nEnter the Book Genre >> ";
        getline(cin, book.genre);
        book.is_borrowed = false;
        storage.insert(book); // Saving the book to the database
        cout << "\nBook added successfully!" << endl;
    }
}
void updateBook(auto& storage)
{

    if (auto book = storage.template get_pointer<Book>(chosenBookID))
    {
        cout << "\nEnter new Title (Current: " << book->title << ") >> ";
        cin.ignore();
        getline(cin, book->title);

        cout << "\nEnter new Genre (Current: " << book->genre << ") >> ";
        getline(cin, book->genre);

        cout << "\nEnter new Author ID (Current: " << book->author_id << ") >> ";
        cin >> book->author_id;

        storage.update(*book);
        cout << "\nBook Updated Successfully!" << endl;
    }
    else
    {
        cout << "\nBook not Found!" << endl;
    }
}
void deleteBook(auto& storage)
{
    try
    {
        //First check if the book exists
        storage.template get<Book>(chosenBookID);

        //If no exception, proceed to delete
        storage.template remove<Book>(chosenBookID);

        cout << "\nBook deleted successfully!" << endl;
    }
    catch (const std::system_error& e)
    {
        // Handle the case where the book does not exist
        if (std::string(e.what()).find(" Not Found") != std::string::npos)
        {
            cout << "\nBook not Found!" << endl;
        }
        else
        {
            throw; //Re-throw unexpected exceptions
        }
    }
}

//Actions with borrowers
void addBorrower(auto& storage)
{
    Borrower borrower;
    borrower.id = 1 + storage.template count<Borrower>();
    cout << "\nEnter Name >> ";
    cin.ignore();
    getline(cin, borrower.name);
    cout << "Enter Email >> ";
    getline(cin, borrower.email);
    if (borrower.email.find("@") != std::string::npos)
    {
        storage.template insert<Borrower>(borrower);
    }
    else
    {
        cout << "\nInput a Correct Email Address" << endl;
        addBorrower(storage);
    }
    cout << "\n" << borrower.name << " Added Successfully!" << endl;
}
void listBorrowers(auto& storage) {
    if (storage.template count<Borrower>() == 0)
    {
        cout << "No Borrowers Recorded" << endl;
        addBorrower(storage);
    }
    else
    {
        displayHeader("LIST OF BORROWERS");

        int idWidth = 5;      // Set width for ID
        int nameWidth = 20;   // Set width for Name
        int emailWidth = 30;  // Set width for Email

        cout << "\nID    |" << " Name                 |" << " Email"<< endl;

        for (const auto& borrower : storage.template get_all<Borrower>())
        {
            cout << std::setw(idWidth) << std::left << borrower.id << " | "
                 << std::setw(nameWidth) << std::left << borrower.name << " | "
                 << std::setw(emailWidth) << std::left << borrower.email << "\n";
        }
    }
}
void deleteBorrower(auto& storage)
{
    int choice_for_deletion;
    cout << "Enter the ID >> ";
    cin >> choice_for_deletion;

    if (storage.template count<Borrower>(where(c(&Borrower::id) == choice_for_deletion)) == 0)
    {
        cout << "\nThe Patron with ID(" << choice_for_deletion << ") Does not Exist" << endl;
        return;
    }

    storage.template remove<Borrower>(choice_for_deletion);
    if (!storage.template count<Borrower>(where(c(&Borrower::id) == choice_for_deletion)))
    {
        cout << "\nThe Patron with ID(" << choice_for_deletion << ") was Deleted Successful" << endl;
    }
    else
    {
        cout << "\nDeletion was Unsuccessful" << endl;
    }
}
void enter_as_Borrower(auto& storage)
{
    int id_choice;
    listBorrowers(storage);
    cout << "Choose ID \n>> ";
    cin >> id_choice;
    if (storage.template count<Borrower>(where(c(&Borrower::id) == id_choice)))
    {
        Borrower_switch(storage, id_choice);
    }
    else
    {
        int choice;
        cout << "Invalid ID" << endl;
        cout << endl << "Choose From List Given Before [1] or Create new Borrower [2]?" << endl;
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
            cout << "Invalid Choice, Try Again" << endl;
        }
    }
}
void choose_Borrower(auto& storage, int& borrower_id_choice) {
    int id_choice;
    listBorrowers(storage);
    cout << "Choose ID \n>> ";
    cin >> id_choice;
    borrower_id_choice = id_choice;
}

//borrower actions
void borrowBook(auto& storage, int borrower_id_choice)
{
    int chosenBookID;
    cout << "\nInput ID of the Book you Want to Borrow";
    cout << "\n>> ";
    cin >> chosenBookID;

    // Check if the book exists
    auto book_ptr = storage.template get_pointer<Book>(chosenBookID);
    if (!book_ptr)
    {
        cout << "\nInvalid Book ID. Please try again";
        return;
    }

    //pointer to access the book object
    auto& book = *book_ptr;

    // Check if the book is already borrowed
    if (book.is_borrowed)
    {
        cout << "\nThe book is already borrowed.\n";
        return;
    }

    // Record the borrow action
    auto now = std::chrono::system_clock::now();
    auto now_in_time_t = std::chrono::system_clock::to_time_t(now); // Convert to time_t
    // Convert to tm struct to extract date components
    std::tm* local_time = std::localtime(&now_in_time_t);

    // Format the time as a date-only string (YYYY-MM-DD)
    char borrow_date[11]; // Size for "YYYY-MM-DD" format
    std::strftime(borrow_date, sizeof(borrow_date), "%Y-%m-%d", local_time);

    //Create new borrow record
    BorrowRecord newRecord;
    newRecord.book_id = chosenBookID;
    newRecord.borrow_date = borrow_date; // Store the formatted date as a string
    newRecord.return_date = std::nullopt; //Set return date to nullopt (indicating not returned)
    newRecord.id = 1 + storage.template count<BorrowRecord>();
    newRecord.borrower_id = borrower_id_choice;
    storage.insert(newRecord);

    //Update the borrowed status
    book.is_borrowed = true;
    storage.template update<Book>(book);

    // Get borrower info
    auto borrower = storage.template get_pointer<Borrower>(borrower_id_choice);
    if (borrower)
    {
        cout << "The book '" << book.title << "' was successfully borrowed by "
            << borrower->name << " on " << borrow_date << endl;
    }
}
void listavailablebooks(auto& storage, int borrower_id_choice) {
    clear_screen();
    const int books_per_page = 5;
    int current_page = 1;

    while (true) {
        auto books = storage.template get_all<Book>();
        int total_books = books.size();
        int total_pages = (total_books + books_per_page - 1) / books_per_page;

        if (total_books == 0) {
            cout << "\nNo Books in the Library" << endl;
            return;
        }

        string header = "AVAILABLE BOOKS (PAGE " + to_string(current_page) + "/" + to_string(total_pages) + ")";
        displayHeader(header);
        cout << "\nID\t| Title\n";

        int start_index = (current_page - 1) * books_per_page;
        int end_index = min(start_index + books_per_page, total_books);

        // Display available books for the current page
        bool any_books_displayed = false;
        for (int i = start_index; i < end_index; ++i) {
            const auto& book = books[i];
            if (!book.is_borrowed) {  // Only display available books
                cout << book.id << "\t| " << book.title << "\n";
                any_books_displayed = true;
            }
        }

        if (!any_books_displayed) {
            cout << "\nNo Available Books" << endl;
            pause();
            return;
        }

        cout << "\n[P] Previous Page | [N] Next Page"
             << "\n[1] Borrow Book"
             << "\n[2] Return";
        cout << "\n>> ";

        char choice;
        cin >> choice;

        if (tolower(choice) == 'n' && current_page < total_pages) {
            current_page++;
            clear_screen();
        }
        else if (tolower(choice) == 'p' && current_page > 1) {
            current_page--;
            clear_screen();
        }
        else if (tolower(choice) == '1') {
            borrowBook(storage, borrower_id_choice);
            pause();
            clear_screen();
        }
        else if (tolower(choice) == '2') {
            return;
        }
        else {
            cout << "\nInvalid choice, try again.\n";
            pause();
            clear_screen();
        }
    }
}
void listborrowedbooks(auto& storage, int borrower_id_choice)
{
    clear_screen();
    const int books_per_page = 5;
    int current_page = 1;

    while (true) {
        auto borrowRecords = storage.template get_all<BorrowRecord>();
        int total_books = 0;

        for (const auto& record : borrowRecords) {
            if (record.borrower_id == borrower_id_choice && !record.return_date.has_value()) {
                total_books++;
            }
        }

        int total_pages = (total_books + books_per_page - 1) / books_per_page;

        if (total_books == 0) {
            cout << "\nNo Borrowed Books" << endl;
            pause();
            return;
        }

        string header = "BORROWED BOOKS (PAGE " + to_string(current_page) + "/" + to_string(total_pages) + ")";
        displayHeader(header);
        cout << "\nID\t| Title\n";

        bool any_books_displayed = false;
        int start_index = (current_page - 1) * books_per_page;
        int end_index = min(start_index + books_per_page, total_books);

        int displayed_books = 0;

        //for (int i = start_index; i < end_index; ++i) {
        for (const auto& record : borrowRecords) {
            if (record.borrower_id == borrower_id_choice && !record.return_date.has_value()) {
                if (displayed_books >= start_index && displayed_books < end_index) {
                    auto book_ptr = storage.template get_pointer<Book>(record.book_id);
                    if (book_ptr) {
                        cout << book_ptr->id << "\t| " << book_ptr->title << "\n";
                        any_books_displayed = true;
                    }
                }
                displayed_books++;
            }
        }
        //}

        if (!any_books_displayed) {
            cout << "\nNo Borrowed Books" << endl;
            pause();
            return;
        }

        cout << "\n[P] Previous Page | [N] Next Page"
             << "\n[1] Return Book"
             << "\n[2] Return";
        cout << "\n>> ";

        char choice;
        cin >> choice;

        if (tolower(choice) == 'n' && current_page < total_pages) {
            current_page++;
            clear_screen();
        }
        else if (tolower(choice) == 'p' && current_page > 1) {
            current_page--;
            clear_screen();
        }
        else if (tolower(choice) == '1') {
            returnBook(storage, borrower_id_choice);
            pause();
            clear_screen();
        }
        else if (tolower(choice) == '2') {
            return;
        }
        else {
            cout << "\nInvalid choice, try again.\n";
            pause();
            clear_screen();
        }
    }
}
void returnBook(auto& storage, int borrower_id_choice)
{
    int chosenBookID;
    cout << "\nInput the ID of Book you Want to Return";
    cout << "\n>> ";
    cin >> chosenBookID;

    auto book_ptr = storage.template get_pointer<Book>(chosenBookID);
    if (!book_ptr)
    {
        cout << "\nInvalid Book ID. Please try again" << endl;
        return;
    }

    //pointer to access the book object
    auto& book = *book_ptr;

    //Find the associated borrow record with return_date is null
    auto borrowRecords = storage.template get_all<BorrowRecord>();
    BorrowRecord* borrowRecord = nullptr;

    //find the matching one
    for (auto& record : borrowRecords)
    {
        if (record.book_id == chosenBookID && !record.return_date.has_value())
        {
            borrowRecord = &record;
            break;
        }
    }

    //If no borrow record is found
    if (!borrowRecord)
    {
        cout << "\nNo Active Borrow Record Found for This Book" << endl;
        return;
    }

    // Record the return action
    auto now = std::chrono::system_clock::now();
    auto now_in_time_t = std::chrono::system_clock::to_time_t(now); //Convert to time_t
    std::tm* local_time = std::localtime(&now_in_time_t); //Convert to tm struct to extract date components

    //Format the time as a date-only string (YYYY-MM-DD)
    std::stringstream return_date_stream;
    return_date_stream << std::put_time(local_time, "%Y-%m-%d"); //Format as YYYY-MM-DD

    std::string return_date = return_date_stream.str(); //Store the formatted date as a string

    //Update the borrow record with the return date
    borrowRecord->return_date = return_date;

    //Update the borrow record in the storage
    storage.update(*borrowRecord);

    //update book's status
    book.is_borrowed = false;
    storage.template update<Book>(book);

    cout << "\nBook returned successfully on " << return_date << endl;
}
void showbookrecordforuser(auto& storage, int borrower_id_choice)
{
    displayHeader("BORROWING HISTORY");
    auto borrowRecords = storage.template get_all<BorrowRecord>(
        where(c(&BorrowRecord::borrower_id) == borrower_id_choice));

    if (borrowRecords.empty())
    {
        cout << "\nNo Records Found with ID " << borrower_id_choice << "\n";
        return;
    }
    for (const auto& record : borrowRecords)
    {
        cout << "\nRecord ID: " << record.id
            << " | Book ID: " << record.book_id
            << " | Borrow Date: " << record.borrow_date
            << " | Return Date: "
            << (record.return_date ? *record.return_date : "Not Returned")
            << endl;
    }
}

//Functionallity testing
void testAuthors(auto& storage)
{
    bool check1 = false, check2 = false;
    //checking author addition
    try
    {
        string inputa = " J.K. Rowling";
        istringstream inputMocka(inputa);
        cin.rdbuf(inputMocka.rdbuf());
        addAuthor(storage);
        int number_for_id = storage.template count<Author>();
        if (storage.template count<Author>(where(c(&Author::id) == number_for_id)))
        {
            if (storage.template count<Author>(where(c(&Author::name) == "J.K. Rowling")))
            {
                check1 = true;
            }
        }
        //maybe add output checker but questionable
        //checking author deletion
        string inputd = to_string(number_for_id);
        istringstream inputMockd(inputd);
        cin.rdbuf(inputMockd.rdbuf());
        deleteAuthor(storage);
        if (!storage.template count<Author>(where(c(&Author::id) == number_for_id)))
        {
            check2 = true;
        }
    }
    catch (std::system_error& e)
    {
        cout << "ERROR: " << e.code() << " " << e.what() << endl;
    }
    //displaying results
    cout << "\n===================================" << endl;
    if (check1)
    {
        cout << "       Author addition works";
    }
    else
    {
        cout << "    Author addition doesn't work";
    }
    cout << "\n===================================" << endl;
    if (check2)
    {
        cout << "       Author deletion works";
    }
    else
    {
        cout << "    Author deletion doesn't work";
    }
    cout << "\n===================================" << endl;
    pause();
}
void testBooks(auto& storage)
{
    bool check1 = false, check2 = false, check3 = false;
    Author a1;
    a1.id = 1 + storage.template count<Author>();

    a1.name = "Ok";
    storage.template insert<Author>(a1);
    //checking book addition
    try
    {
        string inputs = to_string(a1.id) + "\nFrieren\nAdventure\n3";
        istringstream inputMock(inputs);
        cin.rdbuf(inputMock.rdbuf());
        addBook(storage);
        int number_for_id = storage.template count<Book>();
        if (storage.template count<Book>(where(c(&Book::author_id) == a1.id)) && storage.template count<
            Book>(where(c(&Book::title) == "Frieren")) && storage.template count<Book>(
            where(c(&Book::genre) == "Adventure")))
        {
            check1 = true;
        }
        //checking book editing
        string inputse = " Fiend\nhorror\n1\nE";
        istringstream inputMocke(inputse);
        cin.rdbuf(inputMocke.rdbuf());
        chosenBookID = number_for_id;
        updateBook(storage);
        if (storage.template count<Book>(where(c(&Book::id) == number_for_id)) && storage.template count<
            Book>(where(c(&Book::title) == "Fiend")) && storage.template count<
            Book>(where(c(&Book::genre) == "horror")))
        {
            check2 = true;
        }
        //checking book deleting
        chosenBookID = number_for_id;
        deleteBook(storage);
        if (!storage.template count<Book>(where(c(&Book::id) == number_for_id)))
        {
            check3 = true;
        }
        storage.template remove<Author>(number_for_id);
    }
    catch (std::system_error& e)
    {
        cout << "ERROR: " << e.code() << " " << e.what() << endl;
    }
    chosenBookID = 0;
    //displaying results
    cout << "\n===================================" << endl;
    if (check1)
    {
        cout << "       Book addition works";
    }
    else
    {
        cout << "    Book addition doesn't work";
    }
    cout << "\n===================================" << endl;
    if (check2)
    {
        cout << "       Book editing works";
    }
    else
    {
        cout << "    Book editing doesn't work";
    }
    cout << "\n===================================" << endl;
    if (check3)
    {
        cout << "       Book deletion works";
    }
    else
    {
        cout << "    Book deletion doesn't work\n";
    }
    cout << "\n===================================" << endl;
    pause();
}
void testBorrower(auto& storage)
{
    bool check1 = false, check2 = false;
    int number_for_id;
    number_for_id = 1 + storage.template count<Borrower>();

    //checking borrower addition
    try
    {
        string inputs = " roman\nroman@gmail.com";
        istringstream inputMock(inputs);
        cin.rdbuf(inputMock.rdbuf());
        addBorrower(storage);
        if (storage.template count<Borrower>(where(c(&Borrower::name) == "roman")) && storage.template count<Borrower>(
            where(c(&Borrower::email) == "roman@gmail.com")))
        {
            check1 = true;
        }
        //checking borrower deletion
        string inputd = to_string(number_for_id);
        istringstream inputMockd(inputd);
        cin.rdbuf(inputMockd.rdbuf());
        deleteBorrower(storage);
        if (!(storage.template count<Borrower>(where(c(&Borrower::id) == number_for_id)) && storage.template count<
            Borrower>(where(c(&Borrower::name) == "roman")) && storage.template count<Borrower>(
            where(c(&Borrower::email) == "roman@gmail.com"))))
        {
            check2 = true;
        }
    }
    catch (std::system_error& e)
    {
        cout << "ERROR: " << e.code() << " " << e.what() << endl;
    }
    //displaying results
    cout << "\n===================================" << endl;
    if (check1)
    {
        cout << "     Borrower addition works";
    }
    else
    {
        cout << "  Borrower addition doesn't work";
    }
    cout << "\n===================================" << endl;
    if (check2)
    {
        cout << "     Borrower deletion works";
    }
    else
    {
        cout << "  Borrower deletion doesn't work";
    }
    cout << "\n===================================" << endl;
    pause();
}
void testBorrowRecord(auto& storage)
{
    bool check1 = false, check2 = false;
    //inserting data for proper checking
    Author a1;
    a1.id = 1 + storage.template count<Author>();
    a1.name = "Ok1";
    storage.template insert<Author>(a1);
    Book b1;
    b1.id = 1 + storage.template count<Book>();
    b1.author_id = a1.id;
    b1.genre = "horror";
    b1.title = "Octopus";
    storage.template insert<Book>(b1);
    Borrower borrower;
    borrower.id = 1 + storage.template count<Borrower>();
    borrower.email = "roman@gmail.com";
    borrower.name = "roman";
    storage.template insert<Borrower>(borrower);
    //checking book borrowing
    try
    {
        string inputs = to_string(b1.id);
        istringstream inputMock(inputs);
        cin.rdbuf(inputMock.rdbuf());
        borrowBook(storage, borrower.id);
        if (storage.template count<BorrowRecord>(where(c(&BorrowRecord::book_id) == b1.id)))
        {
            if (auto book_check1 = storage.template get<Book>(b1.id); book_check1.is_borrowed)
            {
                check1 = true;
            }
        }

        //checking book returning
        string input = to_string(b1.id);
        istringstream inputMockk(input);
        cin.rdbuf(inputMockk.rdbuf());
        returnBook(storage, borrower.id);

        if (auto borrow_check = storage.template get_all<BorrowRecord>(where(c(&BorrowRecord::book_id) == b1.id));
            !borrow_check.empty() && borrow_check.front().return_date.has_value())
        {
            if (auto book_check2 = storage.template get<Book>(b1.id); !book_check2.is_borrowed)
            {
                check2 = true;
            }
        }
        //clearing data
        storage.template remove<BorrowRecord>(borrower.id);
        storage.template remove<Borrower>(borrower.id);
        storage.template remove<Book>(b1.id);
        storage.template remove<Author>(a1.id);
    }
    catch (std::system_error& e)
    {
        cout << "ERROR: " << e.code() << " " << e.what() << endl;
    }

    //displaying results
    cout << "\n===================================" << endl;
    if (check1)
    {
        cout << "       Book borrowing works";
    }
    else
    {
        cout << "    Book borrowing doesn't work";
    }
    cout << "\n===================================" << endl;
    if (check2)
    {
        cout << "       Book returning works";
    }
    else
    {
        cout << "    Book returning doesn't work";
    }
    cout << "\n===================================" << endl;
    pause();
}

int main(int id_choice) {
    bool is_test_mode;
    cout << "Pick Mode (0 for Production, 1 for Test) \n>> ";
    cin >> is_test_mode;
    auto storage = setup_database(is_test_mode);
    if (is_test_mode)
    {
        enable_foreign_keys();
        testAuthors(storage);
        testBooks(storage);
        testBorrower(storage);
        testBorrowRecord(storage);
    }
    else {
        enable_foreign_keys();
        main_menu_Switch(storage, id_choice);
    }
    return 0;
}