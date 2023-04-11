#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <openssl/aes.h>
using namespace std;
#define KEYSIZE 16
int main()
{
	int i,j;
	unsigned char encryptText[17];
	const unsigned char plainText[17] = "\x25\x50\x44\x46\x2d\x31\x2e\x35\x0a\x25\xd0\xd4\xc5\xd8\x0a\x34";
	const unsigned char target_str[17] = "\xd0\x6b\xf9\xd0\xda\xb8\xe8\xef\x88\x06\x60\xd2\xaf\x65\xaa\x82";
	int target_time = 1524020929;
	for (i = target_time;i >=target_time - 60*60*2 ; i--)
	{
		srand(i);
		unsigned char key[KEYSIZE];
		unsigned char ini_vec[17] = 		 "\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00\xA2\xB2\xC2\xD2\xE2\xF2";
		for (j = 0; j < KEYSIZE; j++) 
		{
			key[j] = rand() % 256;
			//printf("%.2x", (unsigned char)key[j]);
		}
		AES_KEY enc_key;
		AES_set_encrypt_key(key, 128 , &enc_key);
		AES_cbc_encrypt(plainText, encryptText, 16 , &enc_key, ini_vec, AES_ENCRYPT);     // 加密
		
		int flag=1;
		for(j=0;j<16;j++)
		{
			if(encryptText[j]!=target_str[j])
			{
				flag=0;
				break;
			}
		}
		if(flag)
		{
			cout<<"the key is : ";
			for(j=0;j<16;j++)
			{
				printf("%.2x", (unsigned char)key[j]);
			}
			cout<<endl;
			//return 0;
		}
		
	}
    return 0;
}