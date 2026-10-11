#ifndef BST_H
#define BST_H

#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================
// WHY BST FOR PRODUCTS?
// Products need to be searched frequently and listed
// alphabetically. In a BST, every node's left subtree holds
// "smaller" values and its right subtree holds "larger" values —
// this makes search O(log n) on average (better than Linear
// Search's O(n)), and an "inorder traversal" gives a sorted list
// automatically, with no separate sorting step needed.
//
// AN IMPORTANT TRADE-OFF (worth mentioning in viva): the tree is
// organized by PRODUCT NAME, not Product ID. Reason: if we used ID
// (which auto-increments and always grows) as the key, new
// products would always be inserted on the far right, and the
// tree would degenerate into something like a linked list — worst
// case O(n) search, losing the benefit of balance. Names arrive in
// varied order, so the tree stays more balanced. But this means
// searching by ID (needed for cart/orders) is NOT O(log n) — it's
// O(n), since the whole tree must be scanned. This is a conscious
// design trade-off.
// ============================================================

struct ProductNode
{
    int id;
    std::string name;
    double price;
    int stock;
    ProductNode* left;
    ProductNode* right;

    ProductNode(int i, std::string n, double p, int s)
        : id(i), name(n), price(p), stock(s), left(nullptr), right(nullptr) {}
};

class ProductBST
{
private:
    ProductNode* root;

    // Converts a name to lowercase so "Laptop" and "laptop" are
    // treated as the same key (case-insensitive comparison)
    std::string toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    // INSERT: recursively places the new node in the correct spot
    // based on name. Average case O(log n), worst case O(n) (if
    // names keep arriving in sorted order).
    ProductNode* insertNode(ProductNode* node, int id, std::string name, double price, int stock)
    {
        if (node == nullptr)
            return new ProductNode(id, name, price, stock);

        std::string key = toLower(name);
        std::string nodeKey = toLower(node->name);

        if (key < nodeKey)
            node->left = insertNode(node->left, id, name, price, stock);
        else if (key > nodeKey)
            node->right = insertNode(node->right, id, name, price, stock);
        // if the name already exists, do nothing (avoid duplicates)

        return node;
    }

    // INORDER TRAVERSAL: left → self → right. In a BST this always
    // produces a sorted (alphabetical) result, with no extra
    // sorting step. Time: O(n), since every node is visited once.
    void inorderCollect(ProductNode* node, json& result)
    {
        if (node == nullptr) return;
        inorderCollect(node->left, result);
        json item;
        item["id"] = node->id;
        item["name"] = node->name;
        item["price"] = node->price;
        item["stock"] = node->stock;
        result.push_back(item);
        inorderCollect(node->right, result);
    }

    // SEARCH BY NAME: this is the real payoff of using a BST. At
    // each step, half the tree is discarded. Average O(log n).
    ProductNode* searchByNameHelper(ProductNode* node, const std::string& key)
    {
        if (node == nullptr) return nullptr;
        std::string nodeKey = toLower(node->name);
        if (key == nodeKey) return node;
        if (key < nodeKey) return searchByNameHelper(node->left, key);
        return searchByNameHelper(node->right, key);
    }

    // SEARCH BY ID: the tree is organized by name, not ID — so the
    // whole tree must be scanned here. O(n). This is the trade-off
    // mentioned above.
    ProductNode* searchByIdHelper(ProductNode* node, int id)
    {
        if (node == nullptr) return nullptr;
        if (node->id == id) return node;
        ProductNode* left = searchByIdHelper(node->left, id);
        if (left != nullptr) return left;
        return searchByIdHelper(node->right, id);
    }

    // DELETE: removing a node from a BST has 3 cases:
    // 1) The node has no children (a leaf) — delete it directly
    // 2) The node has exactly one child — move that child up
    // 3) The node has two children — find the smallest node in the
    //    right subtree (the inorder successor), copy its values
    //    into this node, then delete that successor from the right
    //    subtree (recursively)
    ProductNode* deleteNode(ProductNode* node, const std::string& key)
    {
        if (node == nullptr) return nullptr;

        std::string nodeKey = toLower(node->name);

        if (key < nodeKey)
            node->left = deleteNode(node->left, key);
        else if (key > nodeKey)
            node->right = deleteNode(node->right, key);
        else
        {
            // This is the node to delete
            if (node->left == nullptr)
            {
                ProductNode* temp = node->right;
                delete node;
                return temp;
            }
            else if (node->right == nullptr)
            {
                ProductNode* temp = node->left;
                delete node;
                return temp;
            }

            // Two children: find the successor (leftmost node of the right subtree)
            ProductNode* successor = node->right;
            while (successor->left != nullptr)
                successor = successor->left;

            node->id = successor->id;
            node->name = successor->name;
            node->price = successor->price;
            node->stock = successor->stock;

            node->right = deleteNode(node->right, toLower(successor->name));
        }
        return node;
    }

    // Destroy the whole tree (used by the destructor) — postorder
    // (children first, then self), so no dangling pointers remain
    void destroy(ProductNode* node)
    {
        if (node == nullptr) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    ProductBST() : root(nullptr) {}
    ~ProductBST() { destroy(root); }

    void insert(int id, std::string name, double price, int stock)
    {
        root = insertNode(root, id, name, price, stock);
    }

    ProductNode* findByName(std::string name)
    {
        return searchByNameHelper(root, toLower(name));
    }

    ProductNode* findById(int id)
    {
        return searchByIdHelper(root, id);
    }

    void deleteByName(std::string name)
    {
        root = deleteNode(root, toLower(name));
    }

    json toJson()
    {
        json result = json::array();
        inorderCollect(root, result);
        return result;
    }
};

#endif