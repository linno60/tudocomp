#ifndef LZ77_RULE_TEST_UTIL_H
#define LZ77_RULE_TEST_UTIL_H

namespace lz77rule_test {

using namespace tudocomp;

/// A simple builder class for testing the implementation of a compressor.
template<class T>
struct CompressorTest {
    Env env;
    T compressor { env };
    std::vector<uint8_t> m_input;
    Rules m_expected_rules;
    int m_threshold = 2;

    inline CompressorTest input(std::string inp) {
        CompressorTest copy = *this;
        copy.m_input = std::vector<uint8_t> { inp.begin(), inp.end() };
        return copy;
    }

    inline CompressorTest expected_rules(Rules expected) {
        CompressorTest copy = *this;
        copy.m_expected_rules = expected;
        return copy;
    }

    inline CompressorTest threshold(int thresh) {
        CompressorTest copy = *this;
        copy.m_threshold = thresh;
        return copy;
    }

    inline void run() {
        Input inp = Input::from_memory(m_input);
        Rules rules = compressor.compress(inp, m_threshold);

        assert_eq_sequence(rules, m_expected_rules);
    }
};

/// A simple builder class for testing the implementation of a encoder.
template<class T>
struct CoderTest {
    Env env;
    T coder { env };
    std::vector<uint8_t> m_input;
    Rules m_rules;
    std::string m_expected_output;
    bool output_is_string = true;

    inline CoderTest input(std::string inp) {
        CoderTest copy = *this;
        copy.m_input = std::vector<uint8_t> { inp.begin(), inp.end() };
        return copy;
    }

    inline CoderTest rules(Rules rs) {
        CoderTest copy = *this;
        copy.m_rules = rs;
        return copy;
    }

    inline CoderTest expected_output(std::string out) {
        CoderTest copy = *this;
        copy.m_expected_output = out;
        copy.output_is_string = true;
        return copy;
    }

    inline CoderTest expected_output(std::vector<uint8_t> out) {
        CoderTest copy = *this;
        copy.m_expected_output = std::string(out.begin(), out.end());
        copy.output_is_string = false;
        return copy;
    }

    inline void run() {
        Input inp = Input::from_memory(m_input);
        if (output_is_string) {
            auto actual_output = ostream_to_string([&] (std::ostream& out_) {
                Output out = Output::from_stream(out_);
                coder.code(std::move(m_rules), inp, out);
            });
            assert_eq_strings(m_expected_output, actual_output);
        } else {
            auto actual_output = ostream_to_bytes([&] (std::ostream& out_) {
                Output out = Output::from_stream(out_);
                coder.code(std::move(m_rules), inp, out);
            });
            assert_eq_hybrid_strings(std::vector<uint8_t>(
                        m_expected_output.begin(),
                        m_expected_output.end()),
                      actual_output);
        }
    }
};

/// A simple builder class for testing the implementation of a encoder.
template<class T>
struct DecoderTest {
    Env env;
    T decoder { env };
    std::vector<uint8_t> m_input;
    std::string m_expected_output;

    inline DecoderTest input(std::string inp) {
        DecoderTest copy = *this;
        copy.m_input = std::vector<uint8_t> { inp.begin(), inp.end() };
        return copy;
    }

    inline DecoderTest input(std::vector<uint8_t> inp) {
        DecoderTest copy = *this;
        copy.m_input = inp;
        return copy;
    }

    inline DecoderTest expected_output(std::string out) {
        DecoderTest copy = *this;
        copy.m_expected_output = out;
        return copy;
    }

    inline void run() {
        Input inp = Input::from_memory(m_input);
        auto actual_output = ostream_to_string([&] (std::ostream& out_) {
            Output out = Output::from_stream(out_);
            decoder.decode(inp, out);
        });
        assert_eq_strings(m_expected_output, actual_output);
    }
};

template<class Comp, class Cod>
void lz77roundtrip(const std::string input_string) {
    using R = Rules;

    Env env;

    Comp compressor { env };
    Cod coder { env };

    std::vector<uint8_t> inp_vec { input_string.begin(), input_string.end() };
    Input input = Input::from_memory(inp_vec);

    DLOG(INFO) << "ROUNDTRIP TEXT: " << input_string;

    // Compress to rules
    R rules = compressor.compress(input,
                                coder.min_encoded_rule_length(input.size()));

    DLOG(INFO) << "ROUNDTRIP PRE RULES";

    for (auto e : rules) {
        DLOG(INFO) << "ROUNDTRIP RULE: " << e;
    }

    DLOG(INFO) << "ROUNDTRIP POST RULES";

    // Encode input with rules
    std::string coded_string = ostream_to_string([&] (std::ostream& out_) {
        Output out = Output::from_stream(out_);
        coder.code(std::move(rules), input, out);
    });

    DLOG(INFO) << "ROUNDTRIP CODED: " << vec_to_debug_string(coded_string);

    //Decode again
    std::vector<uint8_t> coded_string_vec {
        coded_string.begin(),
        coded_string.end()
    };
    std::string decoded_string = ostream_to_string([&] (std::ostream& out_) {
        Output out = Output::from_stream(out_);
        Input coded_inp = Input::from_memory(coded_string_vec);

        coder.decode(coded_inp, out);
    });

    DLOG(INFO) << "ROUNDTRIP DECODED: " << decoded_string;

    assert_eq_strings(input_string, decoded_string);
}

}

#endif