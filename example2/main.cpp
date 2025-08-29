#include <iostream>
#include "./myrustcode/generated.h"

int main() {
    auto v = rust::std::vec::Vec<int32_t>::new_();
    v.push(42);
    std::cout << "The value is "
              << *v.get(0).unwrap()
              << '\n';
}
