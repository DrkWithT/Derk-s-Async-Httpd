#ifndef DERKHTTPD_MYURI_URI_HPP
#define DERKHTTPD_MYURI_URI_HPP

#include <optional>
#include <string>
#include <map>
#include <variant>

namespace DerkHttpd::Uri {
    using QueryValue = std::variant<std::string, int>;

    struct QueryPair {
        std::string name;
        QueryValue value;
    };

    class Uri {
    private:
        std::string m_path;
        std::map<std::string, QueryValue> m_query;

    public:
        Uri();

        Uri(std::string rel_path, std::map<std::string, QueryValue> query_items) noexcept;

        [[nodiscard]] auto path() const noexcept -> const std::string&;

        [[nodiscard]] auto param(const std::string& name) const noexcept -> std::optional<const QueryValue*>;

        [[nodiscard]] auto params() const noexcept -> const std::map<std::string, QueryValue>&;

        friend constexpr auto operator==(const Uri& lhs, const Uri& rhs) noexcept -> bool {
            if (lhs.m_path != rhs.m_path) {
                return false;
            }

            if (lhs.m_query.size() != rhs.m_query.size()) {
                return false;
            }

            for (const auto& [q_key, q_value] : lhs.m_query) {
                if (rhs.m_query.contains(q_key)) {
                    if (rhs.m_query.at(q_key) == q_value) {
                        continue;
                    }
                }

                return false;
            }

            return true;
        }
    };
}

#endif
