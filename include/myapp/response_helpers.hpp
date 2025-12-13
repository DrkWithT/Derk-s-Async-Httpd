#ifndef DERKHTTPD_MYAPP_RESPONSE_HELPERS_HPP
#define DERKHTTPD_MYAPP_RESPONSE_HELPERS_HPP

#include <utility>
#include <concepts>
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
            res.headers.emplace("Date", get_date_string());

            res.http_status = status;
        }

        template <ResourceKind Resource> requires (std::same_as<Resource, App::EmptyReply>)
        void response_put_all(Http::Response& res, Resource& status_only_dud) {
            Http::Blob response_resource = status_only_dud.as_full_blob();

            res.body = std::move(response_resource);

            res.headers.emplace("Content-Length", "0");
            res.headers.emplace("Content-Type", "*/*");
            res.headers.emplace("Date", get_date_string());

            res.http_status = status_only_dud.get_status();
        }

        template <ResourceKind Resource>
        void response_put_chunked(Http::Response& res, Resource& resource) {
            ChunkIterPtr file_txt_it = resource.as_chunk_iter();

            // @see `App::ResourceKind -> get_mime_desc requirement!`
            res.headers.emplace("Content-Type", resource.get_mime_desc().data());
            res.headers.emplace("Date", get_date_string());

            if (file_txt_it) {
                res.body = file_txt_it;
                res.headers.emplace("Transfer-Encoding", "chunked");

                res.http_status = Http::Status::http_ok;
            } else {
                res.body = {};
                res.headers.emplace("Content-Length", "0");

                res.http_status = Http::Status::http_server_error;
            }
        }
    }
}

#endif
