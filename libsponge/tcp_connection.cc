#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _sender.time_since_last_segment_received(); }

void TCPConnection::segment_received(const TCPSegment &seg) {
    cout << "start state " << state().name() << endl;
    _receiver.segment_received(seg);
    _sender.ack_received(seg.header().ackno, seg.header().win);
    cout << "act " << _sender.stream_in().input_ended() << " " << _receiver.stream_out().input_ended() << _sender.stream_in().eof() << _receiver.stream_out().eof() << endl;
    // cout << "linger " << _linger_after_streams_finish << endl;
    cout << "status:\nnow  " << state().name() << endl;
    // judge future state
    if (state() == TCPState(TCPState::State::ESTABLISHED) || state() == TCPState(TCPState::State::TIME_WAIT)) {
        // send ack after syn+ack
        TCPSegment resp_seg;
        resp_seg.header().ack = true;
        resp_seg.header().seqno = seg.header().ackno;
        resp_seg.header().ackno = _sender.next_seqno();
        segments_out().push(move(resp_seg));
    }
}

bool TCPConnection::active() const {
    return !(_sender.stream_in().eof() && _receiver.stream_out().eof()) || _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
    _sender.stream_in().write(data);
    _sender.fill_window();
    collect_output();
    return data.size();
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);
    cout << "tick " << _sender.time_since_last_segment_received() << " " << _cfg.rt_timeout << endl;
    if (_sender.time_since_last_segment_received() >= _cfg.rt_timeout * 10) {
        _linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    TCPSegment& modified_seg(_sender.segments_out().back());
    // must set seg ackno
    modified_seg.header().ack = true;
    modified_seg.header().ackno = _receiver.ackno().value();
    collect_output();
}

void TCPConnection::connect() {
    _sender.fill_window();
    collect_output();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::collect_output() {
    while (! _sender.segments_out().empty()) {
        _segments_out.push(move(_sender.segments_out().front()));
        _sender.segments_out().pop();
    }
}