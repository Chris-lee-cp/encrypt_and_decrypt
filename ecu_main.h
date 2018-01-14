/*****************************************************************************
**
**  Name:           ecu_main.h
**
**  Description:    Encryption Utility Main header file
**
**  Copyright  2018, junghoon lee(jhoon.chris@gmail.com)
*****************************************************************************/
#ifndef ECU_MAIN_H
#define ECU_MAIN_H

/* Maximum encryption key size(byte) */
#define ECU_KEY_MAX 100


/* Control block for Encryption utility */
struct encrypt_util_cb {
    unsigned int instr_length;
    unsigned int key_size;
    unsigned int block_size;
    unsigned int num_of_enc_thread;
    unsigned char key[ECU_KEY_MAX];
};


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
int main(int argc, char **argv);


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
unsigned int ecu_get_key_size();


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
void ecu_copy_key(unsigned char *dest);


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
unsigned int ecu_get_block_size();


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
unsigned int ecu_set_instr_length(unsigned int len);


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
unsigned int ecu_get_instr_length();


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
unsigned int ecu_get_num_of_enc_thread();


#endif
