#include <iostream>
#include "./myrustcode/generated.h"

int main() {
    std::cout << "The value is "
              << int32_t(rust::crate::get_a_foo().value)
              << '\n';
}
