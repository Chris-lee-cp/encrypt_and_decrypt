// Copyright  2018, junghoon lee(jhoon.chris@gmail.com)
#ifndef ECU_ENC_H
#define ECU_ENC_H

/* Maximum encryption thread number */
#define ECU_ENC_MAX_THREAD_NUM 10


#define ECU_ENC_MAX_QUEUE_NUM 100

struct ecu_enc_msg
{
    unsigned int seq_num;
    unsigned char *p_enc_data;
    unsigned int data_len;
};

struct enc_queue_ipc {
    pthread_mutex_t   lock;
};

/*******************************************************************************
 **
 ** Function        ecu_enc_m_start
 **
 ** Description     init mutex for enc ring buffer
 **
 ** Parameters
 **
 ** Returns         0 is success
 **                 the others are errors
 **
 *******************************************************************************/
int ecu_enc_m_start();


/*******************************************************************************
 **
 ** Function        ecu_enc_t_start
 **
 ** Description     create encryptor thread
 **
 ** Parameters
 **
 ** Returns         0 is success
 **                 the others are errors
 **
 *******************************************************************************/
int ecu_enc_t_start();


/*******************************************************************************
 **
 ** Function        ecu_enc_pop_block
 **
 ** Description     pop encrypted block from encryption buffer
 **
 ** Parameters      none
 **
 ** Returns         pointer to encrypt ring buffer
 **
 *******************************************************************************/
struct ecu_enc_msg * ecu_enc_pop_block();


/*******************************************************************************
 **
 ** Function        ecu_enc_get_block_cnt_in_rb
 **
 ** Description     get number of block in encriptor ring buffer
 **
 ** Parameters      none
 **
 ** Returns         number of block
 **
 *******************************************************************************/
unsigned int ecu_enc_get_block_cnt_in_rb();


#endif
