#ifndef DERKHTTPD_MYAPP_RESPONSE_HELPERS_HPP
#define DERKHTTPD_MYAPP_RESPONSE_HELPERS_HPP

#include <utility>
#include <concepts>
#include <chrono>
#include <string>

#include "myhttp/msgs.hpp"
#include "myapp/contents.hpp"

namespace DerkHttpd::App {
    // NOTE: this constraint on web resources MUST have lifetimes like Rust's `'static` for the returned MIME name: the `std::string_view` must be a string literal.
    template <typename T>
    concept ResourceKind = requires (T arg) {
        {auto(arg.get_mime_desc())} -> std::same_as<std::string_view>;
        {auto(arg.as_chunk_iter())} -> std::same_as<ChunkIterPtr>;
        {auto(arg.as_full_blob())} -> std::same_as<Http::Blob>;
    };

    // Generates a GMT string for HTTP/1.1 responses: `%a, %e %b %Y %T UTC`, referencing: http://stackoverflow.com/questions/63501664/ddg#63502919
    [[nodiscard]] auto get_date_string() -> std::string;

    /// NOTE: LLVM 21 for macOS lacks `std::chrono::parse()`, so I'll do this the old-fashioned way: `std::istringstream` and `std::get_time`.
    [[nodiscard]] auto parse_date_string(const std::string& date) -> std::chrono::seconds;

    [[nodiscard]] auto get_epoch_seconds_now() -> std::chrono::seconds;

    // Contains utils for helper functions which add to a response object. All errors are thrown with messages, and these messages are placed into 500 responses.
    namespace ResponseUtils {
        template <ResourceKind Resource>
        void response_put_all(Http::Response& res, Resource& resource, Http::Status status) {
            Http::Blob response_resource = resource.as_full_blob();
            const auto response_size = response_resource.size();

            res.body = std::move(response_resource);
            
            res.headers.emplace("Content-Length", std::to_string(response_size));
            // @see `App::ResourceKind -> get_mime_desc requirement!`
            res.headers.emplace("Content-Type", resource.get_mime_desc().data());

            if constexpr (std::is_same_v<std::remove_cvref_t<Resource>, App::TextualFile>) {
                res.modify_timestamp = resource.get_modify_time();
            } else {
                res.modify_timestamp = get_epoch_seconds_now();
            }

            res.http_status = status;
        }

        template <ResourceKind Resource> requires (std::same_as<Resource, App::EmptyReply>)
        void response_put_all(Http::Response& res, Resource& status_only_dud) {
            Http::Blob response_resource = status_only_dud.as_full_blob();

            res.body = std::move(response_resource);

            res.headers.emplace("Content-Length", "0");
            res.headers.emplace("Content-Type", "*/*");

            res.modify_timestamp = get_epoch_seconds_now();

            res.http_status = status_only_dud.get_status();
        }

        template <ResourceKind Resource>
        void response_put_chunked(Http::Response& res, Resource& resource) {
            ChunkIterPtr file_txt_it = resource.as_chunk_iter();

            // @see `App::ResourceKind -> get_mime_desc requirement!`
            res.headers.emplace("Content-Type", resource.get_mime_desc().data());
            res.headers.emplace("Transfer-Encoding", "chunked");

            if constexpr (std::is_same_v<Resource, App::TextualFile>) {
                res.modify_timestamp = resource.get_modify_time();
            } else {
                res.modify_timestamp = get_epoch_seconds_now();
            }

            if (file_txt_it) {
                res.body = file_txt_it;

                res.http_status = Http::Status::http_ok;
            } else {
                Http::Blob dud_chunked_data;
                dud_chunked_data.append_range(std::string_view {"0\r\n\r\n"});

                res.body = std::move(dud_chunked_data);

                res.http_status = Http::Status::http_server_error;
            }
        }
    }
}

#endif
