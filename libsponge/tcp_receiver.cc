#include "tcp_receiver.hh"

#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        _syn_received = true;
        _isn = seg.header().seqno;
    }
    auto seqno(seg.header().seqno + seg.header().syn);
    _fin_received |= (seg.header().fin && _syn_received);
    if (_syn_received) {
        auto idx(stream_idx(seqno));
        // cout << seqno << " ack " << ackno().value_or(WrappingInt32{0}) << " idx " << idx << endl;
        _reassembler.push_substring(seg.payload().copy(), idx, _fin_received);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn_received) {
        return {};
    }
    return wrap(stream_out().bytes_written() + _syn_received + stream_out().input_ended(), _isn);
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
