/*****************************************************************************
**
**  Name:           ecu_merger.c
**
**  Description:    reorder encrypted block from encryption buffers.
**                  print out to stdio
**  Copyright  2018, junghoon lee(jhoon.chris@gmail.com)
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

static pthread_t ecu_merger_tid;

/* reorder queue to print out block as sequence number order */
struct ecu_merger_msg ECU_MERGER_REODER_Q[ECU_MERGER_MAX_QUEUE_NUM];

extern struct enc_queue_ipc ECU_ENC_RB_IPC;


/* define static functions */
static void *ecu_merger_thread(void *ptr);
static int ecu_merger_execute(struct ecu_enc_msg *enc_msg);
static int ecu_merger_search_reorder_buffer(unsigned int seq_num);
static int ecu_merger_save_msg_reorder_buffer(struct ecu_enc_msg *enc_msg);




static unsigned int seq_out = 0; /* expected sequence number */
static unsigned int rcv_cnt = 0; /* received block counter */



/*******************************************************************************
 **
 ** Function        ecu_merger_thread
 **
 ** Description     merge thread function
 **                 reorder encrypted block and print out to stdio
 **
 ** Parameters
 **
 ** Returns         void
 **
 *******************************************************************************/
static void *ecu_merger_thread(void *ptr)
{
    struct ecu_enc_msg *enc_msg;
    unsigned int q_cnt;

    while(1)
    {
        pthread_mutex_lock(&ECU_ENC_RB_IPC.lock);
        q_cnt = ecu_enc_get_block_cnt_in_rb();
#ifdef DEBUG
        printf("merger q_cnt:%d\n", q_cnt);
#endif
        if(q_cnt > 0)
        {
#ifdef DEBUG
            printf("merger q_cnt:%d\n", q_cnt);
#endif
            enc_msg = ecu_enc_pop_block();
            pthread_mutex_unlock(&ECU_ENC_RB_IPC.lock);

            if(enc_msg)
            {
                /* check seq number and print out to stdout */
                ecu_merger_execute(enc_msg);
            }
        }
        else
        {
            pthread_mutex_unlock(&ECU_ENC_RB_IPC.lock);
            usleep(1000);
        }
    }
}


/*******************************************************************************
 **
 ** Function        ecu_merger_t_start
 **
 ** Description     create merger thread
 **
 ** Parameters
 **
 ** Returns         0 is success
 **                 the others are errors
 **
 *******************************************************************************/
int ecu_merger_t_start()
{
    int result;
#ifdef DEBUG
    printf("ecu_merger_t_start\n");
#endif

    memset(ECU_MERGER_REODER_Q, sizeof(struct ecu_merger_msg)*ECU_MERGER_MAX_QUEUE_NUM, 0);
    result = pthread_create(&ecu_merger_tid, NULL, ecu_merger_thread, NULL);
    if(result)
    {
        printf("pthread_create error!!");
    }

    return result;
}



/*******************************************************************************
 **
 ** Function        ecu_merger_execute
 **
 ** Description     check seq number
 **                 if seq is corrct, print
 **                 if not, save to reorder buffer
 **
 ** Parameters      encrypted block
 **
 ** Returns         0 is success
 **
 *******************************************************************************/
static int ecu_merger_execute(struct ecu_enc_msg *enc_msg)
{
    unsigned int i;
    unsigned char *data;
    unsigned int total_size, total_in_size;
    unsigned int block_size;

    data = enc_msg->p_enc_data;

#ifdef DEBUG
    printf("seq : 0x%x\n", enc_msg->seq_num);
#endif
    /* compare seqeunce number */
    if (enc_msg->seq_num == seq_out) /* if seq num is correct, print stdout */
    {
        for( i=0 ; i < enc_msg->data_len ; i++ )
        {
            printf("%c", data[i]);
        }
        free(data);
        seq_out++;
    }
    else
    {
        /* save disordered msg to reorder buffer */
        ecu_merger_save_msg_reorder_buffer(enc_msg);
        /* search reorder buffer */
        ecu_merger_search_reorder_buffer(seq_out);
    }

    rcv_cnt++;
    total_in_size = ecu_get_instr_length();

#ifdef DEBUG
    printf("total_size : %d\n", total_size);
#endif

    if( total_in_size > 0 )
    {
        block_size = ecu_get_block_size();
        total_size = rcv_cnt * block_size;

        /* if we have whole data from distributor, exit program */
        if( (total_size == total_in_size) ||
            (total_size + block_size > total_in_size) )
        {
            /* if we have reminder in reorder buffer, print all out */
            while( seq_out < rcv_cnt )
            {
                ecu_merger_search_reorder_buffer(seq_out);
            }
            exit(1);
        }
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function        ecu_merger_search_reorder_buffer
 **
 ** Description     search block having seq number
 **
 ** Parameters      sequence number to find
 **
 ** Returns         0 is success
 **                 -1 is error
 **
 *******************************************************************************/
static int ecu_merger_search_reorder_buffer(unsigned int seq_num)
{
    unsigned int i, j;
    unsigned char *data;

    for (i=0;i<ECU_MERGER_MAX_QUEUE_NUM;i++)
    {
        if((ECU_MERGER_REODER_Q[i].in_use == 1) && (ECU_MERGER_REODER_Q[i].seq_num == seq_num))
        {
            data = ECU_MERGER_REODER_Q[i].p_enc_data;
            for (j = 0;j<ECU_MERGER_REODER_Q[i].data_len;j++)
            {
                printf("%c", data[j]);
            }
            ECU_MERGER_REODER_Q[i].in_use = 0;
            free(ECU_MERGER_REODER_Q[i].p_enc_data);
            seq_out++;

            return 0;
        }
    }
    return -1;
}


/*******************************************************************************
 **
 ** Function        ecu_merger_save_msg_reorder_buffer
 **
 ** Description     save a block to reorder buffer
 **
 ** Parameters      block pointer
 **
 ** Returns         0 is success
 **                 -1 is error
 **
 *******************************************************************************/
static int ecu_merger_save_msg_reorder_buffer(struct ecu_enc_msg *enc_msg)
{
    unsigned int i;

    for (i=0;i<ECU_MERGER_MAX_QUEUE_NUM;i++)
    {
        if(ECU_MERGER_REODER_Q[i].in_use == 0)
        {
            ECU_MERGER_REODER_Q[i].in_use = 1;
            ECU_MERGER_REODER_Q[i].data_len = enc_msg->data_len;
            ECU_MERGER_REODER_Q[i].seq_num = enc_msg->seq_num;
            ECU_MERGER_REODER_Q[i].p_enc_data = malloc(ECU_MERGER_REODER_Q[i].data_len);
            memcpy(ECU_MERGER_REODER_Q[i].p_enc_data, enc_msg->p_enc_data, ECU_MERGER_REODER_Q[i].data_len);
            free(enc_msg->p_enc_data);
            return 0;
        }
    }
    if (i == ECU_MERGER_MAX_QUEUE_NUM)
    {
        /* reorder buffer overflowed */
        printf("reorder buffer overflowed!!\n");
        exit(1);
        return -1;
    }
    return 0;
}
