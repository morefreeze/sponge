Lab 4 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

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

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
