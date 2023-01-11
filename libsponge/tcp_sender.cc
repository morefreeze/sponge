#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <cassert>
#include <iostream>
#include <random>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _next_timeout_ms{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno - _last_ackno; }

void TCPSender::fill_window() {
    size_t left_wnd_size(remaining_window_size());
    cout << "remaining " << left_wnd_size << endl;
    if (left_wnd_size == 0) {
        return;
    }
    if (_next_seqno == 0) {  // send syn
        TCPSegment syn_seg;
        syn_seg.header().syn = true;
        syn_seg.header().seqno = _isn;
        push_new_segment(move(syn_seg));
        return;
    }
    if (_stream.buffer_empty() && !_stream.input_ended()) {
        return;
    }
    // push payload
    TCPSegment seg;
    string payload(_stream.read(min(left_wnd_size, _stream.buffer_size())));
    assert(!_stream.error);
    // cout << "pppppp " << stream_in().bytes_written() << stream_in().buffer_size() << " " << payload.size() << " [" <<
    // payload << "]" << endl;
    seg.header().seqno = wrap(_next_seqno, _isn);
    if (_stream.input_ended()) {
        seg.header().fin = true;
    }
    seg.payload() = Buffer(move(payload));
    // cout << "seg " << seg.header().seqno << seg.payload().size() << endl;
    push_new_segment(move(seg));
    // cout << "end remaining " << _next_seqno << " seg size " << _segments_out.size() << endl;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _wnd_size = window_size;
    _last_ackno = unwrap(ackno, _isn, _stream.bytes_read());
    while (!_in_flight_segments.empty()) {
        auto first(_in_flight_segments.front());
        if (unwrap(first.header().seqno, _isn, _stream.bytes_read()) < _last_ackno) {
            _in_flight_segments.pop();
            reset_timer();
        } else {
            break;
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer_ms += ms_since_last_tick;
    if (_timer_ms < _next_timeout_ms) {
        return ;
    }
    if (!_in_flight_segments.empty()) {
        auto first(_in_flight_segments.front());
        // retx seqno >= last_ackno, discard seg if seqno < last_ackno(like ack_received)
        assert(unwrap(first.header().seqno, _isn, _stream.bytes_read()) >= _last_ackno);
        _segments_out.push(first);
        backoff_timer();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _rx_time;
}

void TCPSender::send_empty_segment() { _stream.write(""); }

void TCPSender::push_new_segment(const TCPSegment &&seg) {
    _segments_out.push(seg);
    _in_flight_segments.push(TCPSegment(seg));
    _next_seqno += seg.length_in_sequence_space();
    if (seg.payload().size() > 0) {
        reset_timer();
    }
}

void TCPSender::reset_timer() {
    _timer_ms = 0;
    _rx_time = 0;
    _next_timeout_ms = _initial_retransmission_timeout;
}

void TCPSender::backoff_timer() {
    assert(_wnd_size > 0);
    ++_rx_time;
    _next_timeout_ms <<= 1;
    _timer_ms = 0;
}