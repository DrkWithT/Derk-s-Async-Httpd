#ifndef DERKHTTPD_MYAPP_CONTENTS_HPP
#define DERKHTTPD_MYAPP_CONTENTS_HPP

#include <string_view>
#include <optional>
#include <memory>
#include <filesystem>
#include <fstream>

#include "myhttp/msgs.hpp"

namespace DerkHttpd::App {
    // TODO: make generic to handle both binary and human-readable content.
    class ChunkIterBase {
    public:
        virtual ~ChunkIterBase() = default;

        virtual auto next() -> std::optional<Http::Blob> = 0;
    };

    /// NOTE: see `ChunkIterBase` for TODOs.
    using ChunkIterPtr = std::shared_ptr<ChunkIterBase>;

    class TextIterator : public ChunkIterBase {
    private:
        std::ifstream* m_ifstream_p;
        std::size_t m_chunk_len;

    public:
        TextIterator(std::ifstream* fs_p, std::size_t chunk_len) noexcept;

        [[nodiscard]] auto next() -> std::optional<Http::Blob> override;
    };

    // Implements the `ResourceKind` requirements for a human-readable file's content.
    class TextualFile {
    private:
        std::ifstream m_data;
        std::string_view m_mime;
        std::size_t m_chunk_len;

        TextualFile(std::filesystem::path path, std::string_view mime, std::size_t chunk_n);

    public:
        [[nodiscard]] static auto create(std::filesystem::path relative_path, std::string_view mime_identifier, std::size_t chunk_n) noexcept -> std::optional<TextualFile>;

        [[nodiscard]] constexpr auto get_mime_desc() const noexcept -> std::string_view {
            return m_mime;
        }

        [[nodiscard]] auto as_chunk_iter() noexcept -> ChunkIterPtr;

        [[nodiscard]] auto as_full_blob() noexcept -> Http::Blob;
    };

    class EmptyReply {
    private:
        Http::Status m_status;

        EmptyReply(Http::Status status) noexcept;
    public:

        [[nodiscard]] static auto create(Http::Status status) noexcept -> std::optional<EmptyReply>;

        [[nodiscard]] constexpr auto get_mime_desc() const noexcept -> std::string_view {
            return "*/*";
        }

        [[nodiscard]] constexpr auto as_chunk_iter() noexcept -> ChunkIterPtr {
            return {};
        }

        [[nodiscard]] constexpr auto as_full_blob() noexcept -> Http::Blob {
            return {};
        }
    };
}

#endif
