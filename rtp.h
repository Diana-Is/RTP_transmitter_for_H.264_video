#ifndef RTP_H
#define RTP_H

#include <stdint.h>

   /*
    * The type definitions below are valid for 32-bit architectures and
    * may have to be adjusted for 16- or 64-bit architectures.
    */

  // RTP data header

    typedef struct { 
        //first byte
       unsigned int cc:4;      // CSRC count 
       unsigned int x:1;       // header extension flag 
       unsigned int p:1;       //   padding flag 
       unsigned int version:2;  // protocol version 
        //second byte
       unsigned int pt:7;      // payload type 
       unsigned int m:1;       // marker bit 
       //3-4 bytes
       unsigned int seq:16;    // sequence number 
       //5-8 bytes
       uint32_t ts;            // timestamp 
       //9-12 bytes
       uint32_t ssrc;          // synchronization source 
      // uint32_t csrc[1];     // optional CSRC list 
   }__attribute__ ((packed)) rtp_hdr_t;

    typedef struct {
        uint8_t type:5; 
        uint8_t nri:2;
        uint8_t f:1;
    }__attribute__ ((packed)) nalu_hdr_t;

    typedef struct {
        uint8_t type: 5;
        uint8_t r: 1;
        uint8_t e: 1;
        uint8_t s: 1;
    } __attribute__ ((packed)) fu_hdr_t;
  
  // RTCP common header word
 
   typedef struct {
      unsigned int count:5;     /* varies by packet type */
      unsigned int p:1;         /* padding flag */
      unsigned int version:2;   /* protocol version */
       unsigned int pt:8;        /* RTCP packet type */
       uint16_t length;           /* pkt len in words, w/o this word */
   } rtcp_common_t; //32 bits

  /* sender report (SR) */
typedef struct {
      uint32_t ssrc;     /* sender generating this report */
      uint32_t ntp_sec;  /* NTP timestamp */
      uint32_t ntp_frac;
      uint32_t rtp_ts;   /* RTP timestamp */
      uint32_t psent;    /* packets sent */
      uint32_t osent;    /* octets sent */
     // rtcp_rr_t rr[1];  /* variable-length list */
} sr_t; //6*32 bits

typedef struct rtcp_sdes {
  uint32_t src;      /* first SSRC/CSRC */
 uint8_t type;       /* type of item (rtcp_sdes_type_t) */
 uint8_t length;     /* length of item (in octets) */
 char data[2];       /* text, not null-terminated */
} sdes_t;

/* BYE */
typedef struct {
uint32_t src;   /* list of sources */
} bye_t;        

  // Big-endian mask for version, padding bit and packet type pair

   #define RTCP_VALID_MASK (0xc000 | 0x2000 | 0xfe)
   #define RTCP_VALID_VALUE ((RTP_VERSION << 14) | RTCP_SR)


typedef struct
{
  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.
  
  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;              // Total: 384 bits or 48 bytes.

   
  
#endif
