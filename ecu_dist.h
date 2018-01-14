// Copyright  2018, junghoon lee(jhoon.chris@gmail.com)

#ifndef ECU_DIST_H
#define ECU_DIST_H

/* Maximum Distributor ring buffer size */
#define ECU_DIST_MAX_QUEUE_NUM 100

/* data structure between distributor and encryptor */
struct ecu_dist_msg
{
    unsigned int seq_num;  /* sequence number of block */
    unsigned char *p_data;
    unsigned int data_len;
    unsigned char *p_key;
    unsigned int key_len;
};

/* synchronization method for DIST ring buffer */
struct ecu_dist_queue_ipc {
    pthread_mutex_t   lock;
};

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
void *ecu_dist_thread(void *ptr);


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
int ecu_dist_m_start();


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
int ecu_dist_t_start();


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
struct ecu_dist_msg* ecu_dist_pop_block();


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
unsigned int ecu_dist_get_block_cnt_in_rb();

#endif
