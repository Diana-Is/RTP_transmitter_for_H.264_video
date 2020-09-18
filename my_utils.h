#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

struct console_inputs{
	FILE* h264_stream;
	char ip_addr[16];//+ \0 character
    unsigned short port_rtp;
    unsigned short port_rtcp;
    unsigned short rtp_pack_len;
    unsigned short pack_len_var;
    unsigned short transl_ratio_numer;
    unsigned short transl_ratio_denom;
};

struct socket_data{
	int s_rtp;
	int s_rtcp;
	int size_rtp;
	int size_rtcp;
	struct sockaddr_in si_other_rtp;
	struct sockaddr_in si_other_rtcp;
};

struct circ_buf{
    uint8_t* cbc_begin;
    uint8_t* cbc_end;
    uint8_t* start_curr_n;
    uint8_t* start_next_n;
    uint8_t* fragm_nalu_pointer;
    int cb_len; //последний индекс, к которому можно обращаться
    long curr_n_len;
    uint8_t* end_of_content;
    int flag;
    uint8_t* cbc;
    int flag_flag;
};

struct rand_util{
  uint32_t ssrc;
  uint16_t seq_num_init;
  uint16_t  seq_num;
  uint16_t abs_seq_num;
  uint32_t ts;
  uint32_t ts_offset;
};

struct rtcp_util{
  int rtcp_flag;
  uint32_t data_sent;
  uint32_t ntp_sec;
  uint32_t ntp_frac;
  unsigned short rtcp_buf_len;
  unsigned short rtcp_bye_len; 
  char ntp_addr[16];//+ \0 character
  unsigned short timezone;
  uint32_t rtCp_ts;
  int last_sent;
};

struct myargs{ 
  uint8_t* p_sendbuf;
  struct circ_buf *p_ncb;
  struct rand_util *p_rndf;
  struct console_inputs *p_C_inp;
  uint8_t* p_rtcp_sendbuf;
  struct rtcp_util *p_rtcp_u;
  struct socket_data *p_D_sock;
  struct timespec *p_sleeping_time;
};

typedef struct {
    unsigned int sign : 1;
    unsigned int exponent : 8;
    unsigned int mantissa : 23;
}__attribute__ ((packed)) my_float_t;


int parse_inputs(int argc,char* argv[],struct console_inputs *C_inp,struct rtcp_util *rtcp_u);

void circ_buf_fill(struct circ_buf *ncb,FILE* h264_stream);

uint8_t* single_nalu_packet(uint8_t* sendbuf, struct circ_buf *ncb, struct rand_util *rndf,struct console_inputs *C_inp,struct rtcp_util *rtcp_u);
uint8_t* aggregation_packet(uint8_t* sendbuf, struct circ_buf *ncb, struct rand_util *rndf,struct console_inputs *C_inp,struct rtcp_util *rtcp_u);
uint8_t* fragmentation_packet(uint8_t* sendbuf,struct circ_buf *ncb, struct rand_util *rndf,struct console_inputs *C_inp,struct rtcp_util *rtcp_u);

int bytes_4_cmp(uint8_t *p);
int bytes_3_cmp(uint8_t *p);
int nalu_type_checker(uint8_t *p);

void rtp_hdr_template(uint8_t* sendbuf,struct rand_util *rndf);
uint8_t* next_nalu_searcher(uint8_t* my_iter,struct circ_buf *ncb);

unsigned char *gen_rdm (unsigned char *pointer,size_t num_bytes);
void update_curr_payload_len(struct circ_buf *ncb,uint8_t* pointer);
int find_next_8_multiplier(int value);
uint8_t* padding_or_reallocation_of_rtp_packet(uint8_t* sendbuf,struct console_inputs *C_inps,int filled_bytes);
uint32_t generate_ts_update(struct console_inputs *C_inp,struct rand_util *rndf, int number);

void* rtp_sending_thread(void *arg);
void* rtcp_sending_thread(void *arg);

void set_waiting_time(struct timespec *sleeping_time, struct console_inputs *C_inp);

void uploader(uint8_t* from_ptr,uint8_t* to_ptr,struct console_inputs *C_inp,struct circ_buf *ncb);
uint8_t* rtcp_packet_fill(uint8_t* rtcp_sendbuf,struct rand_util *rndf,struct rtcp_util *rtcp_u,int type);
void rand_utils_filler(struct rand_util *rndf,struct rtcp_util *rtcp_u,struct timespec *sleeping_time, struct console_inputs *C_inp);
int ntptime(struct rtcp_util *rtcp_u);

#endif