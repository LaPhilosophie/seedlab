#include <stdio.h>
#include <openssl/bn.h>

#define NBITS 256

int main ()
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *e = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *M = BN_new();
    BIGNUM *C = BN_new();
    BIGNUM *res = BN_new();
    
    //对p、q、e M进行赋值
    BN_hex2bn(&n, "DCBFFE3E51F62E09CE7032E2677A78946A849DC4CDDE3A4D0CB81629242FB1A5");
    BN_hex2bn(&e, "010001");
    BN_hex2bn(&d, "74D806F9F3A62BAE331FFE3F0A68AFE35B3D2E4794148AACBC26AA381CD7D30D");
    BN_hex2bn(&M, "4120746f702073656372657421");
	
    //C = M^e mod n
    BN_mod_exp(C, M, e, n, ctx);
    BN_mod_exp(res, C, d, n, ctx);
    
    //print function
    char * number_str = BN_bn2hex(C);
    printf("C = %s \n", number_str);
    
    char * number_str1 = BN_bn2hex(res);
    printf("C^d = %s \n", number_str1);
    
    printf("      4120746f702073656372657421 == \"A top secret \"\n");

    //free 
    OPENSSL_free(number_str);
    OPENSSL_free(number_str1);
    
    BN_clear_free(e);
    BN_clear_free(d);
    BN_clear_free(n);
    BN_clear_free(M);
    BN_clear_free(C);
    BN_clear_free(res);
    return 0;
}