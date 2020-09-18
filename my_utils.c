#include "my_utils.h"
#include "rtp.h"

//--------------------------------------------------------------

int parse_inputs(int argc,char* argv[],struct console_inputs *C_inp,struct rtcp_util *rtcp_u){
    	//assumed that :
    	//argv[1]=name_of_file
    	//argv[2]=ip_adress
    	//argv[3]=rtp_port
    	//argv[5]=transl_ratio_numerator
        //argv[6]=transl_ratio_denominator
        //argv[7]=flag rtcp
        //argv[8]=ntp server IP
        //argv[9]=timezone
         if (argc!=10){
          printf("all input params should be specified, you can fill all the fields except of name of file with word def, defaults will be used \n");
           return 1;}
         else{ 
            //argv[1]=name_of_file
         	if ((C_inp->h264_stream=fopen(argv[1],"r"))== NULL){
			printf("Can't open file\n");
			return 1;}

         	//argv[2]=ip_adress
         	if (strcmp(argv[2],"def")==0){
            //strlcpy
         	strncpy(C_inp->ip_addr,"127.0.0.1",15);
            printf("Packets are sent to localhost 127.0.0.1\n");
         	}
         	else if(strlen(argv[2])<=15){
         	strncpy(C_inp->ip_addr,argv[2],15);
         	}
         	else{
         		printf("too_long_ip_addr\n");
         		return 1;}

         	//argv[3]=rtp_port
			if (strcmp(argv[3],"def")==0){
         	C_inp->port_rtp=51372; //example from RFC 4566
         	C_inp->port_rtcp=C_inp->port_rtp+1;
            printf("default RTP port used = 51372\n");
         	}
         	else if(atoi(argv[3])>0){
         	C_inp->port_rtp=atoi(argv[3]);
         	C_inp->port_rtcp=C_inp->port_rtp+1;
         	}
         	else{
         		printf("wrong_rtp_port_number\n");
         		return 1;}

         	//argv[4]=buf_len
         	if (strcmp(argv[4],"def")==0){
         	C_inp->rtp_pack_len=1400;
            C_inp->pack_len_var=1400;
            printf("default rtp packet size is used = 1400\n");
            }
         	else if(atoi(argv[4])>24){ 
         		if(atoi(argv[4])%8==0){
         		C_inp->rtp_pack_len=atoi(argv[4]);
                C_inp->pack_len_var=C_inp->rtp_pack_len;}
         		else{
                C_inp->rtp_pack_len=find_next_8_multiplier(atoi(argv[4]));
                C_inp->pack_len_var=C_inp->rtp_pack_len;
         		printf("no,rtp packet size needs to be multiplier of 8,so will be %d\n",C_inp->rtp_pack_len);
         		}
         	}
         	else{
         		printf("wrong_buf_len_value\n");
         		return 1;}

         	//argv[5]=transl_ratio_numer
         	if (strcmp(argv[5],"def")==0){
         	C_inp->transl_ratio_numer=1200;
            }
         	else if(atof(argv[5])>0){
         	C_inp->transl_ratio_numer=atof(argv[5]);
         	}
         	else{
         		printf("wrong_translation_speed_ratio_numerator\n");
         		return 1;}

            //argv[6]=transl_ratio_denom
            if (strcmp(argv[6],"def")==0){
            C_inp->transl_ratio_denom=50;
            }
            else if(atof(argv[6])>0){
            C_inp->transl_ratio_denom=atof(argv[6]);
            }
            else{
                printf("wrong_translation_speed_ratio_denominator\n");
                return 1;}

            //argv[7]=rtcp flag
            if (strcmp(argv[7],"def")==0){
            rtcp_u->rtcp_flag=0;
            }
            else if(strcmp(argv[7],"rtcp_on")==0){
            rtcp_u->rtcp_flag=1;
            }
            else{
                printf("wrong_rtcp_parameter\n");
                return 1;}

            //argv[8]=ntp server
            if (strcmp(argv[8],"def")==0){
            printf("%s used as ntp server\n",rtcp_u->ntp_addr);
            }
            else if(strlen(argv[8])<=15){
            strncpy(rtcp_u->ntp_addr,argv[8],15);
            }
            else{
                printf("wrong_ntp_server_IP\n");
                return 1;}

            //argv[9]=timezone
            if (strcmp(argv[9],"def")==0){
            printf("timezone UTC+1\n");
            }
            else if(atoi(argv[9])<=12){
            rtcp_u->timezone=atoi(argv[9]);
            printf("timezone UTC+%d\n", rtcp_u->timezone);
            }
            else{
                printf("wrong_timezone\n");
                return 1;}

         	return 0;} //end of case 9 arguments

}//end of function

//--------------------------------------------------------------

void circ_buf_fill(struct circ_buf *ncb,FILE* h264_stream){
        if(ncb->start_curr_n==NULL){ //first call, begin of file
			size_t bytes_read=fread(ncb->cbc,1,ncb->cb_len+1,h264_stream);
            if(bytes_read!=ncb->cb_len+1){ncb->flag=1;}

             ncb->start_curr_n=next_nalu_searcher(ncb->cbc,ncb);
             ncb->start_next_n=next_nalu_searcher(ncb->start_curr_n+1,ncb);
			if (ncb->start_curr_n==NULL &&ncb->start_next_n==NULL){
				printf("no NALU header found, it is not h264 file, sorry");
				exit (EXIT_FAILURE);
			}
            update_curr_payload_len(ncb,ncb->start_curr_n);
		}//end of case of beginning of file

        else if(ncb->start_curr_n!=NULL &&ncb->start_next_n==NULL){ 

            if (ncb->start_next_n==NULL&&ncb->flag==0){
                
                printf("Expanding buffer\n");
                uint8_t* temp =(uint8_t*) malloc((ncb->cb_len+1)*2);
                memcpy(temp,ncb->cbc,ncb->cb_len+1);
                //destination,source,number of bytes 
                size_t bytes_read=fread(temp+ncb->cb_len+1,1,(ncb->cb_len+1),h264_stream); //начиная с условно пустого бита заполняем  
                if(bytes_read!=ncb->cb_len+1){ncb->flag=1;}
                free(ncb->cbc);
                ncb->cbc=temp;
                ncb->cb_len=(ncb->cb_len+1)*2-1;
                ncb->start_curr_n=ncb->cbc;
                ncb->fragm_nalu_pointer=ncb->cbc;
                ncb->end_of_content=&ncb->cbc[ncb->cb_len]+1;
                ncb->start_next_n=next_nalu_searcher(ncb->cbc,ncb);
                if (ncb->start_next_n!=NULL){
                update_curr_payload_len(ncb,ncb->start_curr_n);} 

            }
            }
            else {
                printf("circ_buf_fill_exit_failure\n" );
                exit (EXIT_FAILURE);

            }

}//end of func

//--------------------------------------------------------------

uint8_t* single_nalu_packet(uint8_t* sendbuf,struct circ_buf *ncb, struct rand_util *rndf,struct console_inputs *C_inp,struct rtcp_util *rtcp_u){
    memset(sendbuf, 0, C_inp->rtp_pack_len);

    //12 bytes rtp header
    rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&sendbuf[0];
    rtp_hdr_template(sendbuf,rndf);
    
    //starting from 13th byte filling rtp header and payload simply copying

    memcpy((uint8_t*)&sendbuf[12],ncb->start_curr_n,ncb->curr_n_len+1); //+1 because curr_n_leng = only payload
    rtcp_u->data_sent=rtcp_u->data_sent+ncb->curr_n_len;

    rtp_hdr->ts=generate_ts_update(C_inp,rndf, rndf->abs_seq_num);
    rtcp_u->rtCp_ts=rtp_hdr->ts;
    sendbuf=padding_or_reallocation_of_rtp_packet(sendbuf,C_inp,(13+ncb->curr_n_len));
    if(ncb->flag_flag!=0){
        ncb->cbc_end=ncb->start_curr_n;
    }
    ncb->flag_flag=1;
    ncb->cbc_begin=ncb->start_next_n;

        ncb->start_curr_n=ncb->start_next_n;
    	ncb->start_next_n=next_nalu_searcher(ncb->start_curr_n+1,ncb);
        if((ncb->start_next_n)!=NULL){
         update_curr_payload_len(ncb,ncb->start_curr_n);
        }
       

        rndf->seq_num++;

        return sendbuf;
    }//end of func

//--------------------------------------------------------------

uint8_t* aggregation_packet(uint8_t* sendbuf,struct circ_buf *ncb, struct rand_util *rndf,struct console_inputs *C_inp,struct rtcp_util *rtcp_u){
    memset(sendbuf, 0,C_inp->rtp_pack_len);
    //first 12 bytes rtp header
    rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&sendbuf[0];
    rtp_hdr_template(sendbuf,rndf);

    //13th byte aggr header
    nalu_hdr_t *aggr_hdr = (nalu_hdr_t*)&sendbuf[12];
    aggr_hdr->f=0;
    aggr_hdr->nri=((*(ncb->start_curr_n)& 96)>>5); 
    aggr_hdr->type=24;

    //14-15th bytes NALU_size
    uint16_t *Nalu_size = (uint16_t*)&sendbuf[13];
    *Nalu_size=htons(ncb->curr_n_len+1);
    //starting from 16th byte filling first NALU header & payload
    memcpy((uint8_t*)&sendbuf[15],ncb->start_curr_n,ncb->curr_n_len+1);

    rtcp_u->data_sent=rtcp_u->data_sent+ncb->curr_n_len;
    int bf=16+ncb->curr_n_len;//bytes filled
    if(ncb->flag_flag!=0){
        ncb->cbc_end=ncb->start_curr_n;
    }
    ncb->flag_flag=1;
    while (next_nalu_searcher(ncb->start_next_n,ncb)!=NULL||nalu_type_checker(ncb->start_next_n)==0||bf<=C_inp->rtp_pack_len-16){
        
            ncb->start_curr_n=ncb->start_next_n;
            ncb->cbc_begin=ncb->start_next_n;
        //The value of NRI MUST be the maximum of all the NAL units carried
        //in the aggregation packet. (RFC 6184)
        if (((*(ncb->start_curr_n))& 96) >>5 > aggr_hdr->nri){
            aggr_hdr->nri=((*(ncb->start_curr_n)& 96)>>5);}

        //The F bit MUST be cleared if all F bits of the aggregated NAL
        //units are zero; otherwise, it MUST be set. (RFC 6184)
         if (*(ncb->start_curr_n) & 128 ==1){
            aggr_hdr->f=1;}

            ncb->start_next_n=next_nalu_searcher(ncb->start_curr_n+1,ncb);
        
        if((ncb->start_next_n)!=NULL){
            update_curr_payload_len(ncb,ncb->start_curr_n);
        }
        else{break;}

        if (bf+3+ncb->curr_n_len>=C_inp->rtp_pack_len){
            break;} //3 = 2 bytes for size, + 1 for next nalu header 
        
        Nalu_size=(uint16_t*)&sendbuf[bf]; //bytes filled 
        *Nalu_size=htons(ncb->curr_n_len+1); //curr_n_length represents only size of payload
        bf=bf+2;
        //filling in next nalu header and next payload
        memcpy((uint8_t*)&sendbuf[bf],ncb->start_curr_n,ncb->curr_n_len+1);
        rtcp_u->data_sent=rtcp_u->data_sent+ncb->curr_n_len;
        bf=bf+ncb->curr_n_len+1;
    
    }//end of while
    rtp_hdr->ts=generate_ts_update(C_inp,rndf, rndf->abs_seq_num);
    rtcp_u->rtCp_ts=rtp_hdr->ts;
    sendbuf=padding_or_reallocation_of_rtp_packet(sendbuf,C_inp,bf);  
    rndf->seq_num++;
    return sendbuf;
}//end of func

//--------------------------------------------------------------

uint8_t* fragmentation_packet(uint8_t* sendbuf,struct circ_buf *ncb, struct rand_util *rndf,struct console_inputs *C_inp,struct rtcp_util *rtcp_u){
    long data_length=0;
    long data_sent=0;
    uint8_t* current_end=NULL;
    if(ncb->end_of_content==&ncb->cbc[ncb->cb_len]+1){
        current_end=&ncb->cbc[ncb->cb_len]+1;
    }
    else{   
        if(ncb->start_curr_n>ncb->end_of_content){
            current_end=&ncb->cbc[ncb->cb_len]+1;
        }
        else{
            current_end=ncb->end_of_content;
        }
    }
    memset(sendbuf, 0, C_inp->rtp_pack_len);
    //first 12 bytes rtp header
    rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&sendbuf[0];
    rtp_hdr_template(sendbuf,rndf);

    //13th byte aggr header
    nalu_hdr_t *fu_indicator = (nalu_hdr_t*)&sendbuf[12];
    fu_indicator->f=0;
    fu_indicator->nri=(*(ncb->start_curr_n)&96)>>5;
    fu_indicator->type=28;

    //14th byte fu header
    fu_hdr_t *fu_hdr = (fu_hdr_t*)&sendbuf[13];
    fu_hdr->r=0;
    fu_hdr->type=*(ncb->start_curr_n)&31;

    if (ncb->fragm_nalu_pointer==ncb->start_curr_n&&ncb->start_curr_n!=ncb->cbc){ //begin fragment
    fu_hdr->s=1;
    fu_hdr->e=0; 
    //starting from 15th byte payload
    uint8_t* nalu_payload = (uint8_t*)&sendbuf[14];
    if(current_end-(ncb->fragm_nalu_pointer+1)>=C_inp->rtp_pack_len-14){
    memcpy(nalu_payload,(ncb->fragm_nalu_pointer+1),C_inp->rtp_pack_len-14);
    ncb->fragm_nalu_pointer=ncb->start_curr_n+C_inp->rtp_pack_len-13;
    rtcp_u->data_sent=rtcp_u->data_sent+C_inp->rtp_pack_len-14;
    }
    else if (current_end-(ncb->fragm_nalu_pointer+1)<C_inp->rtp_pack_len-14) {
    data_length=current_end-(ncb->fragm_nalu_pointer+1);
    memcpy(nalu_payload,(ncb->fragm_nalu_pointer+1),data_length);
    ncb->fragm_nalu_pointer=current_end;
    rtcp_u->data_sent=rtcp_u->data_sent+data_length;
    sendbuf=padding_or_reallocation_of_rtp_packet(sendbuf,C_inp,14+data_length);
    }
    }
    else{//middle fragment
            //starting from 15th byte payload
            uint8_t* nalu_payload = (uint8_t*)&sendbuf[14];
            if(ncb->start_next_n!=NULL){
                if(ncb->start_curr_n==ncb->cbc){
                    data_sent=ncb->curr_n_len-(ncb->fragm_nalu_pointer-ncb->start_curr_n);}
                    else{data_sent=ncb->curr_n_len-(ncb->fragm_nalu_pointer-ncb->start_curr_n-1);}

                if (data_sent>C_inp->rtp_pack_len-14){
                    fu_hdr->s=0;
                    fu_hdr->e=0; 
                    memcpy(nalu_payload,(ncb->fragm_nalu_pointer),C_inp->rtp_pack_len-14);
                    ncb->fragm_nalu_pointer= ncb->fragm_nalu_pointer+C_inp->rtp_pack_len-14;
                    rtcp_u->data_sent=rtcp_u->data_sent+C_inp->rtp_pack_len-14;
                    }
                else{ //end fragment
                    fu_hdr->s=0;
                    fu_hdr->e=1;

                    if(bytes_4_cmp(ncb->start_next_n-4)){
                        data_length=ncb->start_next_n-ncb->fragm_nalu_pointer-4;} //only payload
                    else{
                        data_length=ncb->start_next_n-ncb->fragm_nalu_pointer-3;}
                        memcpy(nalu_payload,(ncb->fragm_nalu_pointer),data_length);

                        rtcp_u->data_sent=rtcp_u->data_sent+data_length;
                        ncb->fragm_nalu_pointer=ncb->start_next_n;
                        sendbuf=padding_or_reallocation_of_rtp_packet(sendbuf,C_inp,14+data_length);
                    }
                    }
            else{ //only middle_packet, next==NULL
                fu_hdr->s=0;
                fu_hdr->e=0; 
                if(current_end-ncb->fragm_nalu_pointer>=C_inp->rtp_pack_len-14){
                    memcpy(nalu_payload,(ncb->fragm_nalu_pointer),C_inp->rtp_pack_len-14);
                    ncb->fragm_nalu_pointer= ncb->fragm_nalu_pointer+C_inp->rtp_pack_len-14;
                    rtcp_u->data_sent=rtcp_u->data_sent+C_inp->rtp_pack_len-14;
                    
                }
                else{      
                data_length=current_end-ncb->fragm_nalu_pointer;
                memcpy(nalu_payload,(ncb->fragm_nalu_pointer),data_length);
                rtcp_u->data_sent=rtcp_u->data_sent+data_length;
                ncb->fragm_nalu_pointer=current_end;

                sendbuf=padding_or_reallocation_of_rtp_packet(sendbuf,C_inp,14+data_length);
                }
            }

                    
        }
    rndf->seq_num++;
    return sendbuf;
    }//end_of_func

//--------------------------------------------------------------

void rand_utils_filler(struct rand_util *rndf,struct rtcp_util *rtcp_u, struct timespec *sleeping_time, struct console_inputs *C_inp){
    set_waiting_time(sleeping_time, C_inp);
    time_t t;   
    srand((unsigned) time(&t));
    unsigned char u16[2];
    unsigned char u32[4];
    memcpy(&(rndf->ssrc), gen_rdm(u32,4),4);
    memcpy(&(rndf->seq_num), gen_rdm(u16,2),2);
    memcpy(&(rndf->seq_num_init),&(rndf->seq_num),2);
    unsigned char y[4];
    y[0]=0;
    y[1]=0;
    y[2]=0;
    y[3]= rand (); 
    memcpy(&(rndf->ts_offset),y,4);
    if(rtcp_u->rtcp_flag==1){
    if(ntptime(rtcp_u)==0){//if we could connect to NTP server
    printf("RTP timestamp is calculated from NTP\n");
    //ITALY = UTC+1
    rndf->ts=(htonl(htonl(rtcp_u->ntp_sec)+3600*rtcp_u->timezone))*90000+htonl(rndf->ts_offset);
    rndf->ts=htonl(rndf->ts);
    }
    else{
    rtcp_u->rtcp_flag=0;
    rndf->ts=rndf->ts_offset;
    printf("coldnt connect to ntp server, error, transmitting only RTP\n");
    }
    }
    else{
    rndf->ts=rndf->ts_offset;
    }
    
    rndf->abs_seq_num=0;
    }//end_of_func

//--------------------------------------------------------------

unsigned char *gen_rdm (unsigned char *pointer,size_t num_bytes){
  for (int i = 0; i < num_bytes; i++)
  {
    pointer[i] = rand ();
  }
  return pointer;
}//end_of_func

//--------------------------------------------------------------

int bytes_4_cmp(uint8_t *p){
	if ((*p==0x00)&&(*(p+1)==0x00)&&(*(p+2)==0x00)&&(*(p+3)==0x01)){
		return 1;}
	else{
		return 0;}
}//end of func

//--------------------------------------------------------------

int bytes_3_cmp(uint8_t *p){
    if ((*p==0x00)&&(*(p+1)==0x00)&&(*(p+2)==0x01)){
        return 1;}
    else{
        return 0;}
}//end of func

//--------------------------------------------------------------

int nalu_type_checker(uint8_t *p){
	if((*p&31)==5||(*p&31)==1) {
		return 1;}
	else{
		return 0;}
}//end_of_func

//--------------------------------------------------------------

void rtp_hdr_template(uint8_t* sendbuf,struct rand_util *rndf){
    rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&sendbuf[0];
    rtp_hdr->version = 2; //as in specification
    rtp_hdr->p = 0; //default, but, very probably will be changed
    rtp_hdr->x = 0; //no extension
    rtp_hdr->cc = 0; //no CSRC identifiers
    rtp_hdr->m = 0; // no need in marker bit
    rtp_hdr->pt = 96; //choosing from range 96-127, because no number assigned for H264
    rtp_hdr->seq = htons(rndf->seq_num);//randomly generated init value
    rtp_hdr->ts = htonl(rndf->ts);
    rtp_hdr->ssrc = htonl(rndf->ssrc);
}//end_of_func

//--------------------------------------------------------------
void set_waiting_time(struct timespec *sleeping_time, struct console_inputs *C_inp){
    float interval= 0.75*((float)C_inp->transl_ratio_denom/C_inp->transl_ratio_numer);
    sleeping_time->tv_sec=interval;
    //getting_rid_of_mantissa, time_t is equivalent to integer
    sleeping_time->tv_nsec=(interval-sleeping_time->tv_sec)*1000000000;
}
//--------------------------------------------------------------

void update_curr_payload_len(struct circ_buf *ncb,uint8_t* pointer){
   if(bytes_4_cmp(ncb->start_next_n-4)){
            ncb->curr_n_len=ncb->start_next_n-pointer-5;} //only payload
    else if (bytes_3_cmp(ncb->start_next_n-3)){
            ncb->curr_n_len=ncb->start_next_n-pointer-4;
            }
    else if(ncb->start_next_n==ncb->end_of_content){
        ncb->curr_n_len=ncb->start_next_n-pointer-1;
    }
    else{
        printf("can't update_curr_payload_len\n");
        exit(EXIT_FAILURE);
    }
}//end_of_func
  

int find_next_8_multiplier(int value){
while(value%8!=0){
    value++;
}
return value;
}//end_of_func

uint8_t* padding_or_reallocation_of_rtp_packet(uint8_t* sendbuf,struct console_inputs *C_inp,int filled_bytes){
 if(C_inp->rtp_pack_len-filled_bytes<=255){   
            sendbuf[C_inp->rtp_pack_len-1]=C_inp->rtp_pack_len-filled_bytes;
            rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&sendbuf[0];
            rtp_hdr->p = 1;
        }
            else{
                if((filled_bytes)%8==0){
                C_inp->pack_len_var=filled_bytes;
                sendbuf=realloc(sendbuf,C_inp->pack_len_var);
                }
                else{
                C_inp->pack_len_var=find_next_8_multiplier(filled_bytes);
                sendbuf=realloc(sendbuf,C_inp->pack_len_var);
                sendbuf[C_inp->pack_len_var-1]=(C_inp->pack_len_var)-filled_bytes;
                rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&sendbuf[0];
                rtp_hdr->p = 1;
                }
            }

return sendbuf;

}//end_of_func

uint32_t generate_ts_update(struct console_inputs *C_inp,struct rand_util *rndf, int number){
        
    double tst=(htonl(rndf->ts)*C_inp->transl_ratio_numer+C_inp->transl_ratio_denom*90000*number)/C_inp->transl_ratio_numer+0.5;
    uint32_t timestamp32 =tst;
    return htonl(timestamp32);

}//end_of_func


uint8_t* rtcp_packet_fill(uint8_t* rtcp_sendbuf,struct rand_util *rndf,struct rtcp_util *rtcp_u,int type){
memset(rtcp_sendbuf, 0, rtcp_u->rtcp_buf_len);
//each periodically transmitted compound RTCP packet MUST include a report packet [RFC 3550]
rtcp_common_t *rtcp_hdr_sr = (rtcp_common_t*)&rtcp_sendbuf[0];
rtcp_hdr_sr->version=2;
rtcp_hdr_sr->p=0; //we have 32 words only here, need no padding.
rtcp_hdr_sr->count=0;//no receiver reports attached
rtcp_hdr_sr->pt=200;
rtcp_hdr_sr->length=htons(6);//1*32 header + 6*32 sr -1; 

sr_t *send_rep=(sr_t*)&rtcp_sendbuf[4];
send_rep->ssrc=htonl(rndf->ssrc);

if(ntptime(rtcp_u)==0){//if we could connect to NTP server
printf("NTP time\n");
send_rep->ntp_sec=htonl(htonl(rtcp_u->ntp_sec)+3600*rtcp_u->timezone); //ITALY = UTC+1
send_rep->ntp_frac=rtcp_u->ntp_frac;
send_rep->rtp_ts=htonl(htonl(send_rep->ntp_sec)*90000+htonl(rndf->ts_offset));
}
else{
    return NULL;
    printf("couldnt connect to ntp server, error\n");
}

send_rep->psent=htonl(rndf->seq_num-rndf->seq_num_init);
send_rep->osent=htonl(rtcp_u->data_sent);


if(type ==0){ //non-BYE packet
//An SDES packet containing a CNAME item MUST be included in each compound RTCP packet [RFC 3550]
rtcp_common_t *rtcp_hdr_sdes=(rtcp_common_t*)&rtcp_sendbuf[28];
rtcp_hdr_sdes->version=2;
rtcp_hdr_sdes->p=0;
rtcp_hdr_sdes->count=1;
rtcp_hdr_sdes->pt=202;
rtcp_hdr_sdes->length=htons(2); //=3-1
//header=1st 32 bit word

sdes_t *rtcp_sdes=(sdes_t*)&rtcp_sendbuf[32];
rtcp_sdes->src=htonl(rndf->ssrc); //2nd 32 bit word 
rtcp_sdes->type=1; //8 bit
rtcp_sdes->length=2;//length of text, 8 bit word 
char data[2]="DI";
memcpy(&(rtcp_sdes->data),&data,2); //intentionaly cheating //3rd 32 bit word
return rtcp_sendbuf;
}

else if(type==1){ //BYE-packet
rtcp_sendbuf=realloc(rtcp_sendbuf,rtcp_u->rtcp_bye_len);
rtcp_common_t *rtcp_hdr_bye=(rtcp_common_t*)&rtcp_sendbuf[28];
rtcp_hdr_bye->version=2;
rtcp_hdr_bye->p=0;
rtcp_hdr_bye->count=1;
rtcp_hdr_bye->pt=203;
rtcp_hdr_bye->length=htons(1);
bye_t *bye=(bye_t*)&rtcp_sendbuf[32];
bye->src=htonl(rndf->ssrc);

return rtcp_sendbuf;
}
}//end_of_func


int ntptime(struct rtcp_util *rtcp_u) {
int portno=123;
uint32_t msg[12];
memset(msg, 0,48);
// Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.
*( ( char * ) &msg + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.
struct sockaddr_in server_addr;

int s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
memset( &server_addr, 0, sizeof( server_addr));
server_addr.sin_family=AF_INET;
server_addr.sin_addr.s_addr = inet_addr(rtcp_u->ntp_addr);
server_addr.sin_port=htons(portno);
// send the data to the timing server

if (sendto(s,msg,48,0,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1){
printf("error with sendto()_ntp\n");
return 1;}

socklen_t len = sizeof (server_addr);
if(recvfrom(s,msg,48,0,(struct sockaddr *)&server_addr,&len)==-1){
printf("error with sendto()_ntp\n");
return 1;}
//destination,source,number of bytes
rtcp_u->ntp_sec=msg[10];
rtcp_u->ntp_frac=msg[11];

close(s);
printf("GOT NTP\n");
return 0;
}
  

//--------------------------------------------------------------

uint8_t* next_nalu_searcher(uint8_t* my_iter,struct circ_buf *ncb){
    uint8_t* current_end=NULL;
    uint8_t* inner_iter=my_iter;
    if(ncb->end_of_content==&ncb->cbc[ncb->cb_len]+1){
        current_end=&ncb->cbc[ncb->cb_len]+1;
    }
    else{   
        if(my_iter>ncb->end_of_content){
            current_end=&ncb->cbc[ncb->cb_len]+1;
        }
        else{
            current_end=ncb->end_of_content;
        }
    }
         for(;inner_iter<=(current_end-4);inner_iter++){
            if(bytes_4_cmp(inner_iter)==1){
            return inner_iter+4;
            break;
           }
           if((bytes_3_cmp(inner_iter)==1)&&(bytes_4_cmp(inner_iter-1)!=1)){
            return inner_iter+3;
            break;
           }
            }//end for
            if(ncb->flag==0&&inner_iter==&ncb->cbc[ncb->cb_len]-2){
            return NULL;}
            else if(ncb->flag==1&&inner_iter==&ncb->cbc[ncb->cb_len]-2){
            return NULL;}
            else if(ncb->flag==1&&inner_iter==ncb->end_of_content-3){
            return ncb->end_of_content;}
}//end_of_func

//---------------------------------------------------------------

void uploader(uint8_t* from_ptr,uint8_t* to_ptr,struct console_inputs *C_inp,struct circ_buf *ncb){
    if(to_ptr>=from_ptr){
        
    size_t bytes_read=fread(from_ptr,1,to_ptr-from_ptr,C_inp->h264_stream); //начиная с условно пустого бита заполняем  
            
            if(bytes_read!=(to_ptr-from_ptr)){
                ncb->end_of_content=from_ptr+bytes_read;      
                ncb->flag=1;
                printf("END_OF_FILE_REACHED\n");     
            }
        }
    else{
        printf("Wrong_pointers_for_upload\n");
        exit(EXIT_FAILURE);
    }

}//end_of_func

void* rtp_sending_thread(void *arg){
struct timespec fora={1,0};
if(clock_nanosleep(CLOCK_REALTIME, 0, &fora, NULL)!=0){
            printf("Clock_failure\n");
            exit(EXIT_FAILURE);}
struct myargs *mrg=arg;
while(mrg->p_ncb->start_next_n==NULL){
circ_buf_fill(mrg->p_ncb,mrg->p_C_inp->h264_stream);
}


while(1){

//we have at least one NALU to send for sure 
if(mrg->p_ncb->start_next_n!=NULL&&mrg->p_ncb->start_curr_n!=mrg->p_ncb->end_of_content){    

if (nalu_type_checker(mrg->p_ncb->start_curr_n)==1){ //PICTURE CASE 
        
        //SINGLE NALU PACKET
        if (mrg->p_ncb->curr_n_len<=mrg->p_C_inp->rtp_pack_len-13){

            mrg->p_sendbuf=single_nalu_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
            rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];
            if(clock_nanosleep(CLOCK_REALTIME, 0, mrg->p_sleeping_time, NULL)!=0){
            printf("Clock_failure\n");
            exit(EXIT_FAILURE);
            }
            if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
            printf("error with sendto()_rtp\n");
            exit (EXIT_FAILURE);}
            printf("single_picture\n");


            if(mrg->p_ncb->flag==0){
            uploader(mrg->p_ncb->cbc_end,mrg->p_ncb->cbc_begin,mrg->p_C_inp,mrg->p_ncb);}

            if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
                    mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
                    mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 
            mrg->p_rndf->abs_seq_num++;
        }//END CASE SINGLE NALU PACKET

        else{//FRAGMENTATION
        
        mrg->p_ncb->fragm_nalu_pointer=mrg->p_ncb->start_curr_n;                
       
        uint16_t fix_seq=mrg->p_rndf->abs_seq_num;
            if(clock_nanosleep(CLOCK_REALTIME, 0, mrg->p_sleeping_time, NULL)!=0){
            printf("Clock_failure\n");
            exit(EXIT_FAILURE);}
            while(mrg->p_ncb->fragm_nalu_pointer!=mrg->p_ncb->start_next_n){
                mrg->p_sendbuf=fragmentation_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
                rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];
                rtp_hdr->ts=generate_ts_update(mrg->p_C_inp,mrg->p_rndf, fix_seq);
                mrg->p_rtcp_u->rtCp_ts=rtp_hdr->ts;
                if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
                printf("error with sendto()_rtp\n");
                exit (EXIT_FAILURE);}
                printf("fragm_picture\n");
                
            }//end_of_while
         mrg->p_rndf->abs_seq_num++;
        if(mrg->p_ncb->flag_flag!=0){
            mrg->p_ncb->cbc_end=mrg->p_ncb->start_curr_n; 
        }
        mrg->p_ncb->flag_flag=1;
        mrg->p_ncb->cbc_begin=mrg->p_ncb->start_next_n;
        if(mrg->p_ncb->flag==0){
        uploader(mrg->p_ncb->cbc_end,mrg->p_ncb->cbc_begin,mrg->p_C_inp,mrg->p_ncb);}
       if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
                    mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
                    mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 
        mrg->p_ncb->start_curr_n=mrg->p_ncb->start_next_n;
        mrg->p_ncb->start_next_n=next_nalu_searcher(mrg->p_ncb->start_curr_n,mrg->p_ncb);
        if((mrg->p_ncb->start_next_n)!=NULL){
        update_curr_payload_len(mrg->p_ncb,mrg->p_ncb->start_curr_n);
            }
                    
        }//END FRAGMENTATION CASE

}//END PICTURE CASE

else{ //NON-PICTURE,parameter NALUs, 
     if(mrg->p_ncb->curr_n_len>mrg->p_C_inp->rtp_pack_len-13){
        //fragmentation of parameter NALU case
         mrg->p_ncb->fragm_nalu_pointer=mrg->p_ncb->start_curr_n;
            
            uint16_t fix_seq=mrg->p_rndf->abs_seq_num;

                while(mrg->p_ncb->fragm_nalu_pointer!=mrg->p_ncb->start_next_n){
                    
                    mrg->p_sendbuf=fragmentation_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);                  
                    rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];
                    rtp_hdr->ts=generate_ts_update(mrg->p_C_inp,mrg->p_rndf, fix_seq);

                    if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
                    printf("error with sendto()_rtp\n");
                    exit (EXIT_FAILURE);}
                    printf("fragm__non-picture\n");
                  }//end of while
                  if(mrg->p_ncb->flag_flag!=0){
                    mrg->p_ncb->cbc_end=mrg->p_ncb->start_curr_n; 
                  }
                    mrg->p_ncb->flag_flag=1;
                mrg->p_ncb->cbc_begin=mrg->p_ncb->start_next_n;
                  if(mrg->p_ncb->flag==0){
                  uploader(mrg->p_ncb->cbc_end,mrg->p_ncb->cbc_begin,mrg->p_C_inp,mrg->p_ncb);}
                  if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
                    mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
                    mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 
                    mrg->p_ncb->start_curr_n=mrg->p_ncb->start_next_n;
                    mrg->p_ncb->start_next_n=next_nalu_searcher(mrg->p_ncb->start_curr_n,mrg->p_ncb);
                        if((mrg->p_ncb->start_next_n)!=NULL){
                            update_curr_payload_len(mrg->p_ncb,mrg->p_ncb->start_curr_n);
                            }
     }
     else{ //data fits in one package
        if(next_nalu_searcher(mrg->p_ncb->start_next_n,(struct circ_buf*)&mrg->p_ncb)!=NULL
            &&nalu_type_checker(mrg->p_ncb->start_next_n)==0
            &&(16+mrg->p_ncb->curr_n_len+3+(next_nalu_searcher(mrg->p_ncb->start_next_n,mrg->p_ncb)-mrg->p_ncb->start_next_n)-5<mrg->p_C_inp->rtp_pack_len)){
            mrg->p_sendbuf=aggregation_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
            rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];

            if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
            printf("error with sendto()_rtp\n");
            exit (EXIT_FAILURE);}
            printf("agg_non_pict\n");
              if(mrg->p_ncb->flag==0){
             uploader(mrg->p_ncb->cbc_end,mrg->p_ncb->cbc_begin,mrg->p_C_inp,mrg->p_ncb);}
            if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
              mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
              mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 
      

        }

        else{
              mrg->p_sendbuf=single_nalu_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
                
            if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
                printf("error with sendto()_rtp\n");
                exit (EXIT_FAILURE);}
                printf("single_non-pict\n");
                 if(mrg->p_ncb->flag==0){
                uploader(mrg->p_ncb->cbc_end,mrg->p_ncb->cbc_begin,mrg->p_C_inp,mrg->p_ncb);}
            if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
                    mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
                    mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 
        }

     }

}//end of case non-picture NALUs

}//end_of_case_of_middle_of_buffer

//CASE: PROCESSING END OF BUFFER
else if(mrg->p_ncb->start_next_n==NULL&&mrg->p_ncb->start_curr_n!=mrg->p_ncb->end_of_content){

 mrg->p_ncb->fragm_nalu_pointer=mrg->p_ncb->start_curr_n;                
       
        uint16_t fix_seq=mrg->p_rndf->abs_seq_num;
                
            while(mrg->p_ncb->fragm_nalu_pointer!=&(mrg->p_ncb->cbc[mrg->p_ncb->cb_len])+1){
                mrg->p_sendbuf=fragmentation_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
                rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];
                rtp_hdr->ts=generate_ts_update(mrg->p_C_inp,mrg->p_rndf, fix_seq);
                mrg->p_rtcp_u->rtCp_ts=rtp_hdr->ts;
                if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
                printf("error with sendto()_rtp\n");
                exit (EXIT_FAILURE);}  
                printf("fragm_pict_picture_first_half\n");

            }//end_of_while

if(nalu_type_checker(mrg->p_ncb->start_curr_n)==1){
mrg->p_rndf->abs_seq_num++;}

if(mrg->p_ncb->flag==0){
uploader(mrg->p_ncb->start_curr_n,&(mrg->p_ncb->cbc[mrg->p_ncb->cb_len])+1,mrg->p_C_inp,mrg->p_ncb);}


if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 

mrg->p_ncb->start_curr_n=mrg->p_ncb->cbc;
mrg->p_ncb->fragm_nalu_pointer=mrg->p_ncb->cbc;
mrg->p_ncb->start_next_n=next_nalu_searcher(mrg->p_ncb->cbc,mrg->p_ncb);

while(mrg->p_ncb->start_next_n==NULL){
    printf("INSIDE\n");
       while(mrg->p_ncb->fragm_nalu_pointer!=&(mrg->p_ncb->cbc[mrg->p_ncb->cb_len])+1){
                mrg->p_sendbuf=fragmentation_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
                rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];
                rtp_hdr->ts=generate_ts_update(mrg->p_C_inp,mrg->p_rndf, fix_seq);
                mrg->p_rtcp_u->rtCp_ts=rtp_hdr->ts;
                if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
                printf("error with sendto()_rtp\n");
                exit (EXIT_FAILURE);}  
                printf("fragm_picture_big_frame_middle_buf\n");

            }//end_of_while
            
if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 

if(mrg->p_ncb->flag==0){
uploader(mrg->p_ncb->cbc,&(mrg->p_ncb->cbc[mrg->p_ncb->cb_len])+1,mrg->p_C_inp,mrg->p_ncb);}
mrg->p_ncb->fragm_nalu_pointer=mrg->p_ncb->cbc;
mrg->p_ncb->start_next_n=next_nalu_searcher(mrg->p_ncb->cbc,mrg->p_ncb);
}

if((mrg->p_ncb->start_next_n)!=NULL){
update_curr_payload_len(mrg->p_ncb,mrg->p_ncb->cbc);}

 while(mrg->p_ncb->fragm_nalu_pointer!=mrg->p_ncb->start_next_n){
                mrg->p_sendbuf=fragmentation_packet(mrg->p_sendbuf,mrg->p_ncb,mrg->p_rndf,mrg->p_C_inp,mrg->p_rtcp_u);
                rtp_hdr_t *rtp_hdr = (rtp_hdr_t*)&mrg->p_sendbuf[0];
                rtp_hdr->ts=generate_ts_update(mrg->p_C_inp,mrg->p_rndf, fix_seq);
                mrg->p_rtcp_u->rtCp_ts=rtp_hdr->ts;
                if (sendto(mrg->p_D_sock->s_rtp, mrg->p_sendbuf, mrg->p_C_inp->pack_len_var, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtp,mrg->p_D_sock->size_rtp)==-1){
                printf("error with sendto()_rtp\n");
                exit (EXIT_FAILURE);}
                printf("fragm_pict_picture_second_half\n");

            }//end_of_while

            if(mrg->p_ncb->flag==0){
            uploader(mrg->p_ncb->cbc,mrg->p_ncb->start_next_n,mrg->p_C_inp,mrg->p_ncb);}
if(mrg->p_ncb->flag==1){
    exit(EXIT_FAILURE);
}

             if(mrg->p_C_inp->pack_len_var!=mrg->p_C_inp->rtp_pack_len){
                    mrg->p_sendbuf=realloc(mrg->p_sendbuf,mrg->p_C_inp->rtp_pack_len);
                    mrg->p_C_inp->pack_len_var=mrg->p_C_inp->rtp_pack_len;} 
        mrg->p_ncb->start_curr_n=mrg->p_ncb->start_next_n;
        mrg->p_ncb->start_next_n=next_nalu_searcher(mrg->p_ncb->start_curr_n,mrg->p_ncb);;
        if((mrg->p_ncb->start_next_n)!=NULL){
        update_curr_payload_len(mrg->p_ncb,mrg->p_ncb->start_curr_n);
            }

}//END OF CASE: PROCESSING END OF BUFFER

//END_OF_FILE_CASE
if(mrg->p_ncb->start_curr_n==mrg->p_ncb->end_of_content){
    mrg->rctp_u->last_sent=1;
   printf("rtp_sending_cycle_end\n");
    break;

}

}//end of big sending while
 
}//end_of_func

void* rtcp_sending_thread(void *arg){
struct myargs *mrg=arg;
mrg->p_rtcp_sendbuf=rtcp_packet_fill(mrg->p_rtcp_sendbuf,mrg->p_rndf,mrg->p_rtcp_u,0);
if(mrg->p_rtcp_sendbuf!=NULL){
if (sendto(mrg->p_D_sock->s_rtcp, mrg->p_rtcp_sendbuf,mrg->p_rtcp_u->rtcp_buf_len, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtcp, mrg->p_D_sock->size_rtcp)==-1){
printf("error with sendto()_rctp");
exit(EXIT_FAILURE);}
printf("sent_rtcp\n");
}
else{
exit(EXIT_FAILURE);}

while(mrg->rtcp_u->last_sent!=1){
mrg->p_rtcp_sendbuf=rtcp_packet_fill(mrg->p_rtcp_sendbuf,mrg->p_rndf,mrg->p_rtcp_u,0);
if(mrg->p_rtcp_sendbuf!=NULL){
if (sendto(mrg->p_D_sock->s_rtcp, mrg->p_rtcp_sendbuf,mrg->p_rtcp_u->rtcp_buf_len, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtcp, mrg->p_D_sock->size_rtcp)==-1){
printf("error with sendto()_rctp");
exit(EXIT_FAILURE);}
printf("sent_rtcp\n");
}
else{
exit(EXIT_FAILURE);}
if(mrg->rtcp_u->last_sent==1){
    break;
}

}

mrg->p_rtcp_sendbuf=rtcp_packet_fill(mrg->p_rtcp_sendbuf,mrg->p_rndf,mrg->p_rtcp_u,1);
if(rtcp_sendbuf!=NULL){
if (sendto(mrg->p_D_sock->s_rtcp, mrg->p_rtcp_sendbuf,mrg->p_rtcp_u->rtcp_buf_len, 0 , (struct sockaddr *) &mrg->p_D_sock->si_other_rtcp, mrg->p_D_sock->size_rtcp)==-1){
printf("error with sendto()_rctp");
exit(EXIT_FAILURE);}
printf("SENT_BYE_RTCP\n");
}
