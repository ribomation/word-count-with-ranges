#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cctype>


class WordIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::string;
    using reference         = const std::string&;
    using pointer           = const std::string*;
    using difference_type   = std::ptrdiff_t;

    WordIterator() = default;
    explicit WordIterator(std::istream& is) : input{&is} {
        read_next();
    }

    reference operator*()  const { return current_word; }
    pointer   operator->() const { return &current_word; }

    WordIterator& operator++() {
        read_next();
        return *this;
    }
    WordIterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(WordIterator const& a, WordIterator const& b) {
        return a.at_eof == b.at_eof;
    }
    friend bool operator!=(WordIterator const& a, WordIterator const& b) {
        return !(a == b);
    }

private:
    std::istream* input{nullptr};
    std::string   current_word{};
    bool          at_eof{true};

    static bool is_letter(char c) {
        auto ch = static_cast<unsigned char>(c);
        return std::isalpha(ch) || ch == '\'';
    }

    void read_next() {
        if (input == nullptr) {
            at_eof = true;
            return;
        }

        current_word.clear();
        for (char ch; input->get(ch) && not is_letter(ch);) {}
        if (input->gcount()) input->unget();

        if (current_word.empty() && input->eof()) {
            at_eof = true;
            input = nullptr;
            return;
        }

        for (char ch; input->get(ch) && is_letter(ch);) {
            current_word.push_back(ch);
        }
        at_eof = false;
    }
};
