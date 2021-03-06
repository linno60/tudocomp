#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace tdc {
namespace io {
    /// \cond INTERNAL

    const uint8_t NULL_ESCAPE_ESCAPE_BYTE = 255;
    const uint8_t NULL_ESCAPE_REPLACEMENT_BYTE = 254;

    class EscapableBuf {
        std::shared_ptr<std::vector<uint8_t>> m_data;
        bool m_is_escaped;
    public:
        inline EscapableBuf():
            m_data(std::shared_ptr<std::vector<uint8_t>>()),
            m_is_escaped(false) {}

        inline EscapableBuf(View view):
            m_data(std::make_shared<std::vector<uint8_t>>(view)),
            m_is_escaped(false) {}

        inline EscapableBuf(std::vector<uint8_t>&& vec):
            m_data(std::make_shared<std::vector<uint8_t>>(std::move(vec))),
            m_is_escaped(false) {}

        inline EscapableBuf(const EscapableBuf& other):
            m_data(other.m_data),
            m_is_escaped(other.m_is_escaped) {}

        inline void escape_and_terminate() {
            auto& v = *m_data;

            if (!m_is_escaped) {
                const size_t old_size = v.size();
                size_t new_size = v.size();

                for (auto b : v) {
                    if (b == 0 || b == NULL_ESCAPE_ESCAPE_BYTE) {
                        new_size++;
                    }
                }

                new_size++; // 0 terminator
                DCHECK_GE(new_size, old_size + 1);

                // NB: The reserve call is neccessary to get an exact reallocation.
                v.reserve(new_size);
                v.resize(new_size);
                v.back() = 0;

                // check that the used vector implementation is actually sane
                DCHECK_EQ(v.capacity(), new_size);

                auto start = v.begin();
                auto mid = v.begin() + old_size;
                auto end = v.begin() + new_size - 1;

                while(start != end) {
                    --mid;
                    --end;

                    if (*mid == 0) {
                        *end = NULL_ESCAPE_REPLACEMENT_BYTE;
                        --end;
                        *end = NULL_ESCAPE_ESCAPE_BYTE;
                    } else if (*mid == NULL_ESCAPE_ESCAPE_BYTE) {
                        *end = NULL_ESCAPE_ESCAPE_BYTE;
                        --end;
                        *end = NULL_ESCAPE_ESCAPE_BYTE;
                    } else {
                        *end = *mid;
                    }
                }

                DCHECK_EQ(v.back(), 0);
                m_is_escaped = true;
            }
        }

        inline bool is_empty() { return !bool(m_data); }

        inline View view() const {
            return *m_data;
        }
    };

    class UnescapeBuffer: public std::streambuf {
    private:
        std::ostream* m_stream;
        bool saw_escape = false;
        bool saw_null_term = false;

        inline void push_unescape(uint8_t c) {
            saw_null_term = false;

            auto f = [&](uint8_t d){
                m_stream->put(d);
            };

            if (saw_escape) {
                saw_escape = false;
                if (c == NULL_ESCAPE_REPLACEMENT_BYTE) {
                    f(0);
                } else {
                    f(NULL_ESCAPE_ESCAPE_BYTE);
                }
            } else if (c == NULL_ESCAPE_ESCAPE_BYTE) {
                saw_escape = true;
            } else if (c == 0) {
                // drop it
                saw_null_term = true;
            } else {
                f(c);
            }
        }

    public:
        inline UnescapeBuffer(std::ostream& stream) : m_stream(&stream) {
        }

        inline virtual ~UnescapeBuffer() {
            DCHECK(saw_null_term);
        }

    protected:
        inline virtual int overflow(int ch) override {
            push_unescape(uint8_t(char(ch)));
            return ch;
        }

        inline virtual int underflow() override {
            return EOF;
        }
    };

    /// \endcond


}}

