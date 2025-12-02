#include <iostream>
#include <fstream>
#include <print>
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ranges>
#include <chrono>
#include <filesystem>
#include <random>
#include "word-iterator.hxx"

int main(int argc, char* argv[]) {
    namespace r = std::ranges;
    namespace v = std::ranges::views;
    namespace c = std::chrono;
    namespace fs = std::filesystem;
    using WordCount = std::pair<std::string, unsigned>;
    using namespace std::string_literals;
    using std::string;

    auto filename = fs::path{"../data/shakespeare.txt"};
    auto min_length = 6U;
    auto max_words = 100U;
    auto max_font  = 200U;
    auto min_font  = 40U;

    for (auto k = 1; k < argc; ++k) {
        auto arg = string{argv[k]};
        if (arg == "--file"s) {
            filename = fs::path{argv[++k]};
        } else if (arg == "--min"s) {
            min_length = std::stoul(argv[++k]);
        } else if (arg == "--max"s) {
            max_words = std::stoul(argv[++k]);
        }
    }

    auto start_time = c::high_resolution_clock::now();
    std::println("Loading {:.2f} MB from '{}'", fs::file_size(filename) / (1024.0 * 1024), filename.string());

    auto freqs = std::unordered_map<string, unsigned>{};
    freqs.reserve(10'000); {
        auto drop_small_words = [min_length](string const& word) -> bool {
            return word.size() >= min_length;
        };
        auto drop_modern_words = [](string const& word) -> bool {
            static auto const modern_words = std::unordered_set{
                "electronic"s, "distributed"s, "copies"s, "copyright"s, "gutenberg"s
            };
            return not modern_words.contains(word);
        };
        auto to_lower_case = [](string word) -> string {
            for (char& c: word) {
                auto ch = static_cast<unsigned char>(c);
                c = static_cast<char>(std::tolower(ch));
            }
            return word;
        };
        auto count_words = [&freqs](string const& word) { ++freqs[word]; };

        auto in = std::ifstream{filename};
        if (not in) {
            std::cerr << "Failed to open file\n";
            return 1;
        }

        // --- Alternative 1 ---
        auto in_iter  = WordIterator{in};
        auto eof_iter = WordIterator{};
        auto read_words_pipeline =
                r::subrange{in_iter, eof_iter}
                | v::filter(drop_small_words)
                | v::transform(to_lower_case)
                | v::filter(drop_modern_words);

        // --- Alternative 2 ---
        /*
        auto strip_non_letters = [](string const& word) {
            auto result = string{};
            result.reserve(word.size());
            auto is_letter = [](char c) -> bool {
                auto ch = static_cast<unsigned char>(c);
                return std::isalpha(ch) || ch == '\'';
            };
            for (char c: word) if (is_letter(c)) result += c;
            return result;
        };
        auto in_iter  = std::istream_iterator<string>{in};
        auto eof_iter = std::istream_iterator<string>{};
        auto read_words_pipeline =
                r::subrange{in_iter, eof_iter}
                | v::transform(strip_non_letters)
                | v::filter(drop_small_words)
                | v::transform(to_lower_case)
                | v::filter(drop_modern_words);
                */

        r::for_each(read_words_pipeline, count_words);
    }

    auto sortable = std::vector<WordCount>(freqs.begin(), freqs.end());
    auto by_count_desc = [](auto const& a, auto const& b) { return a.second > b.second; };
    r::partial_sort(sortable, sortable.begin() + max_words, by_count_desc);

    auto items = sortable | v::take(max_words) | r::to<std::vector<WordCount>>();
    auto max_cnt = items.front().second;
    auto min_cnt = items.back().second;
    auto scale = static_cast<double>(max_font - min_font) / (max_cnt - min_cnt);

    auto R = std::default_random_engine{std::random_device{}()};
    auto rnd_color = [&R]() {
        auto B = std::uniform_int_distribution<unsigned char>{0, 255};
        return std::format("#{:02X}{:02X}{:02X}", B(R), B(R), B(R));
    };

    auto to_span_tag = [scale, min_cnt, min_font, &rnd_color](WordCount const& wc) {
        auto const& word = wc.first;
        auto freq        = wc.second;
        auto size        = static_cast<int>(scale * (freq - min_cnt)) + min_font;
        auto colr        = rnd_color();
        return std::format(
            R"(<span style="font-size: {}px; color: {};" title="The word '{}' occurs {} times" >{}</span>)",
            size, colr, word, freq, word);
    };
    auto tags = items | v::transform(to_span_tag) | r::to<std::vector<string>>();
    r::shuffle(tags, R); {
        auto outfile = fs::path{filename.stem().string() + ".html"s};
        auto out = std::ofstream{outfile};
        if (not out) throw std::runtime_error{"cannot open outfile "s + outfile.string()};

        out << R"(<!DOCTYPE html>
            <html lang="en">
                <head>
                    <meta charset="UTF-8">
                    <meta name="viewport" content="width=device-width, initial-scale=1.0, shrink-to-fit=yes">
                    <title>Word Frequencies</title>
                </head>
            <body>
        )"
                << std::format("<h1>The {} most frequent words of <code>{}</code></h1>", max_words, filename.string());

        for (auto&& tag: tags) out << tag << "\n";

        out << "</body></html>\n";
        std::println("written result to '{}'", outfile.string());
    }

    auto end_time = c::high_resolution_clock::now();
    auto elapsed_time = c::duration_cast<c::milliseconds>(end_time - start_time);
    std::println("Elapsed time was {} ms", elapsed_time.count());
}
