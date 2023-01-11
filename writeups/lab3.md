Lab 3 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [10] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the TCPSender:
[]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: I expected tick will process more logic,
but actually fill_window() did. I need process syn and fin flag carefully. I also need
figure out how many bytes stream should read.
At first I suppose set wnd_size=1 if ack pass wnd_size=0, but it has some tricks here.

- Optional: I think you could make this lab better by:
I should write this lab.md when I'm writing code and thinking.
I may refractor the implement of sender, now it's full of many hardcode and some dirty
styles.

- Optional: I was surprised by:
I'm surprised tick acts so easily that it only just retransmit timeout segments.

- Optional: I'm not sure about: [describe]
