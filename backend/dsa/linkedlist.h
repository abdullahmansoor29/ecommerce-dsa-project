#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================
// WHY LINKED LIST FOR THE CART?
// The cart items get added/removed frequently, and we don't know
// in advance how many items it will have. In an array/vector,
// removing an item from the middle requires shifting all the
// following elements. In a Linked List, we just change one
// pointer — no shifting needed. This is a theoretically good fit
// for this use case.
//
// WEAKNESS (worth mentioning in viva): search is O(n) (no index
// access), and nodes are scattered in memory (cache-unfriendly).
// Since carts are small, this doesn't matter in practice. Real
// e-commerce apps store carts in a database/Redis, not an
// in-memory linked list — we're using one here to demonstrate DSA.
// ============================================================

// One node = one product entry in the cart
struct CartNode
{
    int productId;
    std::string name;
    double price;
    int quantity;
    CartNode* next;   // address of the next node; nullptr means "end of list"

    CartNode(int id, std::string n, double p, int q)
        : productId(id), name(n), price(p), quantity(q), next(nullptr) {}
};

class CartLinkedList
{
private:
    CartNode* head;   // first item in the list
    CartNode* tail;   // last item in the list — lets us add in O(1)
    int size;

public:
    CartLinkedList() : head(nullptr), tail(nullptr), size(0) {}

    // Destructor: free all nodes when the object is destroyed,
    // otherwise heap memory is never returned (memory leak)
    ~CartLinkedList() { clear(); }

    // Lets outside code iterate over the list (e.g. checking stock
    // for every item when placing an order)
    CartNode* getHead() { return head; }

    // LINEAR SEARCH: check every node from start to end.
    // Time: O(n) — a Linked List has no way to "jump" to an index,
    // so we must walk through it one node at a time.
    CartNode* findItem(int productId)
    {
        CartNode* current = head;
        while (current != nullptr)
        {
            if (current->productId == productId)
                return current;
            current = current->next;
        }
        return nullptr;
    }

    // Add an item: if it already exists, merge quantities;
    // otherwise append a new node at the tail.
    // Time: O(n) because of the duplicate check (findItem), but the
    // actual insertion itself is O(1) thanks to the tail pointer.
    void addItem(int productId, std::string name, double price, int quantity)
    {
        CartNode* existing = findItem(productId);
        if (existing != nullptr)
        {
            existing->quantity += quantity;
            return;
        }

        CartNode* newNode = new CartNode(productId, name, price, quantity);

        if (head == nullptr) // list is currently empty
        {
            head = newNode;
            tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
        size++;
    }

    // Remove: we need a "previous" pointer because when we remove a
    // node, the node BEFORE it needs its "next" updated.
    // Time: O(n)
    bool removeItem(int productId)
    {
        CartNode* current = head;
        CartNode* previous = nullptr;

        while (current != nullptr)
        {
            if (current->productId == productId)
            {
                if (previous == nullptr)
                    head = current->next;           // removing the first node
                else
                    previous->next = current->next; // removing a middle/last node

                if (current == tail)
                    tail = previous; // update tail if we removed it

                delete current;
                size--;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    // Used for the undo feature: reduce quantity; if it drops to
    // 0 or below, remove the item entirely.
    bool reduceQuantity(int productId, int amount)
    {
        CartNode* item = findItem(productId);
        if (item == nullptr) return false;

        item->quantity -= amount;
        if (item->quantity <= 0)
            return removeItem(productId);
        return true;
    }

    bool updateQuantity(int productId, int newQuantity)
    {
        CartNode* item = findItem(productId);
        if (item == nullptr) return false;
        item->quantity = newQuantity;
        return true;
    }

    // Computing the total requires walking the whole list — O(n)
    double getTotal()
    {
        double total = 0.0;
        CartNode* current = head;
        while (current != nullptr)
        {
            total += current->price * current->quantity;
            current = current->next;
        }
        return total;
    }

    int getSize() { return size; }

    // Convert the list to a JSON array, so it can be sent to
    // Streamlit in an HTTP response
    json toJson()
    {
        json result = json::array();
        CartNode* current = head;
        while (current != nullptr)
        {
            json item;
            item["productId"] = current->productId;
            item["name"] = current->name;
            item["price"] = current->price;
            item["quantity"] = current->quantity;
            result.push_back(item);
            current = current->next;
        }
        return result;
    }

    // Free every node one by one
    void clear()
    {
        CartNode* current = head;
        while (current != nullptr)
        {
            CartNode* next = current->next; // save the next address first
            delete current;
            current = next;
        }
        head = nullptr;
        tail = nullptr;
        size = 0;
    }
};

#endif