#ifndef _CRYPTO_H_
#define _CRYPTO_H_



#include <sys/types.h>
#include <glib.h>
#include "str.h"



struct crypto_context;
struct rtp_header;
struct rtcp_packet;

typedef int (*crypto_func_rtp)(struct crypto_context *, struct rtp_header *, str *, u_int64_t);
typedef int (*crypto_func_rtcp)(struct crypto_context *, struct rtcp_packet *, str *, u_int64_t);
typedef int (*hash_func_rtp)(struct crypto_context *, char *out, str *in, u_int64_t);
typedef int (*hash_func_rtcp)(struct crypto_context *, char *out, str *in);
typedef int (*session_key_init_func)(struct crypto_context *);
typedef int (*session_key_cleanup_func)(struct crypto_context *);

struct crypto_suite {
	const char *name;
	unsigned int
		master_key_len,
		master_salt_len,
		session_key_len,	/* n_e */
		session_salt_len,	/* n_s */
		srtp_auth_tag,		/* n_a */
		srtcp_auth_tag,
		srtp_auth_key_len,	/* n_a */
		srtcp_auth_key_len;
	unsigned long long int
		srtp_lifetime,
		srtcp_lifetime;
	int kernel_cipher;
	int kernel_hmac;
	crypto_func_rtp encrypt_rtp,
			decrypt_rtp;
	crypto_func_rtcp encrypt_rtcp,
			 decrypt_rtcp;
	hash_func_rtp hash_rtp;
	hash_func_rtcp hash_rtcp;
	session_key_init_func session_key_init;
	session_key_cleanup_func session_key_cleanup;
};

struct crypto_context {
	const struct crypto_suite *crypto_suite;
	/* we only support one master key for now */
	char master_key[16];
	char master_salt[14];
	u_int64_t mki;
	unsigned int mki_len;
	unsigned int tag;

	u_int64_t last_index;
	/* XXX replay list */
	/* <from, to>? */

	char session_key[16]; /* k_e */
	char session_salt[14]; /* k_s */
	char session_auth_key[20];

	void *session_key_ctx[2];

	int have_session_key:1;
};

struct crypto_context_pair {
	struct crypto_context in,
			      out;
};




extern const struct crypto_suite crypto_suites[];
extern const int num_crypto_suites;



const struct crypto_suite *crypto_find_suite(const str *);
int crypto_gen_session_key(struct crypto_context *, str *, unsigned char, int);

static inline int crypto_encrypt_rtp(struct crypto_context *c, struct rtp_header *rtp,
		str *payload, u_int64_t index)
{
	return c->crypto_suite->encrypt_rtp(c, rtp, payload, index);
}
static inline int crypto_decrypt_rtp(struct crypto_context *c, struct rtp_header *rtp,
		str *payload, u_int64_t index)
{
	return c->crypto_suite->decrypt_rtp(c, rtp, payload, index);
}
static inline int crypto_encrypt_rtcp(struct crypto_context *c, struct rtcp_packet *rtcp,
		str *payload, u_int64_t index)
{
	return c->crypto_suite->encrypt_rtcp(c, rtcp, payload, index);
}
static inline int crypto_decrypt_rtcp(struct crypto_context *c, struct rtcp_packet *rtcp,
		str *payload, u_int64_t index)
{
	return c->crypto_suite->decrypt_rtcp(c, rtcp, payload, index);
}
static inline int crypto_init_session_key(struct crypto_context *c) {
	return c->crypto_suite->session_key_init(c);
}
static inline void crypto_cleanup(struct crypto_context *c) {
	if (!c->crypto_suite)
		return;
	if (c->crypto_suite->session_key_cleanup)
		c->crypto_suite->session_key_cleanup(c);
}
static inline void crypto_context_move(struct crypto_context *dst, struct crypto_context *src) {
	int i;

	if (src == dst)
		return;
	crypto_cleanup(dst);
	*dst = *src;
	for (i = 0; i < G_N_ELEMENTS(src->session_key_ctx); i++)
		src->session_key_ctx[i] = NULL;
}



#endif
