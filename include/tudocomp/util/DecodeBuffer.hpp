#ifndef _INCLUDED_DECODE_BUFFER_HPP_
#define _INCLUDED_DECODE_BUFFER_HPP_

#include <iostream>
#include <vector>

#include <sdsl/int_vector.hpp>

#include <glog/logging.h>

#include <tudocomp/util/DCBStrategyNone.hpp>
#include <tudocomp/util/DCBStrategyMap.hpp>
#include <tudocomp/util/DCBStrategyRetargetArray.hpp>

namespace tdc {

template <typename C>
class DecodeBuffer { // C should be one of: DCBStrategyNone, DCBStrategyMap, DCBStrategyRetargetArray

private:
    std::shared_ptr<C> m_cb_strategy;

    size_t m_len;
    uint8_t* m_text;
    sdsl::bit_vector m_decoded;

    size_t m_pos;

public:
    DecodeBuffer(size_t len)
        : m_len(len), m_pos(0) {

        m_text = new uint8_t[len];
        m_decoded = sdsl::bit_vector(len, 0);

        m_cb_strategy = std::shared_ptr<C>(new C(len));
    }

    ~DecodeBuffer() {
        delete[] m_text;
    }

    inline void decode(size_t pos, uint8_t sym) {
        do {
            DCHECK(pos < m_len);

            m_text[pos] = sym;
            m_decoded[pos] = 1;
        } while(m_cb_strategy->next_waiting_for(pos, pos));
    }

    inline void decode(uint8_t sym) {
        decode(m_pos, sym);
        ++m_pos;
    }

    inline void defact(size_t pos, size_t src, size_t num) {
        DCHECK(pos + num <= m_len);

        while(num) {
            if(m_decoded[src]) {
                decode(pos, m_text[src]);
            } else {
                //DLOG(INFO) << "wait: " << pos << " for " << src;
                m_cb_strategy->wait(pos, src);
            }

            ++pos;
            ++src;
            --num;
        }
    }

    inline void defact(size_t src, size_t num) {
        defact(m_pos, src, num);
        m_pos += num;
    }

    /// Write the current state of the decode buffer into an ostream.
    inline void write_to(std::ostream& out) const {
        out.write((const char*)m_text, m_len);
    }

    /// Return the expected size of the decoded text.
    inline size_t size() const {
        return m_len;
    }
};

}

#endif
