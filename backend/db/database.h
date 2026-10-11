#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>

// ============================================================
// WHY IS THE DATABASE SEPARATE FROM THE DSA?
// The DSA (Linked List, Stack, Queue, BST) only organize data AT
// RUNTIME — they live in RAM and disappear the moment the server
// stops. The database (SQLite) is PERSISTENT storage — it survives
// server restarts and reboots, because it's saved to a file on
// disk. That's why users, products, and orders live in the
// database, and the DSA are used only as a "runtime working copy"
// to process/organize that data.
// ============================================================

struct ProductRow
{
    int id;
    std::string name;
    double price;
    int stock;
};

struct OrderRow
{
    int id;
    std::string username;
    double total;
};

class Database
{
private:
    sqlite3* db;   // SQLite library's database "handle" (like a connection)

public:
    // Constructor: opens the database file (creates it if it doesn't exist)
    Database(const std::string& path)
    {
        if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
            std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
    }

    ~Database() { sqlite3_close(db); }

    // Creates tables if they don't already exist. "IF NOT EXISTS"
    // makes this safe to call every time the server starts.
    void setupTables()
    {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS users ("
            "user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT UNIQUE NOT NULL,"
            "password TEXT NOT NULL,"
            "role TEXT NOT NULL DEFAULT 'user');"
            "CREATE TABLE IF NOT EXISTS products ("
            "product_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "price REAL NOT NULL,"
            "stock INTEGER NOT NULL DEFAULT 0);"
            "CREATE TABLE IF NOT EXISTS orders ("
            "order_id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT NOT NULL,"
            "total REAL NOT NULL,"
            "status TEXT NOT NULL DEFAULT 'pending');";

        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK)
        {
            std::cerr << "Setup error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }

    bool userExists(const std::string& username)
    {
        const char* sql = "SELECT 1 FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        bool exists = sqlite3_step(stmt) == SQLITE_ROW;
        sqlite3_finalize(stmt);
        return exists;
    }

    std::string getRole(const std::string& username)
    {
        const char* sql = "SELECT role FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

        std::string role = "";
        if (sqlite3_step(stmt) == SQLITE_ROW)
            role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        sqlite3_finalize(stmt);
        return role;
    }

    bool createUser(const std::string& username, const std::string& hashedPassword, const std::string& role)
    {
        const char* sql = "INSERT INTO users (username, password, role) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
        bool success = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return success;
    }

    std::string checkLogin(const std::string& username, const std::string& hashedPassword)
    {
        const char* sql = "SELECT role FROM users WHERE username = ? AND password = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hashedPassword.c_str(), -1, SQLITE_TRANSIENT);

        std::string role = "";
        if (sqlite3_step(stmt) == SQLITE_ROW)
            role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        sqlite3_finalize(stmt);
        return role;
    }

    // The "?" placeholders have values "bound" to them — this
    // prevents SQL injection (never build SQL by concatenating strings)
    int insertProduct(const std::string& name, double price, int stock)
    {
        const char* sql = "INSERT INTO products (name, price, stock) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, price);
        sqlite3_bind_int(stmt, 3, stock);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (int)sqlite3_last_insert_rowid(db);
    }

    // Updates all fields of a product (used by the admin edit feature)
    void updateProductRow(int productId, const std::string& name, double price, int stock)
    {
        const char* sql = "UPDATE products SET name = ?, price = ?, stock = ? WHERE product_id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, price);
        sqlite3_bind_int(stmt, 3, stock);
        sqlite3_bind_int(stmt, 4, productId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void updateStock(int productId, int newStock)
    {
        const char* sql = "UPDATE products SET stock = ? WHERE product_id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, newStock);
        sqlite3_bind_int(stmt, 2, productId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // Deletes a product (used by the admin delete feature)
    void deleteProduct(int productId)
    {
        const char* sql = "DELETE FROM products WHERE product_id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, productId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    std::vector<ProductRow> getAllProducts()
    {
        std::vector<ProductRow> rows;
        const char* sql = "SELECT product_id, name, price, stock FROM products;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            ProductRow row;
            row.id = sqlite3_column_int(stmt, 0);
            row.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            row.price = sqlite3_column_double(stmt, 2);
            row.stock = sqlite3_column_int(stmt, 3);
            rows.push_back(row);
        }
        sqlite3_finalize(stmt);
        return rows;
    }

    int insertOrder(const std::string& username, double total)
    {
        const char* sql = "INSERT INTO orders (username, total, status) VALUES (?, ?, 'pending');";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 2, total);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (int)sqlite3_last_insert_rowid(db);
    }

    void markOrderCompleted(int orderId)
    {
        const char* sql = "UPDATE orders SET status = 'completed' WHERE order_id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, orderId);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // On server restart, pending orders are reloaded into the queue
    // in the same order (order_id ascending), so FIFO order is preserved
    std::vector<OrderRow> getPendingOrders()
    {
        std::vector<OrderRow> rows;
        const char* sql = "SELECT order_id, username, total FROM orders WHERE status = 'pending' ORDER BY order_id ASC;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            OrderRow row;
            row.id = sqlite3_column_int(stmt, 0);
            row.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            row.total = sqlite3_column_double(stmt, 2);
            rows.push_back(row);
        }
        sqlite3_finalize(stmt);
        return rows;
    }
};

#endif