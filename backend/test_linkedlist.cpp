#include "dsa/LinkedList.h"
#include <iostream>

int main()
{
    CartLinkedList cart;

    cart.addItem(1, "Laptop", 1200.0, 1);
    cart.addItem(2, "Mouse", 25.0, 2);
    cart.addItem(1, "Laptop", 1200.0, 1); // dubara add, quantity badhegi

    std::cout << "Size: " << cart.getSize() << std::endl;
    std::cout << "Total: " << cart.getTotal() << std::endl;

    cart.removeItem(2);
    std::cout << "After removing Mouse, size: " << cart.getSize() << std::endl;

    std::cout << cart.toJson().dump(2) << std::endl;

    return 0;
}