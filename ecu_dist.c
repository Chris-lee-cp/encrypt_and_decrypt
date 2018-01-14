/*****************************************************************************
**
**  Name:           ecu_dist.c
**
**  Description:    segment input stream and distribute each data block
**                  to multi encryptors
**   Copyright  2018, junghoon lee(jhoon.chris@gmail.com)
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

static pthread_t ecu_dist_tid;

/* Ring Buffer between distributor and encryptor */
struct ecu_dist_msg ECU_DIST_RB[ECU_DIST_MAX_QUEUE_NUM];

/* IPC synchronization method for ECU_DIST_RB */
struct ecu_dist_queue_ipc ECU_DIST_RB_IPC;

/* write and read buffer pointer for ECU_DIST_RB ring buffer */
unsigned int ECU_DIST_RB_wptr = 0;
unsigned int ECU_DIST_RB_rptr = 0;



/* static function definitions */
static unsigned int ecu_dist_get_avail_buf_cnt_in_rb();
static int ecu_dist_send_block(char* data, unsigned int len);



/*******************************************************************************
 **
 ** Function        ecu_dist_thread
 **
 ** Description     Distributor thread function
 **                 Read data from input stream and segment data as block size
 **                 and send each block to ECU_DIST_RB
 **
 ** Parameters
 **
 ** Returns         void
 **
 *******************************************************************************/
void *ecu_dist_thread(void *ptr)
{
    char buf[1];
    unsigned int t_length, i;
    unsigned int block_size;
    unsigned char buffer[1000];
    static int read_err = 0;
    int result = 0;
    int sent = 0;
    int avail_buf_count;

#ifdef DEBUG
    printf("ecu_dist_thread started! \n");
#endif

    /* get block size, key_size x 8 */
    block_size = ecu_get_block_size();

    i = 0;
    t_length = 0;

    while(1)
    {
        /* read input string from STDIO */
        if(read(STDIN_FILENO, buf, 1) > 0)
        {
            read_err = 0;
            buffer[i] = buf[0];
            if( i == ( block_size - 1 ) )
            {
                sent = 0;
                while(!sent)
                {
                    pthread_mutex_lock(&ECU_DIST_RB_IPC.lock);
                    /* get available buffer count of DIST ring buffer */
                    avail_buf_count = ecu_dist_get_avail_buf_cnt_in_rb();
                    if (avail_buf_count > 3)
                    {
                        /* send one block to dist queue */
                        result = ecu_dist_send_block(buffer, block_size);
                        sent = 1;
                    }
                    pthread_mutex_unlock(&ECU_DIST_RB_IPC.lock);
                    if ( result < 0 )
                    {
                        printf("Critical error! cannot be happened!");
                        exit(1);
                    }
                }
                i = 0;
            }
            else
            {
                i++;
            }
            t_length++;
        }
        else
        {
            read_err++;
            /* if read() returns error 5 continuous time, stop read */
            if (read_err > 5)
            {
                break;
            }
        }
    }

    /* input stream size should be multiple of block size.
     * however, handles reminder of data if we have.
     * for the unexpected case.
     */
    if( i )
    {
        sent = 0;
        while(!sent)
        {
            pthread_mutex_lock(&ECU_DIST_RB_IPC.lock);
            avail_buf_count = ecu_dist_get_avail_buf_cnt_in_rb();
            if (avail_buf_count > 1)
            {
                result = ecu_dist_send_block(buffer, i);
                sent = 1;
            }
            pthread_mutex_unlock(&ECU_DIST_RB_IPC.lock);
        }
    }

    /* configure total length of input stream */
    ecu_set_instr_length(t_length);

#ifdef DEBUG
    printf("input size is %d\n", t_length);
#endif
}


/*******************************************************************************
 **
 ** Function        ecu_dist_m_start
 **
 ** Description     init Mutex for Distributor ring buffer access.
 **
 ** Parameters
 **
 ** Returns         0 is success
 **                 the others are error
 **
 *******************************************************************************/
int ecu_dist_m_start()
{
    int result;
#ifdef DEBUG
    printf("ecu_dist_m_start\n");
#endif

    result = pthread_mutex_init(&ECU_DIST_RB_IPC.lock, NULL);
    if (result != 0)
    {
        printf("pthread_mutex_init error %d\n", result);
    }

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_dist_t_start
 **
 ** Description     Create Distributor thread.
 **
 ** Parameters
 **
 ** Returns         0 is success
 **                 the others are error
 **
 *******************************************************************************/
int ecu_dist_t_start()
{
    int result;
#ifdef DEBUG
    printf("ecu_dist_t_start\n");
#endif
    /* create thread */
    result = pthread_create(&ecu_dist_tid, NULL, ecu_dist_thread, NULL);
    if(result)
    {
        printf("pthread_create error!!");
    }

    return result;
}



/*******************************************************************************
 **
 ** Function        ecu_dist_pop_block
 **
 ** Description     pop one block from DIST ring buffer
 **
 ** Parameters      none
 **
 ** Returns         pointer to one attained block from ring buffer
 **
 *******************************************************************************/
struct ecu_dist_msg* ecu_dist_pop_block()
{
    struct ecu_dist_msg *msg = NULL;

    if (ECU_DIST_RB_rptr == ECU_DIST_RB_wptr)
    {
        /* no data in circular queue */
        printf("error! no data in DIST circular queue !!\n");
    }
    else
    {
        msg = malloc(sizeof(struct ecu_dist_msg));

        msg->data_len = ECU_DIST_RB[ECU_DIST_RB_rptr].data_len;
        msg->key_len = ECU_DIST_RB[ECU_DIST_RB_rptr].key_len;
        msg->p_data = ECU_DIST_RB[ECU_DIST_RB_rptr].p_data;
        msg->p_key = ECU_DIST_RB[ECU_DIST_RB_rptr].p_key;
        msg->seq_num = ECU_DIST_RB[ECU_DIST_RB_rptr].seq_num;

#ifdef DEBUG
        printf("pop seq:%d\n", msg->seq_num);
#endif
        ECU_DIST_RB_rptr++;

        if (ECU_DIST_RB_rptr == ECU_DIST_MAX_QUEUE_NUM)
        {
            ECU_DIST_RB_rptr = 0;
        }
    }

    return msg;
}


/*******************************************************************************
 **
 ** Function        ecu_dist_get_block_cnt_in_rb
 **
 ** Description     Get number of block in DIST ring buffer
 **
 ** Parameters      none
 **
 ** Returns         number of blocks
 **
 *******************************************************************************/
unsigned int ecu_dist_get_block_cnt_in_rb()
{
    unsigned int result = 0;

    if(ECU_DIST_RB_wptr > ECU_DIST_RB_rptr)
    {
        result = ECU_DIST_RB_wptr - ECU_DIST_RB_rptr;
    }
    if(ECU_DIST_RB_wptr < ECU_DIST_RB_rptr)
    {
        result = (ECU_DIST_RB_wptr + ECU_DIST_MAX_QUEUE_NUM) - ECU_DIST_RB_rptr;
    }

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_dist_send_block
 **
 ** Description     send one block to DIST ring buffer
 **
 ** Parameters      data : pointer to a block
 **                 len : length of a block
 **
 ** Returns         0 is success
 **                 -1 is error
 **
 *******************************************************************************/
static int ecu_dist_send_block(char* data, unsigned int len)
{
    struct ecu_dist_msg *msg;
    static unsigned int seq_num = 0;

    msg = &ECU_DIST_RB[ECU_DIST_RB_wptr];

    msg->data_len = len;
    msg->key_len = ecu_get_key_size();
    msg->p_data = malloc(msg->data_len);
    msg->p_key = malloc(msg->key_len);
    msg->seq_num = seq_num++;
    memcpy(msg->p_data, data, len);
    ecu_copy_key(msg->p_key);

    ECU_DIST_RB_wptr++;

    if (ECU_DIST_RB_wptr == ECU_DIST_MAX_QUEUE_NUM)
    {
        ECU_DIST_RB_wptr = 0;
    }
    if (ECU_DIST_RB_wptr == ECU_DIST_RB_rptr)
    {
        /* ring buffer overflow */
        printf("error! dist ring buffer full!!\n");
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        ecu_dist_get_avail_buf_cnt_in_rb
 **
 ** Description     Get available buffer count from DIST ring buffer
 **
 ** Parameters      none
 **
 ** Returns         number of available buffer
 **
 *******************************************************************************/
static unsigned int ecu_dist_get_avail_buf_cnt_in_rb()
{
    unsigned int result = 0;

    /* queue is empty */
    if(ECU_DIST_RB_wptr == ECU_DIST_RB_rptr)
    {
        result = ECU_DIST_MAX_QUEUE_NUM;
    }
    else if(ECU_DIST_RB_wptr > ECU_DIST_RB_rptr)
    {
        result = ECU_DIST_MAX_QUEUE_NUM - (ECU_DIST_RB_wptr - ECU_DIST_RB_rptr);
    }
    else if(ECU_DIST_RB_wptr < ECU_DIST_RB_rptr)
    {
        result = ECU_DIST_RB_rptr - ECU_DIST_RB_wptr;
    }

    return result;
}
