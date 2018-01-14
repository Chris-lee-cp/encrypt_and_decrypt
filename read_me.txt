**** what is encryptUtil *****
will perform a simple XOR cryptographic transform on a given set of data - an XOR stream encryptor. 

This utility has three main thread groups
1. distributor
2. encryptors(multi threads, up to 10)
3. merger

and three main buffer
1. distributor ring buffer (between distributor and encryptor)
2. encryptor ring buffer (between encryptor and merger)
3. re-order buffer (merger thread)




***** How to build ******
> make clean 
> make



***** How to execute ******
> cat test_file2 | ./encryptUtil -n 7 -k keyfile > test_file2_1



***** How to verify ******
> cat test_file2 | ./encryptUtil -n 7 -k keyfile > test_file2_1
> cat test_file2_1 | ./encryptUtil -n 7 -k keyfile > test_file2_2
> diff test_file2 test_file2_2

test_file2 and test_file2_2 should be the same. 
VERIFIED!!


**** Required command-line options ****
encryptUtil [-n #] [-k keyfile]
-n # Number of threads to create
-k keyfile Path to file containing key


