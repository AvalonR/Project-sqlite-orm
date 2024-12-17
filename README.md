# Library Management System  

A C++ library management system built using SQLite-ORM.  

## Features:  

### 1. Database Management:  
  • **Dynamic Schema Creation:** Automatically sets up tables and relationships without manual SQL queries.  
  • **Entity Relationships:** Implements foreign key constraints for relationships between Author, Book, Borrower, and BorrowRecord tables.  
  • **Cascade Delete:** Deletes dependent records (e.g., all books by an author) when a parent entity is removed.  
  • **Update Restrictions:** Prevents updates to keys in certain cases, maintaining data consistency.  
  • **Dual Database Modes:**  
    - **Production Mode:** Uses a persistent `library.db` file for data storage.  
    - **Test Mode:** Employs an in-memory database (`:memory:`) for isolated testing of the operations present in the system.  

    **Example Code:**
    
    ```cpp
    auto setup_database(bool is_test = false) {
    string db_name = is_test ? ":memory:" : "library.db"; //Use in-memory DB for testing
    auto storage = make_storage(
        db_name,
        make_table(
            "Author",
            make_column("id", &Author::id, primary_key()),
            make_column("name", &Author::name)
        ),
    ```

### 2. Library Operations:  
  • **Author Management:**  
    - Add, remove, or list authors in the system.  
    - Link authors to their books through a relational database structure.  

    **Example Code:**

    ```cpp
    Author author;
    author.id = 1 + storage.template count<Author>();
    cout << "Enter new Author Name" << "\n>> ";
    cin.ignore();
    getline(cin, author.name);
    storage.template insert(author);
    cout << author.name << " Added Succesfully!" << endl;
    ```

  • **Book Management:**  
    - Add new books to the system and associate them with authors.  
    - Track borrowing status (borrowed or available).  
    - Display book details, including genre, title, and borrow status.  

    **Example Code:**
    
    ```cpp
    
    //Listing all books
    auto books = storage.get_all<Book>();
    for (const auto& book : books) {
        cout << "Book ID: " << book.id << ", Title: " << book.title 
             << ", Borrowed: " << (book.is_borrowed ? "Yes" : "No") << endl;
    }
    ```

  • **Borrower Management:**  
    - Add and manage borrowers with essential details such as name and email.  
    - Track borrowing records, including the date borrowed and the return date.  

    **Example Code:**

    ```cpp

    // Listing all borrowers
    auto borrowers = storage.get_all<Borrower>();
    for (const auto& borrower : borrowers) {
        cout << "Borrower ID: " << borrower.id << ", Name: " << borrower.name << endl;
    }
    ```

  • **Book Borrowing and Return:**  
    - Allows borrowers to check out books and mark them as borrowed.  
    - Update return dates when books are returned.  

    **Example Code:**

    ```cpp
    // Borrowing a Book
    int borrower_id = 1;  // Assume Borrower ID 1 exists
    int book_id = 1;  // Assume Book ID 1 exists

    BorrowRecord new_record;
    new_record.book_id = book_id;
    new_record.borrower_id = borrower_id;
    new_record.borrow_date = "2024-12-17";  // Example date format
    new_record.return_date = "2024-12-24"; // Example return date
    storage.insert(new_record);

    // Marking a book as borrowed
    Book& book = storage.get<Book>(book_id);
    book.is_borrowed = true;
    storage.update(book);
    ```

  • **Detailed Book Listings:**  
    - List all books associated with a specific author.  
    - Show the borrowing status of books, providing a clear view of which books are available or borrowed.  

    **Example Code:**

    ```cpp
    int author_id = 1;  // Assume Author ID 1 exists
    auto books = storage.get_all<Book>(where(c(&Book::author_id) == author_id));
    for (const auto& book : books) {
        cout << "Book ID: " << book.id << ", Title: " << book.title 
             << ", Borrowed: " << (book.is_borrowed ? "Yes" : "No") << endl;
    }
    ```

### 3. User-Friendly Console Interface:  
  • **Clear Menu Navigation:**  
    ![Screenshot 1](https://github.com/user-attachments/assets/a16206bf-ddb6-4b7f-9adb-6e313e9b5f2a)   
  • **Interactive Prompts:**  
    - Prompts are provided for every user input, ensuring clarity and ease of use when entering data.  
    - User input is validated with clear messages, reducing the chance of errors or confusion.  
  • **Structured Output:**  
    - Information is presented in a clean, organized format, with tables and headers clearly delineating key data.  
    - The output is dynamically generated based on user actions, ensuring only relevant information is displayed.  
  • **Error Handling and Feedback:**  
    - Friendly error messages guide users when something goes wrong.  
  • **Efficient Data Display:**  
    - Listings are formatted neatly with aligned columns and pagination, making it easy to read and understand large datasets.  
    ![Screenshot 3](https://github.com/user-attachments/assets/11f2bb24-a8d2-4b9c-a3e2-8f51ed281ece)  
