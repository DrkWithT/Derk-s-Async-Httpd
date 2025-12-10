#include <string>
#include <sstream>

#include "myapp/contents.hpp"

namespace DerkHttpd::App {
    TextIterator::TextIterator(std::ifstream* fs_p, std::size_t chunk_len) noexcept
    : m_ifstream_p {fs_p}, m_chunk_len {chunk_len} {}

    [[nodiscard]] auto TextIterator::next() -> std::optional<Http::Blob> {
        if (m_ifstream_p->eof()) {
            return Http::Blob {};
        }

        Http::Blob temp;

        for (std::size_t chunk_count = 0; chunk_count < m_chunk_len; ++chunk_count) {
            if (auto ascii_b = m_ifstream_p->get(); ascii_b != -1) {
                temp.push_back(static_cast<char>(ascii_b));
            } else {
                return {};
            }
        }

        return temp;
    }


    TextualFile::TextualFile(std::filesystem::path path, std::string_view mime, std::size_t chunk_n)
    : m_data {path}, m_mime {mime}, m_chunk_len {chunk_n} {}

    auto TextualFile::create(std::filesystem::path relative_path, std::string_view mime_identifier, std::size_t chunk_n) noexcept -> std::optional<TextualFile> {
        if (!relative_path.is_relative() && !relative_path.has_filename()) {
            return {};
        }

        return TextualFile {relative_path, mime_identifier, chunk_n};
    }

    auto TextualFile::as_chunk_iter() noexcept -> ChunkIterPtr {
        return std::make_shared<TextIterator>(&m_data, m_chunk_len);
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


    EmptyReply::EmptyReply(Http::Status status) noexcept
    : m_status {status} {}

    auto EmptyReply::create(Http::Status status) noexcept -> std::optional<EmptyReply> {
        return EmptyReply {status};
    }
}
