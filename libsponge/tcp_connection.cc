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
    cout << "recv seg " << seg.header().summary() << endl;
    cout << "start state " << state().name() << endl;
    _receiver.segment_received(seg);
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
    auto after_recv_state(state());
    cout << "recv state " << after_recv_state.name() << endl;
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        // cout << "sender ack " << state().name() << endl;
        // cout << "bytes in flight " << _sender.bytes_in_flight() << endl;
        auto after_send_state(state());
        if (after_send_state == TCPState(TCPState::State::FIN_WAIT_1)) {
        } else if (after_send_state == TCPState(TCPState::State::FIN_WAIT_2)) {

        } else if (after_send_state == TCPState(TCPState::State::CLOSING)) {

        } else if (after_send_state == TCPState(TCPState::State::CLOSE_WAIT)) {

        } else if (after_send_state == TCPState(TCPState::State::LAST_ACK)) {

        } else {

        }
        _sender.fill_window();
        // if only ack received or seg_out is not empty then don't send ack for it
        if (!only_ack(seg.header()) && _sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        // if (!outbound_done() && !_remote_fin_ack && !_recv_my_fin_ack) {
        //     _remote_fin_ack = (_sender.stream_in().eof() && _receiver.stream_out().eof());
        //     _sender.send_empty_segment();
        // }
        collect_output();
    }
}

bool TCPConnection::active() const {
    return !(_sender.stream_in().eof() && _receiver.stream_out().eof()) || _linger_after_streams_finish;
}

size_t TCPConnection::write(const string &data) {
    size_t byte_written = _sender.stream_in().write(data);
    _sender.fill_window();
    collect_output();
    return byte_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);
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
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::collect_output() {
    while (! _sender.segments_out().empty()) {
        TCPSegment &seg = _sender.segments_out().front();
        cout << "before coll seg " << seg.header().summary() << endl;
        if (_receiver.ackno().has_value() && !seg.header().ack) {
            cout << "recv ack " << _receiver.ackno().value() << " next seq " << _sender.next_seqno() << endl;
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            if (_receiver.stream_out().eof()) {
                cout << "add 1" << endl;
                seg.header().win = 1234;
            }
            cout << "recv stream_idx " << seg.header().ackno.raw_value() << endl;
                
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