#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    _cap = capacity;
    _ri = _wi = 0;
    _data = string(capacity, ' ');
}

size_t ByteStream::write(const string &data) {
    if (_wi == _ri + _cap) {
        return 0;
    }
    size_t i;
    for (i = 0; i < data.size() && _wi < _ri + _cap; ++i) {
        _data[_wi % _cap] = data[i];
        _wi++;
    }
    return i;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string result;
    size_t ori_ri(_ri);
    for (size_t i = 0; i < len; ++i) {
        result += _data[ori_ri % _cap];
        ori_ri++;
    }
    return result;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    read(len);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string result;
    if (len > buffer_size()) {
        _error = true;
        return result;
    }
    for (size_t i = 0; i < len; ++i) {
        result += _data[_ri % _cap];
        _ri++;
    }
    return result;
}

void ByteStream::end_input() { _write_end = true; }

bool ByteStream::input_ended() const { return _write_end; }

size_t ByteStream::buffer_size() const { return (_wi - _ri); }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _ri == _wi && input_ended(); }

size_t ByteStream::bytes_written() const { return _wi; }

size_t ByteStream::bytes_read() const { return _ri; }

size_t ByteStream::remaining_capacity() const { return _cap - buffer_size(); }
