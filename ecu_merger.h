#ifndef ECU_MERGER_H
#define ECU_MERGER_H


#define ECU_MERGER_MAX_QUEUE_NUM 1000

struct ecu_merger_msg
{
    unsigned int seq_num;
    unsigned char *p_enc_data;
    unsigned int data_len;
    unsigned char in_use;
};

int ecu_merger_t_start();


#endif
