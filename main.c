#include <stdio.h>   //printf
#include <string.h> //memset
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h> 
#include <unistd.h>
#include "my_utils.h"
#include "rtp.h"

int main(int argc, char* argv[]){

//I. reading input----------------------------------------------
struct console_inputs C_inp; 
struct rtcp_util rtcp_u={0,0,0,0,40,36,"37.247.53.178",1,0,0};
struct timespec sleeping_time={0,0};

if(parse_inputs(argc,argv,(struct console_inputs*)&C_inp,(struct rtcp_util*)&rtcp_u)==1)
    return 1;
//--------------------------------------------------------------

//II. sockets creation, not in separate function because sockets block
struct socket_data D_sock;

D_sock.size_rtp=sizeof(D_sock.si_other_rtp);
D_sock.size_rtcp=sizeof(D_sock.si_other_rtcp);
    if ( (D_sock.s_rtp=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("failed_socket_rtp\n");
            return 1;}
    if ( (D_sock.s_rtcp=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
            printf("failed_socket_rtcp\n");
            return 1;}

    memset((char *) &D_sock.si_other_rtp, 0, sizeof(D_sock.si_other_rtp));
    D_sock.si_other_rtp.sin_family = AF_INET;
    D_sock.si_other_rtp.sin_port = htons(C_inp.port_rtp);
    
    memset((char *) &D_sock.si_other_rtcp, 0, sizeof(D_sock.si_other_rtcp));
    D_sock.si_other_rtcp.sin_family = AF_INET;
    D_sock.si_other_rtcp.sin_port = htons(C_inp.port_rtcp); // Here, the port number is defined. htons() ensures that the byte order is correct (Host TO Network order/Short integer). 
    if (inet_pton(AF_INET,C_inp.ip_addr, &D_sock.si_other_rtp.sin_addr) == 0) 
        { printf("inet_aton() failed\n");
        return 1;}
    
    if (inet_pton(AF_INET,C_inp.ip_addr, &D_sock.si_other_rtcp.sin_addr) == 0) 
    {   printf("inet_aton() failed\n");
        return 1;} 
//-------------------------------------------------------------- 


//III. memory allocation----------------------------------------  

uint8_t* sendbuf= (uint8_t*) malloc(C_inp.rtp_pack_len);
uint8_t* rtcp_sendbuf =(uint8_t*) malloc(rtcp_u.rtcp_buf_len);

memset(sendbuf,0, C_inp.rtp_pack_len);
memset(rtcp_sendbuf,0, rtcp_u.rtcp_buf_len);
struct circ_buf ncb={NULL,NULL,NULL,NULL,NULL,65534,0,NULL,0,NULL,0};

ncb.cbc =(uint8_t*) malloc(ncb.cb_len+1);
ncb.end_of_content=&ncb.cbc[ncb.cb_len]+1;//one after(like end of C++ vector)
ncb.cbc_begin=ncb.cbc;
ncb.cbc_end=ncb.cbc;
memset(ncb.cbc,0, ncb.cb_len+1);
struct rand_util rndf;
rand_utils_filler((struct rand_util*) &rndf,(struct rtcp_util*)&rtcp_u,(struct timespec*)&sleeping_time,(struct console_inputs*)&C_inp);
struct myargs mrg={sendbuf,(struct circ_buf*) &ncb,(struct rand_util*) &rndf,(struct console_inputs*)&C_inp,rtcp_sendbuf,(struct rtcp_util*)&rtcp_u,(struct socket_data*)&D_sock,(struct timespec*)&sleeping_time};

//III.A. thread creation----------------------------------------
pthread_t rtp_thr, rtcp_thr; 
//--------------------------------------------------------------


//IV. Main cycle(s)---------------------------------------------
printf("Starting threads\n");

pthread_create(&rtp_thr,NULL,rtp_sending_thread,(struct myargs*) &mrg); 
if(rtcp_u.rtcp_flag==1){
pthread_create(&rtcp_thr,NULL,rtcp_sending_thread,(struct myargs*) &mrg);
} 

//V. Closing, freeing and finalizing ---------------------------

pthread_join(rtp_thr, NULL); 
if(rtcp_u.rtcp_flag==1){
pthread_join(rtcp_thr, NULL);
}  
close(D_sock.s_rtp);
close(D_sock.s_rtcp);
fclose(C_inp.h264_stream);
free(sendbuf);
free(rtcp_sendbuf);
free(ncb.cbc);
//---------------------------------------------------------------

return 0;

}//end of main()  