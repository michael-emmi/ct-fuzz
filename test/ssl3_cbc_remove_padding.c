#include <string.h>
#include <stdio.h>
#include "ct-fuzz.h"

typedef struct ssl3_record_st {
    unsigned int length;
    unsigned char *data;    
    int type;
    unsigned char *input;
} SSL3_RECORD;

typedef struct ssl3_state_st {
    long flags;
    unsigned char read_sequence[8];
} SSL3_STATE;

typedef struct evp_cipher_st {
    unsigned long flags;
} EVP_CIPHER;

typedef struct evp_cipher_ctx_st {
    const EVP_CIPHER *cipher;
} EVP_CIPHER_CTX;

typedef struct ssl_st {
    char *expand;
    unsigned long options;
    struct ssl3_state_st *s3;   
    EVP_CIPHER_CTX *enc_read_ctx; 
    int slicing_cheat;
} SSL;

/*-
 * ssl3_cbc_remove_padding removes padding from the decrypted, SSLv3, CBC
 * record in |rec| by updating |rec->length| in constant time.
 *
 * block_size: the block size of the cipher used to encrypt the record.
 * returns:
 *   0: (in non-constant time) if the record is publicly invalid.
 *   1: if the padding was valid
 *  -1: otherwise.
 */
#define NEWLINE printf("\n");
int ssl3_cbc_remove_padding(const SSL *s,
                            //SSL3_RECORD *rec,
                            unsigned block_size, unsigned mac_size)
{
  unsigned len = __ct_fuzz_get_arr_len(s);
  printf("SSL size: %u\n", len);
  for (unsigned i = 0; i < 1; ++i, ++s) {
    NEWLINE
    printf("expand: ");
    NEWLINE
    for (unsigned j = 0; j < __ct_fuzz_get_arr_len(s->expand); j++)
      printf("%c, ", s->expand[j]);
    NEWLINE
    printf("options: %lu\n", s->options);
    printf("s3: ");
    for (unsigned k = 0; k < __ct_fuzz_get_arr_len(s->s3); ++k) {
      printf("flags: %lu, ", s->s3[k].flags);
      printf(" read_sequence: %c..%c", s->s3[k].read_sequence[0], s->s3[k].read_sequence[7]);
      NEWLINE
    }
    printf("enc_read_ctx: %u, %u\n", __ct_fuzz_get_arr_len(s->enc_read_ctx), __ct_fuzz_get_arr_len(s->enc_read_ctx->cipher));
    printf("slicing_cheat: %d\n", s->slicing_cheat);
    printf("========================================\n");
  }
  printf("block_size: %u\n", block_size);
  printf("mac_size: %u\n", mac_size);
  printf("****************************************\n");
  return 1;
}

CT_FUZZ_SPEC(int, ssl3_cbc_remove_padding, const SSL *s,unsigned block_size, unsigned mac_size) {
  unsigned len = __ct_fuzz_get_arr_len(s);
  printf("SSL size: %u\n", len);
  for (unsigned i = 0; i < len; ++i, ++s) {
    NEWLINE
    printf("expand: ");
    for (unsigned j = 0; j < __ct_fuzz_get_arr_len(s->expand); j++)
      printf("%c, ", s->expand[j]);
    NEWLINE
    printf("options: %lu\n", s->options);
    printf("s3: ");
    for (unsigned k = 0; k < __ct_fuzz_get_arr_len(s->s3); ++k) {
      printf("flags: %lu, ", s->s3[k].flags);
      printf(" read_sequence: %c..%c", s->s3[k].read_sequence[0], s->s3[k].read_sequence[7]);
      NEWLINE
    }
    printf("enc_read_ctx: %u, %u\n", __ct_fuzz_get_arr_len(s->enc_read_ctx), __ct_fuzz_get_arr_len(s->enc_read_ctx->cipher));
    printf("slicing_cheat: %d\n", s->slicing_cheat);
    printf("========================================\n");
  }
  printf("block_size: %u\n", block_size);
  printf("mac_size: %u\n", mac_size);
  printf("****************************************\n");
}
