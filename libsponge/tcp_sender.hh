#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    std::queue<TCPSegment> _in_flight_segments{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;
    unsigned int _next_timeout_ms{0};

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    uint64_t _last_ackno{0};

    // window size receive from ack, use wnd_size() to get real value
    uint16_t _wnd_size{1};

    // retransmit timer in ms
    size_t _timer_ms{0};

    // how many times retransmit
    unsigned int _rx_time{0};

    bool _fin_sent{false};

    uint16_t remaining_window_size() const {
        auto bif(bytes_in_flight());
        if (wnd_size() > bif) {
            return wnd_size() - bif;
        }
        return 0;
    }

    // _wnd_size record original value from ack, it returns real value
    uint16_t wnd_size() const {
      return _wnd_size == 0 ? 1 : _wnd_size;
    }

    // when seg has more than one byte then push it to stream_out
    // it returns if push successfully
    bool push_new_segment(const TCPSegment &&seg);

    void reset_timer();

    // reset timer and next_timeout backoff
    void backoff_timer();

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
    size_t time_since_last_segment_received() const { return _timer_ms; }
    
    unsigned int next_timeout_ms() const { return _next_timeout_ms; }

    bool is_closed() const { DEBUG(next_seqno_absolute());return next_seqno_absolute() == 0; }
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
