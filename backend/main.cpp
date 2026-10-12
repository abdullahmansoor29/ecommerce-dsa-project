#include "third_party/httplib.h"
#include "dsa/LinkedList.h"
#include "dsa/BST.h"
#include "dsa/Stack.h"
#include "dsa/Queue.h"
#include "dsa/Sorting.h"
#include "db/Database.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <map>
#include <functional>
#include <sstream>
#include<cstdlib>

using json = nlohmann::json;

// Instead of storing the password directly, we "hash" it — turning
// it into a string that's hard to reverse back to the original
// password. NOTE: this is not industry-grade (like bcrypt/argon2),
// but it's much better than plaintext and needs no extra library.
std::string hashPassword(const std::string& password)
{
    std::string salted = password + "dsa_project_salt_2026"; // the "salt" makes guessing harder
    std::size_t h = std::hash<std::string>{}(salted);
    std::ostringstream oss;
    oss << h;
    return oss.str();
}

int main()
{
    httplib::Server server;
    
     // Allow the frontend (hosted on a different domain) to call this API.
    // Without this, browsers block cross-origin requests by default.
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    Database db("../database/shop.db");
    db.setupTables();

    // Load products from the database into the BST — this is our
    // "runtime" structure used for search/sort/traversal
    ProductBST products;
    for (auto& row : db.getAllProducts())
        products.insert(row.id, row.name, row.price, row.stock);

    // Each user has their own cart and their own undo-stack,
    // identified by username. std::map here is just a "container
    // of users" — the actual DSA (CartLinkedList, UndoStack) inside
    // are manually implemented.
    std::map<std::string, CartLinkedList> carts;
    std::map<std::string, UndoStack> undoStacks;

    // On restart, pending orders are reloaded back into the queue
    OrderQueue pendingOrders;
    for (auto& row : db.getPendingOrders())
        pendingOrders.enqueue(row.id, row.username, row.total);

    // Helper: checks whether a username belongs to an admin —
    // every admin-only endpoint uses this
    auto isAdmin = [&db](const std::string& username) {
        return db.getRole(username) == "admin";
    };

    // ==================== AUTH ====================

    server.Get("/signup", [&db](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username") || !req.has_param("password"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username and password"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");
        std::string password = req.get_param_value("password");
        std::string role = req.has_param("role") ? req.get_param_value("role") : "user";

        if (db.userExists(username))
        {
            res.status = 409;
            res.set_content(json{{"error", "username already taken"}}.dump(), "application/json");
            return;
        }

        db.createUser(username, hashPassword(password), role);
        res.set_content(json{{"status", "signed_up"}, {"role", role}}.dump(), "application/json");
    });

    server.Get("/login", [&db](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username") || !req.has_param("password"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username and password"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");
        std::string password = req.get_param_value("password");
        std::string role = db.checkLogin(username, hashPassword(password));

        if (role.empty())
        {
            res.status = 401;
            res.set_content(json{{"error", "invalid username or password"}}.dump(), "application/json");
            return;
        }

        res.set_content(json{{"status", "logged_in"}, {"role", role}}.dump(), "application/json");
    });

    // ==================== PRODUCTS ====================

    // BST's inorder traversal — always returns an alphabetically sorted result
    server.Get("/products", [&products](const httplib::Request&, httplib::Response& res)
    {
        res.set_content(products.toJson().dump(), "application/json");
    });

    server.Get("/products/search", [&products](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("name"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need name"}}.dump(), "application/json");
            return;
        }

        ProductNode* found = products.findByName(req.get_param_value("name"));
        if (found == nullptr)
        {
            res.status = 404;
            res.set_content(json{{"error", "not found"}}.dump(), "application/json");
            return;
        }

        json reply;
        reply["id"] = found->id;
        reply["name"] = found->name;
        reply["price"] = found->price;
        reply["stock"] = found->stock;
        res.set_content(reply.dump(), "application/json");
    });

    server.Get("/products/sorted", [&db](const httplib::Request&, httplib::Response& res)
    {
        std::vector<ProductRow> rows = db.getAllProducts();
        mergeSort(rows, 0, (int)rows.size() - 1);

        json result = json::array();
        for (auto& r : rows)
            result.push_back({{"id", r.id}, {"name", r.name}, {"price", r.price}, {"stock", r.stock}});

        res.set_content(result.dump(), "application/json");
    });

    server.Get("/products/price-range", [&db](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("min") || !req.has_param("max"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need min, max"}}.dump(), "application/json");
            return;
        }

        double minPrice = std::stod(req.get_param_value("min"));
        double maxPrice = std::stod(req.get_param_value("max"));

        std::vector<ProductRow> rows = db.getAllProducts();
        mergeSort(rows, 0, (int)rows.size() - 1);

        int startIndex = lowerBound(rows, minPrice);

        json result = json::array();
        for (int i = startIndex; i < (int)rows.size() && rows[i].price <= maxPrice; i++)
            result.push_back({{"id", rows[i].id}, {"name", rows[i].name}, {"price", rows[i].price}, {"stock", rows[i].stock}});

        res.set_content(result.dump(), "application/json");
    });

        server.Get("/products/add", [&db, &products, isAdmin](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("admin_username") || !isAdmin(req.get_param_value("admin_username")))
        {
            res.status = 403;
            res.set_content(json{{"error", "admin access required"}}.dump(), "application/json");
            return;
        }

        if (!req.has_param("name") || !req.has_param("price") || !req.has_param("stock"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need name, price, stock"}}.dump(), "application/json");
            return;
        }

        std::string name = req.get_param_value("name");
        double price = std::stod(req.get_param_value("price"));
        int stock = std::stoi(req.get_param_value("stock"));

        // If a product with this name already exists, treat this as a
        // restock instead of creating a duplicate: add to its stock
        // and update its price. We modify the node directly rather than
        // deleting and reinserting, since the BST key (name) is unchanged
        // — restructuring the tree here would be unnecessary work.
        ProductNode* existing = products.findByName(name);
        if (existing != nullptr)
        {
            existing->stock += stock;
            existing->price = price;
            db.updateProductRow(existing->id, name, existing->price, existing->stock);

            json reply;
            reply["status"] = "restocked";
            reply["id"] = existing->id;
            reply["new_stock"] = existing->stock;
            res.set_content(reply.dump(), "application/json");
            return;
        }

        int newId = db.insertProduct(name, price, stock);
        products.insert(newId, name, price, stock);

        json reply;
        reply["status"] = "added";
        reply["id"] = newId;
        res.set_content(reply.dump(), "application/json");
    });

    // Admin: update a product (delete + reinsert in the BST, so the
    // name-sorted property always stays correct)
    server.Get("/products/update", [&db, &products, isAdmin](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("admin_username") || !isAdmin(req.get_param_value("admin_username")))
        {
            res.status = 403;
            res.set_content(json{{"error", "admin access required"}}.dump(), "application/json");
            return;
        }

        if (!req.has_param("id") || !req.has_param("name") || !req.has_param("price") || !req.has_param("stock"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need id, name, price, stock"}}.dump(), "application/json");
            return;
        }

        int id = std::stoi(req.get_param_value("id"));
        std::string newName = req.get_param_value("name");
        double newPrice = std::stod(req.get_param_value("price"));
        int newStock = std::stoi(req.get_param_value("stock"));

        ProductNode* existing = products.findById(id);
        if (existing == nullptr)
        {
            res.status = 404;
            res.set_content(json{{"error", "product not found"}}.dump(), "application/json");
            return;
        }

        std::string oldName = existing->name;
        products.deleteByName(oldName);
        products.insert(id, newName, newPrice, newStock);

        db.updateProductRow(id, newName, newPrice, newStock);

        res.set_content(json{{"status", "updated"}}.dump(), "application/json");
    });

    // Admin: delete a product (from both the BST and the database)
    server.Get("/products/delete", [&db, &products, isAdmin](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("admin_username") || !isAdmin(req.get_param_value("admin_username")))
        {
            res.status = 403;
            res.set_content(json{{"error", "admin access required"}}.dump(), "application/json");
            return;
        }

        if (!req.has_param("id"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need id"}}.dump(), "application/json");
            return;
        }

        int id = std::stoi(req.get_param_value("id"));
        ProductNode* existing = products.findById(id);
        if (existing == nullptr)
        {
            res.status = 404;
            res.set_content(json{{"error", "product not found"}}.dump(), "application/json");
            return;
        }

        std::string name = existing->name;
        products.deleteByName(name);
        db.deleteProduct(id);

        res.set_content(json{{"status", "deleted"}}.dump(), "application/json");
    });

    // ==================== CART ====================

    server.Get("/cart/add", [&carts, &undoStacks, &products](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username") || !req.has_param("id") || !req.has_param("qty"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username, id, qty"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");
        int id = std::stoi(req.get_param_value("id"));
        int qty = std::stoi(req.get_param_value("qty"));

        ProductNode* product = products.findById(id);
        if (product == nullptr)
        {
            res.status = 404;
            res.set_content(json{{"error", "product not found"}}.dump(), "application/json");
            return;
        }

        if (product->stock < qty)
        {
            res.status = 400;
            res.set_content(json{{"error", "not enough stock"}, {"available", product->stock}}.dump(), "application/json");
            return;
        }

        carts[username].addItem(product->id, product->name, product->price, qty);
        undoStacks[username].push("add", product->id, product->name, product->price, qty);

        json reply;
        reply["status"] = "added";
        reply["cart"] = carts[username].toJson();
        reply["total"] = carts[username].getTotal();
        res.set_content(reply.dump(), "application/json");
    });

    server.Get("/cart/view", [&carts](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");
        json reply;
        reply["cart"] = carts[username].toJson();
        reply["total"] = carts[username].getTotal();
        res.set_content(reply.dump(), "application/json");
    });

    server.Get("/cart/remove", [&carts, &undoStacks](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username") || !req.has_param("id"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username, id"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");
        int id = std::stoi(req.get_param_value("id"));

        CartNode* existing = carts[username].findItem(id);
        if (existing == nullptr)
        {
            json reply;
            reply["status"] = "not_found";
            reply["cart"] = carts[username].toJson();
            reply["total"] = carts[username].getTotal();
            res.set_content(reply.dump(), "application/json");
            return;
        }

        // Remember the full quantity so undo can restore it exactly
        undoStacks[username].push("remove", existing->productId, existing->name, existing->price, existing->quantity);
        carts[username].removeItem(id);

        json reply;
        reply["status"] = "removed";
        reply["cart"] = carts[username].toJson();
        reply["total"] = carts[username].getTotal();
        res.set_content(reply.dump(), "application/json");
    });

    server.Get("/cart/undo", [&carts, &undoStacks](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");

        std::string action;
        int id;
        std::string name;
        double price;
        int qty;

        bool popped = undoStacks[username].pop(action, id, name, price, qty);

        if (!popped)
        {
            res.status = 400;
            res.set_content(json{{"error", "nothing to undo"}}.dump(), "application/json");
            return;
        }

        // Undoing an "add" = reduce quantity. Undoing a "remove" = add it back.
        if (action == "add")
            carts[username].reduceQuantity(id, qty);
        else if (action == "remove")
            carts[username].addItem(id, name, price, qty);

        json reply;
        reply["status"] = "undone";
        reply["undone_action"] = action;
        reply["cart"] = carts[username].toJson();
        reply["total"] = carts[username].getTotal();
        res.set_content(reply.dump(), "application/json");
    });

    // ==================== ORDERS ====================

    server.Get("/orders/place", [&carts, &undoStacks, &db, &pendingOrders, &products](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("username"))
        {
            res.status = 400;
            res.set_content(json{{"error", "need username"}}.dump(), "application/json");
            return;
        }

        std::string username = req.get_param_value("username");

        if (carts[username].getSize() == 0)
        {
            res.status = 400;
            res.set_content(json{{"error", "cart is empty"}}.dump(), "application/json");
            return;
        }

        // First check stock for every item in the cart — so if one
        // item runs short, the whole order fails cleanly rather
        // than partially going through
        CartNode* current = carts[username].getHead();
        while (current != nullptr)
        {
            ProductNode* product = products.findById(current->productId);
            if (product == nullptr || product->stock < current->quantity)
            {
                res.status = 400;
                res.set_content(json{{"error", "not enough stock for " + current->name}}.dump(), "application/json");
                return;
            }
            current = current->next;
        }

        // Now reduce stock (in both the BST runtime copy and the database)
        current = carts[username].getHead();
        while (current != nullptr)
        {
            ProductNode* product = products.findById(current->productId);
            product->stock -= current->quantity;
            db.updateStock(product->id, product->stock);
            current = current->next;
        }

        double total = carts[username].getTotal();
        int orderId = db.insertOrder(username, total);
        pendingOrders.enqueue(orderId, username, total);

        carts[username].clear();
        undoStacks[username].clear();

        json reply;
        reply["status"] = "order_placed";
        reply["order_id"] = orderId;
        reply["total"] = total;
        res.set_content(reply.dump(), "application/json");
    });

    server.Get("/orders/pending", [&pendingOrders, isAdmin](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("admin_username") || !isAdmin(req.get_param_value("admin_username")))
        {
            res.status = 403;
            res.set_content(json{{"error", "admin access required"}}.dump(), "application/json");
            return;
        }
        res.set_content(pendingOrders.toJson().dump(), "application/json");
    });

    server.Get("/orders/process", [&pendingOrders, &db, isAdmin](const httplib::Request& req, httplib::Response& res)
    {
        if (!req.has_param("admin_username") || !isAdmin(req.get_param_value("admin_username")))
        {
            res.status = 403;
            res.set_content(json{{"error", "admin access required"}}.dump(), "application/json");
            return;
        }

        int orderId;
        std::string username;
        double total;

        bool success = pendingOrders.dequeue(orderId, username, total);

        if (!success)
        {
            res.status = 400;
            res.set_content(json{{"error", "no pending orders"}}.dump(), "application/json");
            return;
        }

        db.markOrderCompleted(orderId);

        json reply;
        reply["status"] = "processed";
        reply["order_id"] = orderId;
        reply["username"] = username;
        reply["total"] = total;
        res.set_content(reply.dump(), "application/json");
    });

    server.Get("/health", [](const httplib::Request&, httplib::Response& res)
    {
        res.set_content(json{{"status", "ok"}, {"message", "C++ backend is running"}}.dump(), "application/json");
    });

    std::cout << "Server running on http://localhost:8080" << std::endl;
    server.listen("localhost", 8080);

    return 0;
}