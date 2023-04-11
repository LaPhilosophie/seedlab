#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <openssl/aes.h>

using namespace std;

#define LEN 32 // 256 bits

int main()
{
	unsigned char* key = (unsigned char * ) malloc(sizeof(unsigned char) * LEN);
	FILE * random = fopen("/dev/urandom", "r");
	fread(key, sizeof(unsigned char) * LEN, 1, random);
	fclose(random);
	for(int i=0;i<LEN;i++)
	{
		printf("%.2x",key[i]);
	}
	cout<<endl;
    return 0;
}
