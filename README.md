# RTP_transmitter_for_H.264_video
Console application for transmitting H.264 using RTP and RCTP protocols written in C.

User should provide following arguments to main(argc, argv[]) executing the file "app" (created with Makefile and gcc) in console:
- name of file intended for transmition, optionally including path to the file in the computer file system.
-IP-address of the computer, which will be receiving the stream, default "localhost".
- port number of the computer, which will be receiving the stream, default 51372.
- size of RTP-packet in bytes, default 1400.
- numerator for frames-per-second calculation, default 1200.
- denominator for frames-per-second calculation, default 50.
- “trigger”, switching program regimes: only RTP-packets ("def")/ RTP+RCTP-packets ("rtcp_on").
- IP-address of NTP-server for quiering timestamp for RTCP-packets, default 37.247.53.178.
- Timezone of user for correct calculation of RTCP-packet's timestamps, default UTC+3.

References:
RFC 3550 – RTP: A Transport Protocol for Real-Time Applications:https://tools.ietf.org/html/rfc3550, свободный.
RFC 6184 – RTP Payload Format for H.264 Video:https://tools.ietf.org/html/rfc6184.
RFC 4566 – SDP: Session Description Protocol: https://tools.ietf.org/html/rfc4566.
ITU-T H.264: https://www.itu.int/rec/T-REC-H.264-201704-I/en.

Application was created as a course work for the MSc course "Operation Systems", RTU MIREA.
