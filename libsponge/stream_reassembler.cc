#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}


//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    insert(data, index);
    while (!_list.empty() && _list.front().idx <= _expected_idx) {
        auto node = _list.front();
        if (_expected_idx < node.idx + node.v.size()) {
            _expected_idx += _output.write(node.v.substr(_expected_idx - node.idx));
        }
        _list.pop_front();
    }
    if (eof) {
        _eof = true;
    }
    if (_eof && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unordered_bytes; }

bool StreamReassembler::empty() const { return _list.empty(); }

void StreamReassembler::insert(const std::string &v, size_t idx) { 
    Node node{std::move(v), idx};
    auto cur = _list.begin();
    while (cur != _list.end() && cur->idx < idx) {
        cur++;
    }
    if (cur == _list.end()) {
        _list.emplace_back(std::move(node));
    } else if (cur->idx > idx) {
        _list.insert(cur, std::move(node));
    } // cur->idx == idx duplicated
}
