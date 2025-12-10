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

    // Contains utils for helper functions which add to a response object. All errors are thrown with messages, and these messages are placed into 500 responses.
    namespace ResponseUtils {
        template <ResourceKind Resource>
        void response_put_all(Http::Response& res, Resource& resource) {
            Http::Blob response_resource = resource.as_full_blob();
            const auto response_size = response_resource.size();

            res.body = std::move(response_resource);

            res.headers.emplace("Content-Length", std::to_string(response_size));
            // @see `App::ResourceKind -> get_mime_desc requirement!`
            res.headers.emplace("Content-Type", resource.get_mime_desc().data());

            res.http_status = Http::Status::http_ok;
        }

        template <ResourceKind Resource>
        void response_put_chunked(Http::Response& res, Resource& resource) {
            ChunkIterPtr file_txt_it = resource.as_chunk_iter();

            // @see `App::ResourceKind -> get_mime_desc requirement!`
            res.headers.emplace("Content-Type", resource.get_mime_desc().data());

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
