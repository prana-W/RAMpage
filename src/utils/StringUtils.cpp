#include "StringUtils.h"
#include <stdexcept>

optional<long long> StringUtils::parseLong(const string& str) {
    try {
        return stoll(str);
    } catch (const invalid_argument&) {
        return nullopt;
    } catch (const out_of_range&) {
        return nullopt;
    }
}
