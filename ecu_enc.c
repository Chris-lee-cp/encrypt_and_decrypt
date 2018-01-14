/*****************************************************************************
**
**  Name:           ecu_enc.c
**
**  Description:    encrypt each block and send encrypted block
**                  to encrypt ring buffer
**
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "ecu_main.h"
#include "ecu_dist.h"
#include "ecu_enc.h"
#include "ecu_merger.h"

/* pthread ids for N encrypt threads */
static pthread_t ecu_enc_tid[ECU_ENC_MAX_THREAD_NUM];

/* ENC ring buffer synchronization method */
struct enc_queue_ipc ECU_ENC_RB_IPC;

/* Encrytor ring buffer */
struct ecu_enc_msg ECU_ENC_RB[ECU_ENC_MAX_QUEUE_NUM];

/* read and write pointer for ENC ring buffer */
unsigned int ECU_ENC_RB_wptr = 0;
unsigned int ECU_ENC_RB_rptr = 0;

extern struct encrypt_util_cb ENC_CB;
extern struct ecu_dist_queue_ipc ECU_DIST_RB_IPC;


/* static function definitions */
static void *ecu_enc_thread(void *ptr);
static int ecu_enc_execute(struct ecu_dist_msg *dist_msg);
static unsigned int ecu_enc_get_avail_buf_cnt_in_rb();
static int ecu_enc_push_block(unsigned char* data, unsigned int len, unsigned int seq);


/*******************************************************************************
 **
 ** Function        ecu_enc_thread
 **
 ** Description     encryptor thread function
 **                 do XOR encription
 **
 ** Parameters
 **
 ** Returns         void
 **
 *******************************************************************************/
static void *ecu_enc_thread(void *ptr)
{
    struct ecu_dist_msg *dist_msg;
    unsigned int q_cnt;
    int result = 0;

    while(1)
    {
        pthread_mutex_lock(&ECU_DIST_RB_IPC.lock);
        q_cnt = ecu_dist_get_block_cnt_in_rb();
        if(q_cnt > 0)
        {
#ifdef DEBUG
            printf("q_cnt:%d\n",q_cnt);
#endif
            dist_msg = ecu_dist_pop_block();
            pthread_mutex_unlock(&ECU_DIST_RB_IPC.lock);
            if(dist_msg)
            {
#ifdef DEBUG
                printf("encrypt \n");
#endif
                /* do decryption!!! */
                result = ecu_enc_execute(dist_msg);
                if (result < 0)
                {
                    sleep(1);
                }
            }
            else
            {
                printf(" critical error!!\n");
                exit(0);
            }
        }
        else
        {
            pthread_mutex_unlock(&ECU_DIST_RB_IPC.lock);
        }
    }
}


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
int ecu_enc_m_start()
{
    int result;
#ifdef DEBUG
    printf("ecu_enc_m_start\n");
#endif

    result = pthread_mutex_init(&ECU_ENC_RB_IPC.lock, NULL);
    if (result != 0)
    {
        printf("pthread_mutex_init error %d\n", result);
    }

    return result;
}


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
int ecu_enc_t_start()
{
    unsigned int num_of_thread = 0;
    unsigned int i;
    int result;
#ifdef DEBUG
    printf("ecu_enc_t_start\n");
#endif

    num_of_thread = ecu_get_num_of_enc_thread();

    for(i=0;i<num_of_thread;i++)
    {
#ifdef DEBUG
	    printf("ecu_enc thread %d\n", i);
#endif
        /* create thread */
        result = pthread_create(&ecu_enc_tid[i], NULL, ecu_enc_thread, NULL);
        if(result)
        {
            printf("pthread_create error!!");
            break;
        }
    }

    return result;
}


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
struct ecu_enc_msg * ecu_enc_pop_block()
{
    struct ecu_enc_msg *msg = NULL;

    if (ECU_ENC_RB_rptr == ECU_ENC_RB_wptr)
    {
        /* no data in circular queue */
#ifdef DEBUG
        printf("error! no data in ENC circular queue !!\n");
#endif
    }
    else
    {
        msg = malloc(sizeof(struct ecu_enc_msg));

        msg->data_len = ECU_ENC_RB[ECU_ENC_RB_rptr].data_len;
        msg->p_enc_data = malloc(msg->data_len);
        msg->seq_num = ECU_ENC_RB[ECU_ENC_RB_rptr].seq_num;
        memcpy(msg->p_enc_data, ECU_ENC_RB[ECU_ENC_RB_rptr].p_enc_data, msg->data_len);

        free(ECU_ENC_RB[ECU_ENC_RB_rptr].p_enc_data);

        ECU_ENC_RB_rptr++;

        if (ECU_ENC_RB_rptr == ECU_ENC_MAX_QUEUE_NUM)
        {
            ECU_ENC_RB_rptr = 0;
        }
    }

    return msg;
}


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
unsigned int ecu_enc_get_block_cnt_in_rb()
{
    unsigned int result = 0;

    if(ECU_ENC_RB_wptr > ECU_ENC_RB_rptr)
    {
        result = ECU_ENC_RB_wptr - ECU_ENC_RB_rptr;
    }
    if(ECU_ENC_RB_wptr < ECU_ENC_RB_rptr)
    {
        result = (ECU_ENC_RB_wptr + ECU_ENC_MAX_QUEUE_NUM) - ECU_ENC_RB_rptr;
    }

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_enc_execute
 **
 ** Description     execute encryption and shift key
 **
 ** Parameters
 **
 ** Returns         0 is success
 **                 the others are errors
 **
 *******************************************************************************/
static int ecu_enc_execute(struct ecu_dist_msg *dist_msg)
{
    unsigned int i, j;
    unsigned char *data;
    unsigned char *key;
    unsigned char *enc_data;
    unsigned int temp = 0;
    int result = 0;
    int sent = 0;
    unsigned int enc_buf_avail;

    enc_data = malloc(dist_msg->data_len);
    data = dist_msg->p_data;
    key = dist_msg->p_key;


    for( i = 0 ; i < dist_msg->data_len ; i = i + dist_msg->key_len )
    {
        for( j = 0; j < dist_msg->key_len ; j++)
        {
            enc_data[i+j] = (data[i+j] ^ key[j]) & 0xFF;
#ifdef DEBUG
            printf("%02x %02x %02x\n", enc_data[i+j], data[i+j], key[j]);
#endif
        }

        temp = 0;
        /* 1-bit shift left key value */
        for( j = 0 ; j < dist_msg->key_len ; j++)
        {
            temp = (key[j] << 1) | ((temp>>8)&1);
            key[j] = (unsigned char)(temp & 0xFF);
            if (j == dist_msg->key_len - 1)
            {
                key[0] = (unsigned char)((key[0]|(temp>>8)&1) & 0xFF);
            }
        }
#ifdef DEBUG
        for( j= 0 ; j < dist_msg->key_len ; j++)
        {
            /* verify key */
            printf("key:%d:%hhx\n", j, key[j] );
        }
#endif
    }

    sent = 0;
    while(!sent)
    {
        pthread_mutex_lock(&ECU_ENC_RB_IPC.lock);
        /* check enc buffer is available */
        enc_buf_avail = ecu_enc_get_avail_buf_cnt_in_rb();
        if (enc_buf_avail > 3)
        {
            /* push data to ECU_ENC_RB */
            result = ecu_enc_push_block(enc_data, dist_msg->data_len, dist_msg->seq_num);
            sent = 1;
        }
        else
        {
#ifdef DEBUG
            printf("enc buffer full!\n");
#endif
        }
        pthread_mutex_unlock(&ECU_ENC_RB_IPC.lock);
    }
    free(data);
    free(key);

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_enc_get_avail_buf_cnt_in_rb
 **
 ** Description     get available buffer count from ENC ring buffer
 **
 ** Parameters
 **
 ** Returns         number of available buffers
 **
 *******************************************************************************/
static unsigned int ecu_enc_get_avail_buf_cnt_in_rb()
{
    unsigned int result = 0;

    if (ECU_ENC_RB_wptr == ECU_ENC_RB_rptr)
    {
        result = ECU_ENC_MAX_QUEUE_NUM;
    }
    else if(ECU_ENC_RB_wptr > ECU_ENC_RB_rptr)
    {
        result = ECU_ENC_MAX_QUEUE_NUM - (ECU_ENC_RB_wptr - ECU_ENC_RB_rptr);
    }
    else if(ECU_ENC_RB_wptr < ECU_ENC_RB_rptr)
    {
        result = ECU_ENC_RB_rptr - ECU_ENC_RB_wptr;
    }

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_enc_push_block
 **
 ** Description     send encrypted block to encryption buffer
 **
 ** Parameters      data : encrypted buffer pointer
 **                 len : length of encrypted data
 **                 seq : sequence number
 **
 ** Returns         number of available buffers
 **
 *******************************************************************************/
static int ecu_enc_push_block(unsigned char* data, unsigned int len, unsigned int seq)
{
    struct ecu_enc_msg *msg;

#ifdef DEBUG
    printf("ecu_enc_push_block! %d\n", seq);
#endif
    msg = &ECU_ENC_RB[ECU_ENC_RB_wptr];

    msg->data_len = len;
    msg->p_enc_data = data;
    msg->seq_num = seq;

    ECU_ENC_RB_wptr++;

    if (ECU_ENC_RB_wptr == ECU_ENC_MAX_QUEUE_NUM)
    {
        ECU_ENC_RB_wptr = 0;
    }
    if (ECU_ENC_RB_wptr == ECU_ENC_RB_rptr)
    {
        /* ring buffer overflow */
#ifdef DEBUG
        printf("error! ENC ring buffer full!!\n");
#endif
        return -1;
    }

    return 0;
}


