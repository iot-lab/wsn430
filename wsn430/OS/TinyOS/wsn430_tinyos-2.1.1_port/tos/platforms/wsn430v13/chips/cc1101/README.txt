This is the adaptation of the CC2420 radio chip support for the CC1101.
Only the lower layers of the stack have been modified, in order to keep
compatibility with the CC2420 stack, therefore all the unmodified files
have kept their original names, with the 'CC2420' prefix. The modified
files have been renamed with the 'CC1101' prefix.

The modified files are those found in the transmit, receive and control
subfolders.

So far there are a few limitations:
  - ACK are not sent
  - the maximum packet size is 64bytes


To compile in the default Ack LPL version, #define the preprocessor variable:
  LOW_POWER_LISTENING
  
To compile in the PacketLink (auto-retransmission) layer, #define:
  PACKET_LINK
  
To remove all acknowledgements, #define (or use CC2420Config in 2.0.2)
  CC2420_NO_ACKNOWLEDGEMENTS
  
To use hardware auto-acks instead of software acks, #define:
  CC2420_HW_ACKNOWLEDGEMENTS

To stop using address recognition on the radio hardware, #define:
  CC2420_NO_ADDRESS_RECOGNITION

