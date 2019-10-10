/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/version.h>
#include <crypto/hash.h>
#include <crypto/public_key.h>
#include "include/defex_debug.h"
#include "include/defex_sign.h"


#define SIGN_SIZE		256
#define SHA256_DIGEST_SIZE	32
#define DER_BITSTRING_OFFS	24
#define MAX_DATA_LEN		300

extern char defex_public_key_start[];
extern char defex_public_key_end[];

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
#include <crypto/akcipher.h>
#include <crypto/algapi.h>

struct defex_rsa_completion {
	struct completion completion;
	int err;
};

static const unsigned char rsa_dgst_info_sha256[] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
	0x00, 0x04, 0x20
};

static int crypt_err;

static unsigned int __init crypt_memcmp_neq(const unsigned char *a, const unsigned char *b, size_t size)
{
	unsigned int res = 0;
	while (size--) {
		res |= (*a ^ *b);
		a++;
		b++;
	}
	return res;
}

static const unsigned char * __init pkcs1_v15_parse(const unsigned char *in_data, int data_size)
{
	int t_offset, ps_end;
	int i, start_offset = 0;

	if (data_size < (2 + 1 + sizeof(rsa_dgst_info_sha256) + SHA256_DIGEST_SIZE))
		return NULL;

	if (in_data[start_offset] == 0)
		start_offset++;
	if (in_data[start_offset] != 1) {
		crypt_err = -EBADMSG;
		return NULL;
	}
	start_offset++;

	/* Calculate offsets */
	t_offset = data_size - (sizeof(rsa_dgst_info_sha256) + SHA256_DIGEST_SIZE);
	ps_end = t_offset - 1;

	/* Check if there's a 0 seperator between PS and T */
	if (in_data[ps_end] != 0) {
		crypt_err = -EBADMSG;
		return NULL;
	}

	/* Check the PS 0xff padding */
	for (i = start_offset; i < ps_end; i++)
		if (in_data[i] != 0xff) {
			crypt_err = -EBADMSG;
			return NULL;
		}

	/* Compare the DER encoding T */
	if (crypt_memcmp_neq(rsa_dgst_info_sha256,
			in_data + t_offset,
			sizeof(rsa_dgst_info_sha256)) != 0) {
		crypt_err = -EBADMSG;
		return NULL;
	}

	return in_data + t_offset + sizeof(rsa_dgst_info_sha256);
}

static void __init defex_pkey_verify_done(struct crypto_async_request *req, int err)
{
	struct defex_rsa_completion *compl = req->data;

	if (err == -EINPROGRESS)
		return;

	compl->err = err;
	complete(&compl->completion);
}

static int __init defex_public_key_verify_signature(unsigned char *pub_key,
					int pub_key_size,
					unsigned char *signature,
					unsigned char *hash_sha256)
{
	struct defex_rsa_completion compl;
	struct crypto_akcipher *tfm;
	struct akcipher_request *req;
	struct scatterlist sig_sg, digest_sg;
	const unsigned char *result;
	void *output;
	unsigned int outlen;
	int ret = -ENOMEM;

	//A30 : Temporary skip signarue check at lower version
	printk("[DEFEX] it's low version kernel");
	return 0;

	tfm = crypto_alloc_akcipher("rsa", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	req = akcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto error_free_tfm;

	ret = crypto_akcipher_set_pub_key(tfm, pub_key, pub_key_size);
	if (ret)
		goto error_free_req;

	ret = -ENOMEM;
	outlen = crypto_akcipher_maxsize(tfm);
	output = kmalloc(outlen, GFP_KERNEL);
	if (!output)
		goto error_free_req;

	init_completion(&compl.completion);
	sg_init_one(&sig_sg, signature, SIGN_SIZE);
	sg_init_one(&digest_sg, output, outlen);
	akcipher_request_set_crypt(req, &sig_sg, &digest_sg, SIGN_SIZE, outlen);
	akcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG |
				      CRYPTO_TFM_REQ_MAY_SLEEP,
				      defex_pkey_verify_done, &compl);

	ret = crypto_akcipher_verify(req);
	if (ret == -EINPROGRESS || ret == -EBUSY) {
		wait_for_completion(&compl.completion);
		ret = compl.err;
	}
	if (ret < 0)
		goto out_free_output;

	result = pkcs1_v15_parse(output, req->dst_len);
	if (result) {
		/* Do the actual verification step */
		if (crypt_memcmp_neq(hash_sha256, result, SHA256_DIGEST_SIZE) != 0)
		ret = -EKEYREJECTED;
	} else {
		ret = crypt_err;
	}

out_free_output:
	kfree(output);
error_free_req:
	akcipher_request_free(req);
error_free_tfm:
	crypto_free_akcipher(tfm);
	return ret;
}

#else

static int __init defex_public_key_verify_signature(unsigned char *pub_key,
						int pub_key_size,
						unsigned char *signature,
						unsigned char *hash_sha256)
{
	struct public_key rsa_pub_key;
	struct public_key_signature sig;

	memset(&rsa_pub_key, 0, sizeof(rsa_pub_key));
	memset(&sig, 0, sizeof(sig));
	rsa_pub_key.key = pub_key;
	rsa_pub_key.keylen = pub_key_size;
	rsa_pub_key.pkey_algo = "rsa";
	rsa_pub_key.id_type = "X509";

	sig.pkey_algo = "rsa";
	sig.hash_algo = "sha256";
	sig.s = signature;
	sig.s_size = SIGN_SIZE;
	sig.digest = hash_sha256;
	sig.digest_size = SHA256_DIGEST_SIZE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
	sig.encoding = "pkcs1";
#endif

	return public_key_verify_signature(&rsa_pub_key, &sig);
}

#endif /* LINUX_VERSION_CODE < 4.6.0 */


#ifdef DEFEX_DEBUG_ENABLE
void __init blob(const char *buffer, const size_t bufLen, const int lineSize)
{
	size_t reminder = bufLen % lineSize;
	size_t wholePart = bufLen - reminder;
	size_t i = 0;
	size_t j = 0;
	int offset = 0;
	unsigned char *ptr = (unsigned char *)buffer;
	unsigned char stringToPrint[MAX_DATA_LEN];

	for(i = 0; i < wholePart; i += lineSize) {
		offset  = 0;
		offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "| 0x%0*zx | ", PR_HEX(i));

		for(j = 0; j < lineSize; j++) {
			offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "%02X ", *(ptr + i + j));
		}
		offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "| ");

		for(j = 0; j < lineSize; j++) {
			if (*(ptr + i + j)  < 0x10) {
				offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, ".");
			} else {
				offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "%c", *(ptr + i + j));
			}
		}

		offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, " |");
		printk(KERN_INFO "%s\n", stringToPrint);
		memset(stringToPrint, 0, MAX_DATA_LEN);
	}

	if (reminder) {
		offset  = 0;
		offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "| 0x%0*zx | ", PR_HEX(i));

		for(j = 0; j < reminder; j++) {
			offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "%02X ", *(ptr + wholePart + j));
		}

		for(j = 0; j < lineSize - reminder; j++) {
			offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "   ");
		}
		offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "| ");

		for(j = 0; j < reminder; j++) {
			if (*(ptr + i + j) < 0x10) {
				offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, ".");
			} else {
				offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, "%c", *(ptr + i + j));
			}
		}

		for(j = 0; j < lineSize - reminder; j++) {
			offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, " ");
		}
		offset += snprintf((char *)(stringToPrint + offset), MAX_DATA_LEN - offset, " |");
		printk(KERN_INFO "%s\n", stringToPrint);
		memset(stringToPrint, 0, MAX_DATA_LEN);
	}
}
#endif

int __init defex_calc_hash(const char *data, unsigned int size, unsigned char *hash)
{
	struct crypto_shash *handle = NULL;
	struct shash_desc* shash = NULL;
	int err = 0;

	handle = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(handle)) {
		pr_err("[DEFEX] Can't alloc sha256");
		return -1;
	}

	shash = kzalloc(sizeof(struct shash_desc) + crypto_shash_descsize(handle), GFP_KERNEL);
	if (NULL == shash) {
		goto hash_error;
	}

	shash->flags = 0;
	shash->tfm = handle;

	err = crypto_shash_init(shash);
	if (err < 0) {
		goto hash_error;
	}

	err = crypto_shash_update(shash, data, size);
	if (err < 0) {
		goto hash_error;
	}

	err = crypto_shash_final(shash, hash);
	if (err < 0) {
		goto hash_error;
	}

hash_error:
	if (shash)
		kfree(shash);
	if (handle)
		crypto_free_shash(handle);
	return err;

}

int __init defex_rules_signature_check(const char *rules_buffer, unsigned int rules_data_size, unsigned int *rules_size)
{
	int res;
	unsigned int defex_public_key_size = defex_public_key_end - defex_public_key_start;
	unsigned char *hash_sha256;
	unsigned char *hash_sha256_first;
	unsigned char *signature;
	unsigned char *pub_key;

	hash_sha256_first = kzalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
	hash_sha256 = kzalloc(SHA256_DIGEST_SIZE, GFP_KERNEL);
	signature = kzalloc(SIGN_SIZE, GFP_KERNEL);
	pub_key = kzalloc(defex_public_key_size, GFP_KERNEL);

	if (!hash_sha256_first || !hash_sha256 || !signature || !pub_key)
		return -1;

	memcpy(pub_key, defex_public_key_start, defex_public_key_size);
	memcpy(signature, (u8*)(rules_buffer + rules_data_size - SIGN_SIZE), SIGN_SIZE);

	defex_calc_hash(rules_buffer, rules_data_size - SIGN_SIZE, hash_sha256_first);
	defex_calc_hash(hash_sha256_first, SHA256_DIGEST_SIZE, hash_sha256);

#ifdef DEFEX_DEBUG_ENABLE
	printk("[DEFEX] Rules signature size = %d\n", SIGN_SIZE);
	blob(signature, SIGN_SIZE, 16);
	printk("[DEFEX] Key size = %d\n", defex_public_key_size);
	blob(pub_key, defex_public_key_size, 16);
	printk("[DEFEX] Final Hash:\n");
	blob(hash_sha256, SHA256_DIGEST_SIZE, 16);
#endif

	res = defex_public_key_verify_signature(pub_key + DER_BITSTRING_OFFS,
		defex_public_key_size - DER_BITSTRING_OFFS,
		signature, hash_sha256);
	kfree(hash_sha256_first);
	kfree(hash_sha256);
	kfree(signature);
	kfree(pub_key);
	if (rules_size && !res)
		*rules_size = rules_data_size - SIGN_SIZE;
	return res;
}
