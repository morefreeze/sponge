#include "tcp_connection.hh"

#include <iostream>
#include <cassert>

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
    DEBUG(seg.header().summary());
    DEBUG(state().name());
    // if the RST flag is set, sets both the inbound and outbound streams to error
    // TODO: and kill connection permanently.
    if (seg.header().rst) {
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        return;
    }
    // give seg to receiver so it can inspect the fileds it cares: seqno, SYNC, payload, FIN
    _receiver.segment_received(seg);
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    auto after_recv_state(state());
    cout << "recv state " << after_recv_state.name() << endl;
    // if ACK flag is set, tells the sender about the fields it cares: ackno and wnd_size
    if (seg.header().ack) {
    // if (_receiver.ackno().has_value())
        // if incoming seg occupied any seqno, Connection makes sure that at least one seg is sent in reply.
        // to update ackno and wnd_size
        _sender.ack_received(seg.header().ackno, seg.header().win);
        auto after_send_state(state()); // for debug
        // cout << "sender ack " << state().name() << endl;
        // cout << "bytes in flight " << _sender.bytes_in_flight() << endl;
        _sender.fill_window();
        // responding "keep-alive" seg
        // receive a seg with an invalid ackno to see if your connection is still alive
        // your connection should reply these segments
        // DEBUG(_receiver.ackno().has_value());
        // DEBUG(seg.length_in_sequence_space());
        // DEBUG(seg.header().seqno);
        // DEBUG(_receiver.ackno().value());
        if (_receiver.ackno().has_value() && seg.payload().size() == 0
        && seg.header().seqno == _receiver.ackno().value() - 1) {
            _sender.send_empty_segment();
        }
        // if only ack received or seg_out is not empty then don't send ack for it
        // if (!only_ack(seg.header()) && _sender.segments_out().empty()) {
        //     _sender.send_empty_segment();
        // }
        collect_output();
    }
}

bool TCPConnection::active() const {
    // return !(_sender.stream_in().eof() && _receiver.stream_out().eof()) || _linger_after_streams_finish;
    return !(prereq1() && prereq2() && prereq3()) || _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
    size_t byte_written = _sender.stream_in().write(data);
    _sender.fill_window();
    collect_output();
    return byte_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // abort the connection, and send a rst seg,
    // if the number of consecutive retx > TCPConfig::MAX_RETX_ATTEMPTS
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        send_rst_seg();
    }
    _sender.tick(ms_since_last_tick);
    // TODO: end the connection if necessary

    collect_output();
    cout << "tick " << ms_since_last_tick << " " << _sender.time_since_last_segment_received() << " " << _cfg.rt_timeout << endl;
    if (_sender.time_since_last_segment_received() >= _cfg.rt_timeout * 10) {
        _linger_after_streams_finish = false;
    }

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
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
            send_rst_seg();
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

// connection will send segment whenever sender push seg onto its outgoing queue.
void TCPConnection::collect_output() {
    while (! _sender.segments_out().empty()) {
        TCPSegment &seg = _sender.segments_out().front();
        cout << "before coll seg " << seg.header().summary() << endl;
        // connection will ask the recevier for the fields: ackno and wnd_size
        if (_receiver.ackno().has_value() && !seg.header().ack) {
            // cout << "recv ack " << _receiver.ackno().value() << " next seq " << _sender.next_seqno() << endl;
            // if there is an ackno, it will set ack
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win = _receiver.window_size() > std::numeric_limits<uint16_t>::max()
            ? std::numeric_limits<uint16_t>::max()
            : _receiver.window_size();
        segments_out().emplace(move(seg));
        _sender.segments_out().pop();
        cout << "coll seg " << seg.header().summary() << endl;
        cout << "stream out seg " << segments_out().back().header().summary() << endl;
    }
}

void TCPConnection::send_rst_seg() {
    // you can force sender to generate an empty seg with the proper seqno by calling send_empty_segment()
    _sender.send_empty_segment();
    assert(_sender.segments_out().size() == 1);
    TCPSegment &rst_seg = _sender.segments_out().front();
    rst_seg.header().rst = true;
    _sender.segments_out().pop();
    _segments_out.emplace(move(rst_seg));
}