/*****************************************************************************
**
**  Name:           ecu_main.c
**
**  Description:    Encryption Utility Main
**
** This utility has three parts
** - distributor (ecu_dist.c) : read hex data from stdin,
**                              segment data, send data to encryptor
** - encryptor (ecu_enc.c) : encrypt data with key value,
**                          bit shift key value
** - merger (ecu_merger.c) : reassemble encrypted data with seq number
**                           manage reorder buffer.
**
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "ecu_main.h"
#include "ecu_dist.h"
#include "ecu_enc.h"
#include "ecu_merger.h"


/* static function definitions */
static int ecu_init();
static void ecu_help();
static int ecu_set_key_size(unsigned int size);
static unsigned int ecu_set_block_size(unsigned int size);
static void ecu_display_key();
static int ecu_set_num_of_enc_thread(unsigned int num);


/* Encryption utility control block */
struct encrypt_util_cb ENC_CB;



/*******************************************************************************
 **
 ** Function        main
 **
 ** Description     This is the main function
 **
 ** Parameters      Program's arguments
 **
 ** Returns         status
 **
 *******************************************************************************/
int main(int argc, char **argv)
{
    FILE *fp;
    int num_of_thread;
    int result;
    unsigned int key_size;

    if (argc != 5)
    {
        printf("error input parameter!\n");
        ecu_help();
        return -1;
    }

    /* init control block */
    ecu_init();


    /* process input parameters */
    if(strcmp(argv[1], "-n") == 0)
    {
        num_of_thread = atoi(argv[2]);
#ifdef DEBUG
        printf("create thread %d\n", num_of_thread);
#endif

        result = ecu_set_num_of_enc_thread(num_of_thread);
        if( result < 0 )
        {
            return -1;
        }
    }

    if(strcmp(argv[3], "-k") == 0)
    {
#ifdef DEBUG
        printf("key file name is %s\n", argv[4]);
#endif
        fp = fopen(argv[4], "r");
        fseek(fp, 0, SEEK_END);
        key_size = ftell(fp);
        result = ecu_set_key_size(key_size);
        if(result < 0)
        {
            return -1;
        }
        fseek(fp, 0, SEEK_SET);
        fread(ENC_CB.key, key_size, 1, fp);

        ecu_display_key();
        ecu_set_block_size(key_size*8);
    }

    /* init mutex for distributor */
    result = ecu_dist_m_start();
    if( result )
    {
        printf("ecu_dist_m_start error %d\n", result);
        return -1;
    }

    /* init mutex for encryptor */
    result = ecu_enc_m_start();
    if( result )
    {
        printf("ecu_enc_m_start error %d\n", result);
        return -1;
    }

    /* start merger thread */
    result = ecu_merger_t_start();
    if( result )
    {
        printf("ecu_merger_t_start error %d\n", result);
        return -1;
    }

    /* start encryptor threads */
    result = ecu_enc_t_start();
    if( result )
    {
        printf("ecu_enc_t_start error %d\n", result);
        return -1;
    }

    /* start distributor thread */
    result = ecu_dist_t_start();
    if( result )
    {
        printf("ecu_dist_t_start error %d\n", result);
        return -1;
    }

    while(1)
    {
#ifdef DEBUG
        printf("main while loop!\n");
#endif
        sleep(1);
    }

    return 0;
}




/*******************************************************************************
 **
 ** Function        ecu_get_key_size
 **
 ** Description     Get XOR cryptographic key value
 **
 ** Parameters      None
 **
 ** Returns         size of key
 **
 *******************************************************************************/
unsigned int ecu_get_key_size()
{
    unsigned int result;

    result = ENC_CB.key_size;

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_copy_key
 **
 ** Description     Copy XOR cryptographic key value to input pointer
 **
 ** Parameters      destination pointer
 **
 ** Returns         none
 **
 *******************************************************************************/
void ecu_copy_key(unsigned char *dest)
{
    memcpy(dest, ENC_CB.key, ENC_CB.key_size);
}



/*******************************************************************************
 **
 ** Function        ecu_get_block_size
 **
 ** Description     Get size of data block for encryptor.
 **
 ** Parameters      none
 **
 ** Returns         size of block
 **
 *******************************************************************************/
unsigned int ecu_get_block_size()
{
    unsigned int result;

    result = ENC_CB.block_size;

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_set_instr_length
 **
 ** Description     Set total size of input data
 **
 ** Parameters      size of input data
 **
 ** Returns         0 is success
 **
 *******************************************************************************/
unsigned int ecu_set_instr_length(unsigned int len)
{
    ENC_CB.instr_length = len;

    return 0;
}


/*******************************************************************************
 **
 ** Function        ecu_get_instr_length
 **
 ** Description     Get total size of input data
 **
 ** Parameters      none
 **
 ** Returns         size of input data
 **
 *******************************************************************************/
unsigned int ecu_get_instr_length()
{
    unsigned int result;

    result = ENC_CB.instr_length;

    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_get_num_of_enc_thread
 **
 ** Description     Get number of threads for encryption
 **
 ** Parameters      none
 **
 ** Returns         number of thread
 **
 *******************************************************************************/
unsigned int ecu_get_num_of_enc_thread()
{
    unsigned int result;
    result = ENC_CB.num_of_enc_thread;
    return result;
}


/*******************************************************************************
 **
 ** Function        ecu_init
 **
 ** Description     Initialize Encryptor util Control Block
 **
 ** Parameters      none
 **
 ** Returns         status
 **
 *******************************************************************************/
static int ecu_init()
{
#ifdef DEBUG
    printf("ecu_init!\n");
#endif
    memset(&ENC_CB, sizeof(struct encrypt_util_cb), 0);

    return 0;
}


/*******************************************************************************
 **
 ** Function        ecu_help
 **
 ** Description     Display how to use encryptUtil
 **
 ** Parameters      none
 **
 ** Returns         none
 **
 *******************************************************************************/
static void ecu_help()
{
    printf(" encryptUtil version 2.0\n");
    printf("usage\n");
    printf(" encryptUtil [-n #] [-k keyfile]\n");
    printf(" -n # Number of threads to create. 10 is maximum\n");
    printf(" -k keyfile Path to file containing key\n");
}


/*******************************************************************************
 **
 ** Function        ecu_set_key_size
 **
 ** Description     Set XOR cryptographic key value
 **
 ** Parameters      size of key
 **
 ** Returns         0 is okay, -1 is error.
 **
 *******************************************************************************/
static int ecu_set_key_size(unsigned int size)
{
    if(size > ECU_KEY_MAX)
    {
        printf("Key size is too big! %d\n", size);
        return -1;
    }
#ifdef DEBUG
    printf("Set Key size %d\n", size);
#endif
    ENC_CB.key_size = size;
    return 0;
}


/*******************************************************************************
 **
 ** Function        ecu_display_key
 **
 ** Description     print key value for debugging
 **
 ** Parameters      none
 **
 ** Returns         none
 **
 *******************************************************************************/
static void ecu_display_key()
{
#ifdef DEBUG
    unsigned int i;

    printf("\n");
    for(i=0;i<ENC_CB.key_size;i++)
    {
        printf("%02X ", ENC_CB.key[i]);
    }
    printf("\n");
#endif
}


/*******************************************************************************
 **
 ** Function        ecu_set_num_of_enc_thread
 **
 ** Description     Set number of threads for encryption
 **
 ** Parameters      number of thread
 **
 ** Returns         0 is success
 **                 -1 is error
 **
 *******************************************************************************/
static int ecu_set_num_of_enc_thread(unsigned int num)
{
    if (num > ECU_ENC_MAX_THREAD_NUM)
    {
        printf("error thread number:%d\n", num);
        return -1;
    }
    ENC_CB.num_of_enc_thread = num;

    return 0;
}


/*******************************************************************************
 **
 ** Function        ecu_set_block_size
 **
 ** Description     Set size of data block for encryptor.
 **
 ** Parameters      size of block
 **
 ** Returns         0 is success
 **
 *******************************************************************************/
static unsigned int ecu_set_block_size(unsigned int size)
{
#ifdef DEBUG
    printf("Set block size %d\n", size);
#endif
    ENC_CB.block_size = size;

    return 0;
}
