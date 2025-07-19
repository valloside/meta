#include <string>
#include "test.h"

using namespace valloside;

meta::type get() {
    constexpr meta::type string = meta::reflect<std::string>();
    return string;
}