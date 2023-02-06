Lab 4 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about 11 hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the TCPConnection:
[]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with:
run docker on my mac, so I met some problem when start tun, I added --device /dev/net/tun:/dev/net/tun
when docker run, but it still does not work. I search again and find that need to add --privileged

I figure out why there is another segments_out but _sender's already had one, I need collect_output
from _sender after call fill_window()

I used `auto modified_seg(_sender.segments_out().back())`, so compiler threat modified_seg as `TCESegment`
without &, so what I modified on it didn't take any effect.
- Optional: I think you could make this lab better by:

Add test name in fsm_active_close so that I can find which test failed quickly.
test1 check TCPConn as left hand side, test2 check as opposing side.

## DO NOT send ACK for ACK
I make a big mistake, I can't process send FIN, recv ACK, I sent another ACK for this ACK.
I fix this.

## AF with same ackno
Until to test5, I fail on reset_timer, because I don't send ACK for ACK, so when receive fin sender
don't reset timer. But if I always reset timer when call ack_received, it will fail on check_lab3 t_send_extra. I figure out why this fail, because remote send AF separately with same ackno,
I need reset timer after sender have sent fin because it will only receive fin or ack for fin.

## sender timer don't reset correctly
if in_flight_segments has nothing don't reset timer, but fin_sent is an exception.

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
