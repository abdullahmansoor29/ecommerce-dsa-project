#ifndef QUEUE_H
#define QUEUE_H

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ============================================================
// WHY QUEUE FOR PENDING ORDERS?
// In the real world, orders are usually processed "in the order
// they arrived" — that's FIFO (First In, First Out), which is
// exactly what a Queue models. Whichever order was placed first
// is the one the admin processes first — this is both fair and
// matches natural business logic.
//
// WEAKNESS: if priority orders were needed later (e.g. an "urgent"
// order jumping the line), a plain Queue wouldn't be enough — a
// Priority Queue (heap-based) would be required. In our scope, all
// orders are treated equally.
// ============================================================

// One order that hasn't been processed yet
struct OrderNode
{
    int orderId;
    std::string username;
    double total;
    OrderNode* next;

    OrderNode(int id, std::string user, double t)
        : orderId(id), username(user), total(t), next(nullptr) {}
};

class OrderQueue
{
private:
    OrderNode* front;  // the earliest order — this is where dequeue removes from
    OrderNode* rear;   // the most recently added order — this is where enqueue adds
    int count;

public:
    OrderQueue() : front(nullptr), rear(nullptr), count(0) {}
    ~OrderQueue() { clear(); }

    // ENQUEUE: a new order always goes at the rear. Time: O(1)
    // — the same idea as Linked List's addItem, just called "enqueue" here.
    void enqueue(int orderId, std::string username, double total)
    {
        OrderNode* node = new OrderNode(orderId, username, total);
        if (rear == nullptr) // queue was empty
        {
            front = node;
            rear = node;
        }
        else
        {
            rear->next = node;
            rear = node;
        }
        count++;
    }

    // DEQUEUE: remove the earliest order (FIFO). Time: O(1)
    // — the front pointer gives direct access to the start of the list.
    bool dequeue(int& orderId, std::string& username, double& total)
    {
        if (front == nullptr) return false; // no pending orders

        OrderNode* temp = front;
        orderId = temp->orderId;
        username = temp->username;
        total = temp->total;

        front = front->next;
        if (front == nullptr) rear = nullptr; // queue is now empty

        delete temp;
        count--;
        return true;
    }

    bool isEmpty() { return front == nullptr; }
    int size() { return count; }

    // JSON representation of the whole queue — the order matches
    // the order the orders arrived in (front to rear), so the admin
    // sees the correct FIFO order
    json toJson()
    {
        json result = json::array();
        OrderNode* current = front;
        while (current != nullptr)
        {
            json item;
            item["orderId"] = current->orderId;
            item["username"] = current->username;
            item["total"] = current->total;
            result.push_back(item);
            current = current->next;
        }
        return result;
    }

    void clear()
    {
        while (front != nullptr)
        {
            OrderNode* next = front->next;
            delete front;
            front = next;
        }
        rear = nullptr;
        count = 0;
    }
};

#endif