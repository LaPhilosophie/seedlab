#include <stdio.h>
#include <openssl/bn.h>

#define NBITS 256

int main ()
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *p = BN_new();
    BIGNUM *p_1 = BN_new();
    BIGNUM *q = BN_new();
    BIGNUM *q_1 = BN_new();
    BIGNUM *e = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *phi_n = BN_new();
    BIGNUM *res = BN_new();
    BIGNUM *one = BN_new();
    //对p、q、e进行赋值
    BN_hex2bn(&p, "F7E75FDC469067FFDC4E847C51F452DF");
    BN_hex2bn(&q, "E85CED54AF57E53E092113E62F436F4F");
    BN_hex2bn(&e, "0D88C3");
    BN_hex2bn(&one, "1");
    //计算得出p*q=n
    BN_mul(n, p, q, ctx);
    //phi_n=(p-1)*(q-1)
    BN_sub(p_1, p, one);
    BN_sub(q_1, q, one);
    BN_mul(phi_n,p_1, q_1, ctx);
    BN_mod_inverse(d, e, phi_n, ctx);

    //print function
    char * number_str = BN_bn2hex(n);
    char * number_str1 = BN_bn2hex(d);
    printf("p * q =  %s\n , d = %s \n", number_str,number_str1);
    
    //free 
    OPENSSL_free(number_str);
    OPENSSL_free(number_str1);
    
    BN_clear_free(p);
    BN_clear_free(q);
    BN_clear_free(e);
    BN_clear_free(d);
    BN_clear_free(n);
    BN_clear_free(p_1);
    BN_clear_free(q_1);
    BN_clear_free(phi_n);
    BN_clear_free(res);
    BN_clear_free(one);
    
    return 0;
}
