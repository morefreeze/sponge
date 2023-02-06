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
    DEBUG(_segments_out.size());
    DEBUG(next_seqno());
    DEBUG(left_wnd_size);
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
    DEBUG(_fin_sent);
    if (_fin_sent) {
        return;
    }
    do {
        // push payload
        TCPSegment seg;
        string payload(_stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, min(left_wnd_size, _stream.buffer_size()))));
        assert(!_stream.error());
        seg.header().seqno = next_seqno();
        if (_stream.eof() && !_fin_sent && payload.size() < left_wnd_size) {
            seg.header().fin = true;
            _fin_sent = true;
        }
        seg.payload() = Buffer(move(payload));
        DEBUG(seg.header().summary());
        if (!push_new_segment(move(seg))) {
            break;
        }
        left_wnd_size = remaining_window_size();
    } while (!_stream.buffer_empty());
    DEBUG(_segments_out.size());
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto ackno_raw_value(unwrap(ackno, _isn, _stream.bytes_read()));
    cout << "sender ack recv " << ackno_raw_value << " " << _next_seqno << endl;
    if (ackno_raw_value > _next_seqno) { // invalid ackno
        return ;
    }
    _wnd_size = window_size;
    cout << "sender check in flight size " << _in_flight_segments.size() << endl;
    if (_fin_sent) {
        // if fin_sent, receive remote fin or ack for fin should reset timer even in_flight_segments is empty
        reset_timer();
    }
    while (!_in_flight_segments.empty()) {
        auto first(_in_flight_segments.front());
        cout << "in flight " << first.header().summary() << endl;
        auto seg_ackno(unwrap(first.header().seqno, _isn, _stream.bytes_read()) + first.length_in_sequence_space());
        cout << "in flight ack " << seg_ackno << " " << ackno_raw_value << endl;
        if (seg_ackno <= ackno_raw_value) {
            _in_flight_segments.pop();
            _last_ackno = seg_ackno;
            reset_timer();
        } else {
            break;
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer_ms += ms_since_last_tick;
    DEBUG(_timer_ms);
    DEBUG(_next_timeout_ms);
    if (_timer_ms < _next_timeout_ms) {
        return ;
    }
    DEBUG(_in_flight_segments.size());
    // in flight segment has nothing, don't start timer
    // if fin_sent don't need reset timer only wait 10*rx_timeout
    if (_in_flight_segments.empty()) {
        if (!_fin_sent) {
            reset_timer();
        }
        return ;
    }
    // there are some in flight segments and timeout, re-tx them and next_timeout*2
    auto first(_in_flight_segments.front());
    // retx seqno >= last_ackno, discard seg if seqno < last_ackno(like ack_received)
    assert(unwrap(first.header().seqno, _isn, _stream.bytes_read()) >= _last_ackno);
    _segments_out.push(first);
    backoff_timer();
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _rx_time;
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    segments_out().emplace(move(seg));
}

bool TCPSender::push_new_segment(const TCPSegment &&seg) {
    if (seg.length_in_sequence_space() > 0) {
        _segments_out.push(seg);
        _in_flight_segments.push(TCPSegment(seg));
        _next_seqno += seg.length_in_sequence_space();
        return true;
        // reset_timer();
    }
    return false;
}

void TCPSender::reset_timer() {
    _timer_ms = 0;
    _rx_time = 0;
    _next_timeout_ms = _initial_retransmission_timeout;
}

void TCPSender::backoff_timer() {
    ++_rx_time;
    if (_wnd_size > 0) { // when win_size==0 treat as 1 and don't backoff
        _next_timeout_ms <<= 1;
    }
    _timer_ms = 0;
}