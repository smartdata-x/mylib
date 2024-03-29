/*
 * librdkafka - Apache Kafka C library
 *
 * Copyright (c) 2015, Magnus Edenhill
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#define _DARWIN_C_SOURCE  /* MSG_DONTWAIT */

#include "rdkafka_int.h"
#include "rdaddr.h"
#include "rdkafka_transport.h"
#include "rdkafka_transport_int.h"
#include "rdkafka_broker.h"

#include <errno.h>

#if WITH_VALGRIND
/* OpenSSL relies on uninitialized memory, which Valgrind will whine about.
 * We use in-code Valgrind macros to suppress those warnings. */
#include <valgrind/memcheck.h>
#else
#define VALGRIND_MAKE_MEM_DEFINED(A,B)
#endif


#ifdef _MSC_VER
#define socket_errno WSAGetLastError()
#else
#include <sys/socket.h>
#define socket_errno errno
#define SOCKET_ERROR -1
#endif

/* AIX doesn't have MSG_DONTWAIT */
#ifndef MSG_DONTWAIT
#  define MSG_DONTWAIT MSG_NONBLOCK
#endif


#if WITH_SSL
static mtx_t *rd_kafka_ssl_locks;
static int    rd_kafka_ssl_locks_cnt;

static once_flag rd_kafka_ssl_init_once = ONCE_FLAG_INIT;
#endif




/**
 * Close and destroy a transport handle
 */
void rd_kafka_transport_close (rd_kafka_transport_t *rktrans) {
#if WITH_SSL
	if (rktrans->rktrans_ssl) {
		SSL_shutdown(rktrans->rktrans_ssl);
		SSL_free(rktrans->rktrans_ssl);
	}
#endif

#if WITH_SASL
	if (rktrans->rktrans_sasl.conn)
		sasl_dispose(&rktrans->rktrans_sasl.conn);
#endif

	if (rktrans->rktrans_recv_buf)
		rd_kafka_buf_destroy(rktrans->rktrans_recv_buf);

	if (rktrans->rktrans_s != -1) {
#ifndef _MSC_VER
		close(rktrans->rktrans_s);
#else
		closesocket(rktrans->rktrans_s);
#endif
	}

	rd_free(rktrans);
}


static const char *socket_strerror(int err) {
#ifdef _MSC_VER
	static RD_TLS char buf[256];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
		       err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)buf, sizeof(buf)-1, NULL);
	return buf;
#else
	return rd_strerror(err);
#endif
}







static ssize_t
rd_kafka_transport_socket_sendmsg (rd_kafka_transport_t *rktrans,
				   const struct msghdr *msg,
				   char *errstr, size_t errstr_size) {
#ifndef _MSC_VER
	ssize_t r;

#ifdef sun
	/* See recvmsg() comment. Setting it here to be safe. */
	socket_errno = EAGAIN;
#endif
	r = sendmsg(rktrans->rktrans_s, msg, MSG_DONTWAIT
#ifdef MSG_NOSIGNAL
		    | MSG_NOSIGNAL
#endif
		);
	if (r == -1) {
		if (socket_errno == EAGAIN)
			return 0;
		rd_snprintf(errstr, errstr_size, "%s", rd_strerror(errno));
	}
	return r;
#else
	int i;
	ssize_t sum = 0;

	for (i = 0; i < msg->msg_iovlen; i++) {
		ssize_t r;

		r = send(rktrans->rktrans_s,
			 msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len, 0);
		if (r == SOCKET_ERROR) {
			if (sum > 0 || WSAGetLastError() == WSAEWOULDBLOCK)
				return sum;
			else {
				rd_snprintf(errstr, errstr_size, "%s",
					    socket_strerror(WSAGetLastError()));
				return -1;
			}
		}

		sum += r;
		if ((size_t)r < msg->msg_iov[i].iov_len)
			break;

	}
	return sum;
#endif
}


static ssize_t
rd_kafka_transport_socket_recvmsg (rd_kafka_transport_t *rktrans,
				   struct msghdr *msg,
				   char *errstr, size_t errstr_size) {
#ifndef _MSC_VER
	ssize_t r;
#ifdef sun
	/* SunOS doesn't seem to set errno when recvmsg() fails
	 * due to no data and MSG_DONTWAIT is set. */
	socket_errno = EAGAIN;
#endif
	r = recvmsg(rktrans->rktrans_s, msg, MSG_DONTWAIT);
	if (r == -1 && socket_errno == EAGAIN)
		return 0;
	else if (r == 0) {
		/* Receive 0 after POLLIN event means connection closed. */
		rd_snprintf(errstr, errstr_size, "Disconnected");
		return -1;
	} else if (r == -1)
		rd_snprintf(errstr, errstr_size, "%s", rd_strerror(errno));

	return r;
#else
	ssize_t sum = 0;
	int i;

	for (i = 0; i < msg->msg_iovlen; i++) {
		ssize_t r;

		r = recv(rktrans->rktrans_s,
			 msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len, 0);
		if (r == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;
			rd_snprintf(errstr, errstr_size, "%s", socket_strerror(WSAGetLastError()));
			return -1;
		}
		sum += r;
		if ((size_t)r < msg->msg_iov[i].iov_len)
			break;
	}
	return sum;
#endif
}




/**
 * CONNECT state is failed (errstr!=NULL) or done (TCP is up, SSL is working..).
 * From this state we either hand control back to the broker code,
 * or if authentication is configured we ente the AUTH state.
 */
void rd_kafka_transport_connect_done (rd_kafka_transport_t *rktrans,
				      char *errstr) {
	rd_kafka_broker_t *rkb = rktrans->rktrans_rkb;

#if WITH_SASL
	if (!errstr &&
	    (rkb->rkb_proto == RD_KAFKA_PROTO_SASL_PLAINTEXT ||
	     rkb->rkb_proto == RD_KAFKA_PROTO_SASL_SSL)) {
		char sasl_errstr[512];
		if (rd_kafka_sasl_client_new(rkb->rkb_transport, sasl_errstr,
					     sizeof(sasl_errstr)) == -1) {
			errno = EINVAL;
			rd_kafka_broker_fail(rkb,
					     RD_KAFKA_RESP_ERR__AUTHENTICATION,
					     "Failed to initialize "
					     "SASL authentication: %s",
					     sasl_errstr);
			return;

		}

		rd_kafka_broker_lock(rkb);
		rd_kafka_broker_set_state(rkb, RD_KAFKA_BROKER_STATE_AUTH);
		rd_kafka_broker_unlock(rkb);

		return;
	}
#endif

	rd_kafka_broker_connect_done(rkb, errstr);
}



#if WITH_SSL


/**
 * Serves the entire OpenSSL error queue and logs each error.
 * The last error is not logged but returned in 'errstr'.
 *
 * If 'rkb' is non-NULL broker-specific logging will be used,
 * else it will fall back on global 'rk' debugging.
 */
static char *rd_kafka_ssl_error (rd_kafka_t *rk, rd_kafka_broker_t *rkb,
				 char *errstr, int errstr_size) {
    unsigned long l;
    const char *file, *data;
    int line, flags;
    int cnt = 0;

    while ((l = ERR_get_error_line_data(&file, &line, &data, &flags)) != 0) {
	char buf[256];

	if (cnt++ > 0) {
		/* Log last message */
		if (rkb)
			rd_rkb_log(rkb, LOG_ERR, "SSL", "%s", errstr);
		else
			rd_kafka_log(rk, LOG_ERR, "SSL", "%s", errstr);
	}
	
	ERR_error_string_n(l, buf, sizeof(buf));

	rd_snprintf(errstr, errstr_size, "%s:%d: %s: %s",
		    file, line, buf, (flags & ERR_TXT_STRING) ? data : "");

    }

    if (cnt == 0)
    	    rd_snprintf(errstr, errstr_size, "No error");
    
    return errstr;
}


static void rd_kafka_transport_ssl_lock_cb (int mode, int i,
					    const char *file, int line) {
	if (mode & CRYPTO_LOCK)
		mtx_lock(&rd_kafka_ssl_locks[i]);
	else
		mtx_unlock(&rd_kafka_ssl_locks[i]);
}

static unsigned long rd_kafka_transport_ssl_threadid_cb (void) {
	return (unsigned long)thrd_current();
}


/**
 * Global OpenSSL cleanup.
 *
 * NOTE: This function is never called, see rd_kafka_transport_term()
 */
static RD_UNUSED void rd_kafka_transport_ssl_term (void) {
	int i;

	ERR_free_strings();

	for (i = 0 ; i < rd_kafka_ssl_locks_cnt ; i++)
		mtx_destroy(&rd_kafka_ssl_locks[i]);

	rd_free(rd_kafka_ssl_locks);
}


/**
 * Global OpenSSL init.
 */
static void rd_kafka_transport_ssl_init (void) {
	int i;
	
	rd_kafka_ssl_locks_cnt = CRYPTO_num_locks();
	rd_kafka_ssl_locks = rd_malloc(rd_kafka_ssl_locks_cnt *
				       sizeof(*rd_kafka_ssl_locks));
	for (i = 0 ; i < rd_kafka_ssl_locks_cnt ; i++)
		mtx_init(&rd_kafka_ssl_locks[i], mtx_plain);

	CRYPTO_set_id_callback(rd_kafka_transport_ssl_threadid_cb);
	CRYPTO_set_locking_callback(rd_kafka_transport_ssl_lock_cb);
	
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
}


/**
 * Set transport IO event polling based on SSL error.
 *
 * Returns -1 on permanent errors.
 *
 * Locality: broker thread
 */
static __inline int
rd_kafka_transport_ssl_io_update (rd_kafka_transport_t *rktrans, int ret,
				  char *errstr, int errstr_size) {
	int serr = SSL_get_error(rktrans->rktrans_ssl, ret);
	int serr2;

	switch (serr)
	{
	case SSL_ERROR_WANT_READ:
		rd_kafka_transport_poll_set(rktrans, POLLIN);
		break;

	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_CONNECT:
		rd_kafka_transport_poll_set(rktrans, POLLOUT);
		break;

	case SSL_ERROR_SYSCALL:
		if (!(serr2 = SSL_get_error(rktrans->rktrans_ssl, ret))) {
			if (ret == 0)
				errno = ECONNRESET;
			rd_snprintf(errstr, errstr_size,
				    "SSL syscall error: %s", rd_strerror(errno));
		} else
			rd_snprintf(errstr, errstr_size,
				    "SSL syscall error number: %d: %s", serr2,
				    rd_strerror(errno));
		return -1;

	default:
		rd_kafka_ssl_error(NULL, rktrans->rktrans_rkb,
				   errstr, errstr_size);
		return -1;
	}

	return 0;
}

static ssize_t
rd_kafka_transport_ssl_sendmsg (rd_kafka_transport_t *rktrans,
				const struct msghdr *msg,
				char *errstr, size_t errstr_size) {
	int i;
	ssize_t sum = 0;

	for (i = 0; i < (int)msg->msg_iovlen; i++) {
		int r;

		if (unlikely(msg->msg_iov[i].iov_len == 0))
		    continue;

		r = SSL_write(rktrans->rktrans_ssl, 
			      msg->msg_iov[i].iov_base,
			      msg->msg_iov[i].iov_len);

		if (unlikely(r <= 0)) {
			if (rd_kafka_transport_ssl_io_update(rktrans, r,
							     errstr,
							     errstr_size) == -1)
				return -1;
			else
				return sum;
		}


		sum += r;
		if ((size_t)r < msg->msg_iov[i].iov_len)
			break;

	}
	return sum;
}
 
static ssize_t
rd_kafka_transport_ssl_recvmsg (rd_kafka_transport_t *rktrans,
				struct msghdr *msg,
				char *errstr, size_t errstr_size) {
	ssize_t sum = 0;
	int i;
	
	for (i = 0; i < (int)msg->msg_iovlen; i++) {
		int r;

		r = SSL_read(rktrans->rktrans_ssl,
			     msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);

		if (unlikely(r <= 0)) {
			if (rd_kafka_transport_ssl_io_update(rktrans, r,
							     errstr,
							     errstr_size) == -1)
				return -1;
			else
				return sum;
		}

		VALGRIND_MAKE_MEM_DEFINED(msg->msg_iov[i].iov_base, r);

		sum += r;
		if ((size_t)r < msg->msg_iov[i].iov_len)
			break;
	}
	return sum;

}


/**
 * OpenSSL password query callback
 *
 * Locality: application thread
 */
static int rd_kafka_transport_ssl_passwd_cb (char *buf, int size, int rwflag,
					     void *userdata) {
	rd_kafka_t *rk = userdata;
	int pwlen;

	rd_kafka_dbg(rk, SECURITY, "SSLPASSWD",
		     "Private key file \"%s\" requires password",
		     rk->rk_conf.ssl.key_location);

	if (!rk->rk_conf.ssl.key_password) {
		rd_kafka_log(rk, LOG_WARNING, "SSLPASSWD",
			     "Private key file \"%s\" requires password but "
			     "no password configured (ssl.key.password)",
			     rk->rk_conf.ssl.key_location);
		return -1;
	}


	pwlen = strlen(rk->rk_conf.ssl.key_password);
	memcpy(buf, rk->rk_conf.ssl.key_password, RD_MIN(pwlen, size));

	return pwlen;
}

/**
 * Set up SSL for a newly connected connection
 *
 * Returns -1 on failure, else 0.
 */
static int rd_kafka_transport_ssl_connect (rd_kafka_broker_t *rkb,
					   rd_kafka_transport_t *rktrans,
					   char *errstr, int errstr_size) {
	int r;

	rktrans->rktrans_ssl = SSL_new(rkb->rkb_rk->rk_conf.ssl.ctx);
	if (!rktrans->rktrans_ssl)
		goto fail;

	if (!SSL_set_fd(rktrans->rktrans_ssl, rktrans->rktrans_s))
		goto fail;

	r = SSL_connect(rktrans->rktrans_ssl);
	if (r == 1) {
		/* Connected, highly unlikely since this is a
		 * non-blocking operation. */
		rd_kafka_transport_connect_done(rktrans, NULL);
		return 0;
	}

		
	if (rd_kafka_transport_ssl_io_update(rktrans, r,
					     errstr, errstr_size) == -1)
		return -1;
	
	return 0;

 fail:
	rd_kafka_ssl_error(NULL, rkb, errstr, errstr_size);
	return -1;
}


static RD_UNUSED int
rd_kafka_transport_ssl_io_event (rd_kafka_transport_t *rktrans, int events) {
	int r;
	char errstr[512];

	if (events & POLLOUT) {
		r = SSL_write(rktrans->rktrans_ssl, NULL, 0);
		if (rd_kafka_transport_ssl_io_update(rktrans, r,
						     errstr,
						     sizeof(errstr)) == -1)
			goto fail;
	}

	return 0;

 fail:
	/* Permanent error */
	rd_kafka_broker_fail(rktrans->rktrans_rkb, RD_KAFKA_RESP_ERR__TRANSPORT,
			     "%s", errstr);
	return -1;
}


/**
 * Verify SSL handshake was valid.
 */
static int rd_kafka_transport_ssl_verify (rd_kafka_transport_t *rktrans) {
	long int rl;
	X509 *cert;

	cert = SSL_get_peer_certificate(rktrans->rktrans_ssl);
	X509_free(cert);
	if (!cert) {
		rd_kafka_broker_fail(rktrans->rktrans_rkb,
				     RD_KAFKA_RESP_ERR__SSL,
				     "Broker did not provide a certificate");
		return -1;
	}

	if ((rl = SSL_get_verify_result(rktrans->rktrans_ssl)) != X509_V_OK) {
		rd_kafka_broker_fail(rktrans->rktrans_rkb,
				     RD_KAFKA_RESP_ERR__SSL,
				     "Failed to verify broker certificate: %s",
				     X509_verify_cert_error_string(rl));
		return -1;
	}

	rd_rkb_dbg(rktrans->rktrans_rkb, SECURITY, "SSLVERIFY",
		   "Broker SSL certificate verified");
	return 0;
}

/**
 * SSL handshake handling.
 * Call repeatedly (based on IO events) until handshake is done.
 *
 * Returns -1 on error, 0 if handshake is still in progress, or 1 on completion.
 */
static int rd_kafka_transport_ssl_handhsake (rd_kafka_transport_t *rktrans) {
	rd_kafka_broker_t *rkb = rktrans->rktrans_rkb;
	char errstr[512];
	int r;
	
	r = SSL_do_handshake(rktrans->rktrans_ssl);
	
	if (r == 1) {
		/* SSL handshake done. Verify. */
		if (rd_kafka_transport_ssl_verify(rktrans) == -1)
			return -1;

		rd_kafka_transport_connect_done(rktrans, NULL);
		return 1;
		
	} else if (rd_kafka_transport_ssl_io_update(rktrans, r,
						    errstr,
						    sizeof(errstr)) == -1) {
		rd_kafka_broker_fail(rkb, RD_KAFKA_RESP_ERR__SSL,
				     "SSL handshake failed: %s%s", errstr,
				     strstr(errstr, "unexpected message") ?
				     ": client authentication might be "
				     "required (see broker log)" : "");
				     
		return -1;
	}

	return 0;
}


/**
 * Once per rd_kafka_t handle cleanup of OpenSSL
 *
 * Locality: any thread
 *
 * NOTE: rd_kafka_wrlock() MUST be held
 */
void rd_kafka_transport_ssl_ctx_term (rd_kafka_t *rk) {
	SSL_CTX_free(rk->rk_conf.ssl.ctx);
	rk->rk_conf.ssl.ctx = NULL;
}

/**
 * Once per rd_kafka_t handle initialization of OpenSSL
 *
 * Locality: application thread
 *
 * NOTE: rd_kafka_wrlock() MUST be held
 */
int rd_kafka_transport_ssl_ctx_init (rd_kafka_t *rk,
				     char *errstr, int errstr_size) {
	int r;
	SSL_CTX *ctx;

	call_once(&rd_kafka_ssl_init_once, rd_kafka_transport_ssl_init);

	
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (!ctx)
		goto fail;


	/* Key file password callback */
	SSL_CTX_set_default_passwd_cb(ctx, rd_kafka_transport_ssl_passwd_cb);
	SSL_CTX_set_default_passwd_cb_userdata(ctx, rk);

	/* Ciphers */
	if (rk->rk_conf.ssl.cipher_suites) {
		rd_kafka_dbg(rk, SECURITY, "SSL",
			     "Setting cipher list: %s",
			     rk->rk_conf.ssl.cipher_suites);
		if (!SSL_CTX_set_cipher_list(ctx,
					     rk->rk_conf.ssl.cipher_suites)) {
			rd_snprintf(errstr, errstr_size,
				    "No recognized ciphers");
			goto fail;
		}
	}


	if (rk->rk_conf.ssl.ca_location) {
		/* CA certificate location, either file or directory. */
		int is_dir = rd_kafka_path_is_dir(rk->rk_conf.ssl.ca_location);

		rd_kafka_dbg(rk, SECURITY, "SSL",
			     "Loading CA certificate(s) from %s %s",
			     is_dir ? "directory":"file",
			     rk->rk_conf.ssl.ca_location);
		
		r = SSL_CTX_load_verify_locations(ctx,
						  !is_dir ?
						  rk->rk_conf.ssl.
						  ca_location : NULL,
						  is_dir ?
						  rk->rk_conf.ssl.
						  ca_location : NULL);

		if (r != 1)
			goto fail;
	}

	if (rk->rk_conf.ssl.cert_location) {
		rd_kafka_dbg(rk, SECURITY, "SSL",
			     "Loading certificate from file %s",
			     rk->rk_conf.ssl.cert_location);

		r = SSL_CTX_use_certificate_file(ctx,
						 rk->rk_conf.ssl.cert_location,
						 SSL_FILETYPE_PEM);

		if (r != 1)
			goto fail;
	}

	if (rk->rk_conf.ssl.key_location) {
		rd_kafka_dbg(rk, SECURITY, "SSL",
			     "Loading private key file from %s",
			     rk->rk_conf.ssl.key_location);

		r = SSL_CTX_use_PrivateKey_file(ctx,
						rk->rk_conf.ssl.key_location,
						SSL_FILETYPE_PEM);
		if (r != 1)
			goto fail;
	}


	SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	rk->rk_conf.ssl.ctx = ctx;
	return 0;

 fail:
	rd_kafka_ssl_error(rk, NULL, errstr, errstr_size);
	SSL_CTX_free(ctx);

	return -1;
}


#endif /* WITH_SSL */


ssize_t
rd_kafka_transport_sendmsg (rd_kafka_transport_t *rktrans,
			    const struct msghdr *msg,
			    char *errstr, size_t errstr_size) {

#if WITH_SSL
	if (rktrans->rktrans_ssl)
		return rd_kafka_transport_ssl_sendmsg(rktrans, msg,
						      errstr, errstr_size);
	else
#endif
		return rd_kafka_transport_socket_sendmsg(rktrans, msg,
							 errstr, errstr_size);
}


ssize_t
rd_kafka_transport_recvmsg (rd_kafka_transport_t *rktrans,
			    struct msghdr *msg,
			    char *errstr, size_t errstr_size) {
#if WITH_SSL
	if (rktrans->rktrans_ssl)
		return rd_kafka_transport_ssl_recvmsg(rktrans, msg,
						      errstr, errstr_size);
	else
#endif
		return rd_kafka_transport_socket_recvmsg(rktrans, msg,
							 errstr, errstr_size);
}




/**
 * Length framed receive handling.
 * Currently only supports a the following framing:
 *     [int32_t:big_endian_length_of_payload][payload]
 *
 * To be used on POLLIN event, will return:
 *   -1: on fatal error (errstr will be updated, *rkbufp remains unset)
 *    0: still waiting for data (*rkbufp remains unset)
 *    1: data complete, (buffer returned in *rkbufp)
 */
int rd_kafka_transport_framed_recvmsg (rd_kafka_transport_t *rktrans,
				       rd_kafka_buf_t **rkbufp,
				       char *errstr, size_t errstr_size) {
	rd_kafka_buf_t *rkbuf = rktrans->rktrans_recv_buf;
	int r;
	const int log_decode_errors = 0;

	/* States:
	 *   !rktrans_recv_buf: initial state; set up buf to receive header.
	 *    rkbuf_len == 0:   awaiting header
	 *    rkbuf_len > 0:    awaiting payload
	 */

	if (!rkbuf) {
		rkbuf = rd_kafka_buf_new(NULL, 1, 4);
		/* Point read buffer to main buffer. */
		rkbuf->rkbuf_rbuf = rkbuf->rkbuf_buf;
		rd_kafka_buf_push(rkbuf, rkbuf->rkbuf_buf, 4);

		rktrans->rktrans_recv_buf = rkbuf;
	}


	r = rd_kafka_transport_recvmsg(rktrans, &rkbuf->rkbuf_msg,
				       errstr, errstr_size);
	if (r == 0)
		return 0;
	else if (r == -1)
		return -1;

	rkbuf->rkbuf_wof += r;

	if (rkbuf->rkbuf_len == 0) {
		/* Frame length not known yet. */
		int32_t frame_len;

		if (rkbuf->rkbuf_wof < sizeof(frame_len)) {
			/* Wait for entire frame header. */
			return 0;
		}

		/* Reader header: payload length */
		rd_kafka_buf_read_i32(rkbuf, &frame_len);

		if (frame_len < 0 ||
		    frame_len > rktrans->rktrans_rkb->
		    rkb_rk->rk_conf.recv_max_msg_size) {
			rd_snprintf(errstr, errstr_size,
				    "Invalid frame size %"PRId32, frame_len);
			return -1;
		}

		rkbuf->rkbuf_len = frame_len;
		if (frame_len == 0) {
			/* Payload is empty, we're done. */
			rktrans->rktrans_recv_buf = NULL;
			*rkbufp = rkbuf;
			return 1;
		}

		/* Allocate memory to hold entire frame payload in contigious
		 * memory. */
		rd_kafka_buf_alloc_recvbuf(rkbuf, frame_len);

		/* Try reading directly, there is probably more data available*/
		return rd_kafka_transport_framed_recvmsg(rktrans, rkbufp,
							 errstr, errstr_size);
	}

	if (rkbuf->rkbuf_wof == rkbuf->rkbuf_len) {
		/* Payload is complete. */
		rktrans->rktrans_recv_buf = NULL;
		*rkbufp = rkbuf;
		return 1;
	}

	/* Wait for more data */
	return 0;

 err:
	if (rkbuf)
		rd_kafka_buf_destroy(rkbuf);
	rd_snprintf(errstr, errstr_size, "Frame header parsing failed");
	return -1;
}


/**
 * TCP connection established.
 * Set up socket options, SSL, etc.
 *
 * Locality: broker thread
 */
static void rd_kafka_transport_connected (rd_kafka_transport_t *rktrans) {
	rd_kafka_broker_t *rkb = rktrans->rktrans_rkb;

        rd_rkb_dbg(rkb, BROKER, "CONNECT",
                   "Connected to %s",
                   rd_sockaddr2str(rkb->rkb_addr_last,
                                   RD_SOCKADDR2STR_F_PORT |
                                   RD_SOCKADDR2STR_F_FAMILY));

	/* Set socket send & receive buffer sizes if configuerd */
	if (rkb->rkb_rk->rk_conf.socket_sndbuf_size != 0) {
		if (setsockopt(rktrans->rktrans_s, SOL_SOCKET, SO_SNDBUF,
			       (void *)&rkb->rkb_rk->rk_conf.socket_sndbuf_size,
			       sizeof(rkb->rkb_rk->rk_conf.
				      socket_sndbuf_size)) == SOCKET_ERROR)
			rd_rkb_log(rkb, LOG_WARNING, "SNDBUF",
				   "Failed to set socket send "
				   "buffer size to %i: %s",
				   rkb->rkb_rk->rk_conf.socket_sndbuf_size,
				   socket_strerror(socket_errno));
	}

	if (rkb->rkb_rk->rk_conf.socket_rcvbuf_size != 0) {
		if (setsockopt(rktrans->rktrans_s, SOL_SOCKET, SO_RCVBUF,
			       (void *)&rkb->rkb_rk->rk_conf.socket_rcvbuf_size,
			       sizeof(rkb->rkb_rk->rk_conf.
				      socket_rcvbuf_size)) == SOCKET_ERROR)
			rd_rkb_log(rkb, LOG_WARNING, "RCVBUF",
				   "Failed to set socket receive "
				   "buffer size to %i: %s",
				   rkb->rkb_rk->rk_conf.socket_rcvbuf_size,
				   socket_strerror(socket_errno));
	}



#if WITH_SSL
	if (rkb->rkb_proto == RD_KAFKA_PROTO_SSL ||
	    rkb->rkb_proto == RD_KAFKA_PROTO_SASL_SSL) {
		char errstr[512];

		/* Set up SSL connection.
		 * This is also an asynchronous operation so dont
		 * propagate to broker_connect_done() just yet. */
		if (rd_kafka_transport_ssl_connect(rkb, rktrans,
						   errstr,
						   sizeof(errstr)) == -1) {
			rd_kafka_transport_connect_done(rktrans, errstr);
			return;
		}
		return;
	}
#endif

	/* Propagate connect success */
	rd_kafka_transport_connect_done(rktrans, NULL);
}


/**
 * IO event handler.
 *
 * Locality: broker thread
 */
static void rd_kafka_transport_io_event (rd_kafka_transport_t *rktrans,
					 int events) {
	char errstr[512];
	int r;
	socklen_t intlen = sizeof(r);
	rd_kafka_broker_t *rkb = rktrans->rktrans_rkb;

	switch (rkb->rkb_state)
	{
	case RD_KAFKA_BROKER_STATE_CONNECT:
#if WITH_SSL
		if (rktrans->rktrans_ssl) {
			/* Currently setting up SSL connection:
			 * perform handshake. */
			rd_kafka_transport_ssl_handhsake(rktrans);
			return;
		}
#endif

		/* Asynchronous connect finished, read status. */
		if (!(events & (POLLOUT|POLLERR|POLLHUP)))
			return;

		if (getsockopt(rktrans->rktrans_s, SOL_SOCKET,
			       SO_ERROR, (void *)&r, &intlen) == -1) {
			rd_kafka_broker_fail(
                                rkb, RD_KAFKA_RESP_ERR__TRANSPORT,
                                "Connect to %s failed: "
                                "unable to get status from "
                                "socket %d: %s",
                                rd_sockaddr2str(rkb->rkb_addr_last,
                                                RD_SOCKADDR2STR_F_PORT |
                                                RD_SOCKADDR2STR_F_FAMILY),
                                rktrans->rktrans_s,
                                rd_strerror(socket_errno));
		} else if (r != 0) {
			/* Connect failed */
                        errno = r;
			rd_snprintf(errstr, sizeof(errstr),
				    "Connect to %s failed: %s",
                                    rd_sockaddr2str(rkb->rkb_addr_last,
                                                    RD_SOCKADDR2STR_F_PORT |
                                                    RD_SOCKADDR2STR_F_FAMILY),
                                    rd_strerror(r));

			rd_kafka_transport_connect_done(rktrans, errstr);
		} else {
			/* Connect succeeded */
			rd_kafka_transport_connected(rktrans);
		}
		break;

	case RD_KAFKA_BROKER_STATE_AUTH:
#if WITH_SASL
		rd_kafka_assert(NULL, rktrans->rktrans_sasl.conn != NULL);

		/* SASL handshake */
		if (rd_kafka_sasl_io_event(rktrans, events,
					   errstr, sizeof(errstr)) == -1) {
			errno = EINVAL;
			rd_kafka_broker_fail(rkb,
					     RD_KAFKA_RESP_ERR__AUTHENTICATION,
					     "SASL authentication failure: %s",
					     errstr);
			return;
		}
#endif
		break;

	case RD_KAFKA_BROKER_STATE_UP:
	case RD_KAFKA_BROKER_STATE_UPDATE:

		if (events & POLLIN) {
			while (rd_kafka_recv(rkb) > 0)
				;
		}

		if (events & POLLHUP) {
			rd_kafka_broker_fail(rkb, RD_KAFKA_RESP_ERR__TRANSPORT,
					     "Connection closed");
			return;
		}

		if (events & POLLOUT) {
			if (rd_atomic32_get(&rkb->rkb_outbufs.rkbq_cnt) > 0)
				while (rd_kafka_send(rkb) > 0)
					;
		}
		break;

	default:
		rd_kafka_assert(rkb->rkb_rk, !*"bad state");
	}
}


/**
 * Poll and serve IOs
 *
 * Locality: broker thread 
 */
void rd_kafka_transport_io_serve (rd_kafka_transport_t *rktrans,
                                  int timeout_ms) {
	rd_kafka_broker_t *rkb = rktrans->rktrans_rkb;
	int events;

	if (rd_atomic32_get(&rkb->rkb_outbufs.rkbq_cnt) > 0)
		rd_kafka_transport_poll_set(rkb->rkb_transport, POLLOUT);

	if ((events = rd_kafka_transport_poll(rktrans, timeout_ms)) <= 0)
                return;

        rd_kafka_transport_poll_clear(rktrans, POLLOUT);

	rd_kafka_transport_io_event(rktrans, events);
}


/**
 * Initiate asynchronous connection attempt.
 *
 * Locality: broker thread
 */
rd_kafka_transport_t *rd_kafka_transport_connect (rd_kafka_broker_t *rkb,
						  const rd_sockaddr_inx_t *sinx,
						  char *errstr,
						  int errstr_size) {
	rd_kafka_transport_t *rktrans;
	int s = -1;
	int on = 1;


        rkb->rkb_addr_last = sinx;

	s = rkb->rkb_rk->rk_conf.socket_cb(sinx->in.sin_family,
					   SOCK_STREAM, IPPROTO_TCP,
					   rkb->rkb_rk->rk_conf.opaque);
	if (s == -1) {
		rd_snprintf(errstr, errstr_size, "Failed to create socket: %s",
			    socket_strerror(socket_errno));
		return NULL;
	}


#ifdef SO_NOSIGPIPE
	/* Disable SIGPIPE signalling for this socket on OSX */
	if (setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)) == -1) 
		rd_rkb_dbg(rkb, BROKER, "SOCKET",
			   "Failed to set SO_NOSIGPIPE: %s",
			   socket_strerror(socket_errno));
#endif

	/* Enable TCP keep-alives, if configured. */
	if (rkb->rkb_rk->rk_conf.socket_keepalive) {
#ifdef SO_KEEPALIVE
		if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE,
			       (void *)&on, sizeof(on)) == SOCKET_ERROR)
			rd_rkb_dbg(rkb, BROKER, "SOCKET",
				   "Failed to set SO_KEEPALIVE: %s",
				   socket_strerror(socket_errno));
#else
		rd_rkb_dbg(rkb, BROKER, "SOCKET",
			   "System does not support "
			   "socket.keepalive.enable (SO_KEEPALIVE)");
#endif
	}


	/* Set the socket to non-blocking */
#ifdef _MSC_VER
	if (ioctlsocket(s, FIONBIO, &on) == SOCKET_ERROR) {
		rd_snprintf(errstr, errstr_size,
			    "Failed to set socket non-blocking: %s",
			    socket_strerror(socket_errno));
		goto err;
	}
#else
	{
		int fl = fcntl(s, F_GETFL, 0);
		if (fl == -1 ||
		    fcntl(s, F_SETFL, fl | O_NONBLOCK) == -1) {
			rd_snprintf(errstr, errstr_size,
				    "Failed to set socket non-blocking: %s",
				    socket_strerror(socket_errno));
			goto err;
		}
	}
#endif


	rd_rkb_dbg(rkb, BROKER, "CONNECT", "Connecting to %s (%s) "
		   "with socket %i",
		   rd_sockaddr2str(sinx, RD_SOCKADDR2STR_F_FAMILY |
				   RD_SOCKADDR2STR_F_PORT),
		   rd_kafka_secproto_names[rkb->rkb_proto], s);

	/* Connect to broker */
	if (connect(s, (struct sockaddr *)sinx,
		    RD_SOCKADDR_INX_LEN(sinx)) == SOCKET_ERROR &&
	    (socket_errno != EINPROGRESS
#ifdef _MSC_VER
		&& socket_errno != WSAEWOULDBLOCK
#endif
		)) {
		rd_rkb_dbg(rkb, BROKER, "CONNECT",
			   "couldn't connect to %s: %s (%i)",
			   rd_sockaddr2str(sinx,
					   RD_SOCKADDR2STR_F_PORT |
					   RD_SOCKADDR2STR_F_FAMILY),
			   socket_strerror(socket_errno), socket_errno);
		rd_snprintf(errstr, errstr_size,
			    "Failed to connect to broker at %s: %s",
			    rd_sockaddr2str(sinx, RD_SOCKADDR2STR_F_NICE),
			    socket_strerror(socket_errno));
		goto err;
	}

	/* Create transport handle */
	rktrans = rd_calloc(1, sizeof(*rktrans));
	rktrans->rktrans_rkb = rkb;
	rktrans->rktrans_s = s;
	rktrans->rktrans_pfd.fd = s;

	/* Poll writability to trigger on connection success/failure. */
	rd_kafka_transport_poll_set(rktrans, POLLOUT);

	return rktrans;

 err:
	if (s != -1) {
#ifndef _MSC_VER
		close(s);
#else
		closesocket(s);
#endif
	}
	return NULL;
}



void rd_kafka_transport_poll_set(rd_kafka_transport_t *rktrans, int event) {
	rktrans->rktrans_pfd.events |= event;
}

void rd_kafka_transport_poll_clear(rd_kafka_transport_t *rktrans, int event) {
	rktrans->rktrans_pfd.events &= ~event;
}


int rd_kafka_transport_poll(rd_kafka_transport_t *rktrans, int tmout) {
#ifndef _MSC_VER
	int r;

	r = poll(&rktrans->rktrans_pfd, 1, tmout);
	if (r <= 0)
		return r;
	return rktrans->rktrans_pfd.revents;
#else
	int r;

	r = WSAPoll(&rktrans->rktrans_pfd, 1, tmout);
	if (r == 0)
		return 0;
	else if (r == SOCKET_ERROR)
		return -1;
	return rktrans->rktrans_pfd.revents;
#endif
}





#if 0
/**
 * Global cleanup.
 * This is dangerous and SHOULD NOT be called since it will rip
 * the rug from under the application if it uses any of this functionality
 * in its own code. This means we will leak some memory (in the OpenSSL case)
 * on exit.
 */
void rd_kafka_transport_term (void) {
#ifdef _MSC_VER
	(void)WSACleanup(); /* FIXME: dangerous */
#endif

#if WITH_SSL
	rd_kafka_transport_ssl_term();
#endif

}
#endif
 
void rd_kafka_transport_init(void) {
#ifdef _MSC_VER
	WSADATA d;
	(void)WSAStartup(MAKEWORD(2, 2), &d);
#endif
}
