#include <utility>
#include <string>
#include <sstream>

#include "myapp/contents.hpp"

namespace DerkHttpd::App {
    TextIterator::TextIterator(std::ifstream fs_p, std::size_t chunk_len) noexcept
    : m_ifstream_p {std::move(fs_p)}, m_chunk_len {chunk_len} {}

    [[nodiscard]] auto TextIterator::next() -> std::optional<Http::Blob> {
        if (m_ifstream_p.eof() || m_chunk_len == 0) {
            return Http::Blob {};
        }

        Http::Blob temp;

        for (std::size_t chunk_count = 0; chunk_count < m_chunk_len; ++chunk_count) {
            if (auto ascii_b = m_ifstream_p.get(); ascii_b != -1) {
                temp.push_back(static_cast<char>(ascii_b));
            } else {
                break;
            }
        }

        return temp;
    }

    void TextIterator::clear() {
        m_ifstream_p = {};
        m_chunk_len = 0;
    }


    TextualFile::TextualFile(std::filesystem::path path, std::string_view mime, std::size_t chunk_n)
    : m_data {path}, m_path {path}, m_mime {mime}, m_chunk_len {chunk_n} {}
    
    [[nodiscard]] static auto dud() noexcept -> std::optional<TextualFile>;

    auto TextualFile::create(std::filesystem::path relative_path, std::string_view mime_identifier, std::size_t chunk_n) noexcept -> std::optional<TextualFile> {
        if (!relative_path.is_relative() && !relative_path.has_filename()) {
            return {};
        }

        return TextualFile {relative_path, mime_identifier, chunk_n};
    }

    /// NOTE: This method is destructive, moving the container `std::ifstream` into a deferred chunk generator for the response data.
    auto TextualFile::as_chunk_iter() noexcept -> ChunkIterPtr {
        return std::make_shared<TextIterator>(std::move(m_data), m_chunk_len);
    }

    auto TextualFile::as_full_blob() noexcept -> Http::Blob {
        std::ostringstream sout;
        std::string line;

        while (std::getline(m_data, line, '\n')) {
            sout << line << '\n';
        }

        Http::Blob blob;
        blob.append_range(sout.str());

        return blob;
    }

    /// NOTE: Gets a file's modification time as seconds since the Epoch start.
    auto TextualFile::get_modify_time() -> std::filesystem::file_time_type {
        return std::filesystem::last_write_time(m_path);
    }


    StringReply::StringReply(std::string s, std::string_view mime) noexcept
    : m_data (std::move(s)), m_mime {mime} {}

    StringReply::StringReply(Http::Blob&& b, std::string_view mime)
    : m_data {}, m_mime {mime} {
        m_data.append(b.begin(), b.end());
    }

    auto StringReply::as_full_blob() noexcept -> Http::Blob {
        Http::Blob blob;

        blob.append_range(m_data);

        return blob;
    }
}
