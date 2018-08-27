/*
 * iov.h
 *
 *  Created on: Aug 27, 2018
 *      Student (coder): Tung Dang
 */

#ifndef QEMU_IOV_H_
#define QEMU_IOV_H_

#include <stdint.h>
#include <stdio.h>

size_t iov_size(const struct iovec *iov, const unsigned int iov_cnt);
size_t iov_from_buf_full(const struct iovec *iov, unsigned int iov_cnt, size_t offset, const void *buf, size_t bytes);
size_t iov_to_buf_full(const struct iovec *iov, const unsigned int iov_cnt, size_t offset, void *buf, size_t bytes);

static inline size_t iov_from_buf(const struct iovec *iov, unsigned int iov_cnt, size_t offset, const void *buf, size_t bytes) {
	if (__builtin_constant_p(bytes) && iov_cnt && offset <= iov[0].iov_len && bytes <= iov[0].iov_len - offset) {
		memcpy(iov[0].iov_base + offset, buf, bytes);
		return bytes;
	}
	else
	{
		return iov_from_buf_full(iov,iov_cnt, offset, buf, bytes);
	}
}

static inline size_t iov_to_buf(const struct iovec *iov, const unsigned int iov_cnt, size_t offset, void *buf, size_t bytes) {
	if (__builtin_constant_p(bytes) && iov_cnt && offset <= iov[0].iov_len && bytes <= iov[0].iov_len - offset)
	{
		memcpy(buf,iov[0].iov_base + offset, bytes);
		return bytes;
	}
	else
	{
		return iov_to_buf_full(iov,iov_cnt, offset, buf, bytes);
	}
}

size_t iov_memset(const struct iovec *iov, const unsigned int iov_cnt, size_t offset, int fillc, size_t bytes);
ssize_t iov_send_recv(int sockfd, const struct iovec *iov, unsigned iov_cnt, size_t offset, size_t bytes, bool do_send);
#define iov_recv(sockfd, iov, iov_cnt, offset, bytes) \
	iov_send_recv(sockfd, iov, iov_cnt, offset, bytes, false)
#define iov_send(sockfd, iov, iov_cnt, offset, bytes) \
	iov_send_recv(sockfd, iov, iov_cnt, offset, bytes, true)

unsigned iov_copy(struct iovec *dst_iov, unsigned int dst_iov_cnt,
                 const struct iovec *iov, unsigned int iov_cnt, size_t offset, size_t bytes);

size_t iov_discard_front(struct iovec **iov, unsigned int *iov_cnt, size_t bytes);
size_t iov_discard_back(struct iovec **iov, unsigned int *iov_cnt, size_t bytes);
typedef struct QEMUIOVector {
	struct iovec *iov;
	int niov;
	int nalloc;
	size_t size;
} QEMUIOVector;

void qemu_iovec_init(QEMUIOVector *qiov, int alloc_hint);
void qemu_iovec_init_external(QEMUIOVector *qiov, struct iovec *iov, int niov);
void qemu_iovec_add(QEMUIOVector *qiov, void *base, size_t len);
void qemu_iovec_concat(QEMUIOVector *dst,
                       QEMUIOVector *src, size_t soffset, size_t sbytes);
size_t qemu_iovec_concat_iov(QEMUIOVector *dst,
                             struct iovec *src_iov, unsigned int src_cnt,
                             size_t soffset, size_t sbytes);
bool qemu_iovec_is_zero(QEMUIOVector *qiov);
void qemu_iovec_destroy(QEMUIOVector *qiov);
void qemu_iovec_reset(QEMUIOVector *qiov);
size_t qemu_iovec_to_buf(QEMUIOVector *qiov, size_t offset,
                         void *buf, size_t bytes);
size_t qemu_iovec_from_buf(QEMUIOVector *qiov, size_t offset,
                           const void *buf, size_t bytes);
size_t qemu_iovec_memset(QEMUIOVector *qiov, size_t offset,
                         int fillc, size_t bytes);
ssize_t qemu_iovec_compare(QEMUIOVector *a, QEMUIOVector *b);
void qemu_iovec_clone(QEMUIOVector *dest, const QEMUIOVector *src, void *buf);
void qemu_iovec_discard_back(QEMUIOVector *qiov, size_t bytes);

#endif /* QEMU_IOV_H_ */
