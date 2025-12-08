#include <algorithm>
#include <utility>

#include "myuri/uri.hpp"

namespace DerkHttpd::Uri {
    Uri::Uri()
    : m_path {}, m_query {} {}

    Uri::Uri(std::string rel_path, std::map<std::string, QueryValue> query_items) noexcept
    : m_path (std::move(rel_path)), m_query (std::move(query_items)) {}

    auto Uri::path() const noexcept -> const std::string& {
        return m_path;
    }

    auto Uri::param(const std::string& name) const noexcept -> std::optional<const QueryValue*> {
        if (auto param_result = std::find_if(m_query.begin(), m_query.end(), [&name](const auto& key_value) {
            return key_value.first == name;
        }); param_result != m_query.end()) {
            return { &param_result->second };
        }

        return {};
    }

    auto Uri::params() const noexcept -> const std::map<std::string, QueryValue>& {
        return m_query;
    }
}
