#ifndef STACK_H
#define STACK_H

#include <string>

// ============================================================
// WHY STACK FOR UNDO?
// Undo always means "reverse the most recent action" — that is
// LIFO (Last In, First Out) behavior, which is exactly what a
// Stack models. Every cart action (add/remove) is "pushed" here;
// pressing "Undo" "pops" the most recent action and reverses it.
//
// WEAKNESS: if a "Redo" feature (undoing an undo) were needed
// later, a second stack would be required to hold undone actions.
// Our current scope only covers simple undo.
// ============================================================

// One action record: what happened in the cart, so it can be reversed
struct ActionNode
{
    std::string action;   // "add" or "remove"
    int productId;
    std::string name;
    double price;
    int quantity;
    ActionNode* next;      // points to the element "below" this one in the stack

    ActionNode(std::string a, int id, std::string n, double p, int q)
        : action(a), productId(id), name(n), price(p), quantity(q), next(nullptr) {}
};

class UndoStack
{
private:
    ActionNode* top;   // we only need "top" — unlike Linked List, we
                        // never need a tail, since we always work from the top
    int count;

public:
    UndoStack() : top(nullptr), count(0) {}
    ~UndoStack() { clear(); }

    // PUSH: the new action always goes on top. The old top becomes
    // this new node's "next". Time: O(1) — no need to walk to the
    // end of anything, which is exactly why Stack fits undo so well.
    void push(std::string action, int productId, std::string name, double price, int quantity)
    {
        ActionNode* node = new ActionNode(action, productId, name, price, quantity);
        node->next = top;
        top = node;
        count++;
    }

    // POP: remove the top element. Values are returned via
    // reference (&) "out parameters", since C++ doesn't let a
    // function return more than one value directly. Time: O(1)
    bool pop(std::string& action, int& productId, std::string& name, double& price, int& quantity)
    {
        if (top == nullptr) return false; // stack is empty — nothing to undo

        action = top->action;
        productId = top->productId;
        name = top->name;
        price = top->price;
        quantity = top->quantity;

        ActionNode* temp = top;
        top = top->next; // the next element becomes the new top
        delete temp;
        count--;
        return true;
    }

    bool isEmpty() { return top == nullptr; }
    int size() { return count; }

    void clear()
    {
        while (top != nullptr)
        {
            ActionNode* next = top->next;
            delete top;
            top = next;
        }
        count = 0;
    }
};

#endif