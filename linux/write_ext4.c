#include <stdio.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>

typedef struct sparse_header {
  unsigned int	magic;		/* 0xed26ff3a */
  unsigned short	major_version;	/* (0x1) - reject images with higher major versions */
  unsigned short	minor_version;	/* (0x0) - allow images with higer minor versions */
  unsigned short	file_hdr_sz;	/* 28 bytes for first revision of the file format */
  unsigned short	chunk_hdr_sz;	/* 12 bytes for first revision of the file format */
  unsigned int	blk_sz;		/* block size in bytes, must be a multiple of 4 (4096) */
  unsigned int	total_blks;	/* total blocks in the non-sparse output image */
  unsigned int	total_chunks;	/* total chunks in the sparse input image */
  unsigned int	image_checksum; /* CRC32 checksum of the original data, counting "don't care" */
				/* as 0. Standard 802.3 polynomial, use a Public Domain */
				/* table implementation */
} sparse_header_t;

#define SPARSE_HEADER_MAGIC	0xed26ff3a

#define CHUNK_TYPE_RAW		0xCAC1
#define CHUNK_TYPE_FILL		0xCAC2
#define CHUNK_TYPE_DONT_CARE	0xCAC3

typedef struct chunk_header {
  unsigned short	chunk_type;	/* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
  unsigned short	reserved1;
  unsigned int	chunk_sz;	/* in blocks in output image */
  unsigned int	total_sz;	/* in bytes of chunk input file including chunk header and data */
} chunk_header_t;

#define SPARSE_HEADER_MAJOR_VER 1

typedef unsigned long lbaint_t;

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#ifdef UNPACK_EXT4_DEBUG
#define FBTDBG(fmt, args...) printf("[%s]: %d:\n"fmt, __func__, __LINE__, ##args)
#define FBTINFO(fmt, args...) printf("[%s]: %d:\n"fmt, __func__, __LINE__, ##args)
#else
#define FBTDBG(fmt, args...) do {} while (0)
#define FBTINFO(fmt, args...) do {} while (0)
#endif

unsigned long block_write(int dev, unsigned int start, unsigned int blkcnt,
	unsigned long blksz, const void *buffer)
{
	off_t offset = start * blksz;
	int size = blkcnt * blksz;
	int ret = 0;
	
	lseek(dev, offset, SEEK_SET);
	if ((ret = write(dev, buffer, size)) != size) {
		printf("block_write: size:%d  ret:%d\n", size, ret);
		return (ret / blksz);
	}
	
	return blkcnt;
}			       

static int unsparse(int dev, unsigned long blksz, 
	unsigned char *source, lbaint_t num_blks)
{
	sparse_header_t *header = (void *) source;
	u32 i;
//	unsigned long blksz = priv.dev_desc->blksz;
	u64 section_size = (u64)num_blks * blksz;
	u64 outlen = 0;
	lbaint_t sector = 0;

	printf("unsparse: blksz=%lu    num_blks=%lu\n", blksz, num_blks);

	FBTINFO("sparse_header:\n");
	FBTINFO("\t         magic=0x%08X\n", header->magic);
	FBTINFO("\t       version=%u.%u\n", header->major_version,
						header->minor_version);
	FBTINFO("\t file_hdr_size=%u\n", header->file_hdr_sz);
	FBTINFO("\tchunk_hdr_size=%u\n", header->chunk_hdr_sz);
	FBTINFO("\t        blk_sz=%u\n", header->blk_sz);
	FBTINFO("\t    total_blks=%u\n", header->total_blks);
	FBTINFO("\t  total_chunks=%u\n", header->total_chunks);
	FBTINFO("\timage_checksum=%u\n", header->image_checksum);

	if (header->magic != SPARSE_HEADER_MAGIC) {
		printf("sparse: bad magic\n");
		return 1;
	}

	if (((u64)header->total_blks * header->blk_sz) > section_size) {
		printf("sparse: section size %llu MB limit: exceeded\n",
				section_size/(1024*1024));
		return 1;
	}

	if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
	    (header->file_hdr_sz != sizeof(sparse_header_t)) ||
	    (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
		printf("sparse: incompatible format\n");
		return 1;
	}

	/* Skip the header now */
	source += header->file_hdr_sz;

	for (i = 0; i < header->total_chunks; i++) {
		u64 clen = 0;
		lbaint_t blkcnt;
		chunk_header_t *chunk = (void *) source;

		FBTINFO("chunk_header:\n");
		FBTINFO("\t    chunk_type=%u\n", chunk->chunk_type);
		FBTINFO("\t      chunk_sz=%u\n", chunk->chunk_sz);
		FBTINFO("\t      total_sz=%u\n", chunk->total_sz);
		/* move to next chunk */
		source += sizeof(chunk_header_t);

		switch (chunk->chunk_type) {
		case CHUNK_TYPE_RAW:
			clen = (u64)chunk->chunk_sz * header->blk_sz;
			FBTINFO("sparse: RAW blk=%d bsz=%d:"
			       " write(sector=%lu,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, sector, clen);

			if (chunk->total_sz != (clen + sizeof(chunk_header_t))) {
				printf("sparse: bad chunk size for"
				       " chunk %d, type Raw\n", i);
				return 1;
			}

			outlen += clen;
			if (outlen > section_size) {
				printf("sparse: section size %llu MB limit:"
				       " exceeded\n", section_size/(1024*1024));
				return 1;
			}
			blkcnt = clen / blksz;
			FBTDBG("sparse: RAW blk=%d bsz=%d:"
			       " write(sector=%lu,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, sector, clen);
#if 0			       
			if (block_write(priv.dev_desc->dev,
						       sector, blkcnt, source)
						!= blkcnt) {
#endif
			if (block_write(dev, sector, blkcnt, blksz, source)
						!= blkcnt) {
				printf("sparse: block write to sector %lu"
					" of %llu bytes (%ld blkcnt) failed\n",
					sector, clen, blkcnt);
				return 1;
			}

			sector += (clen / blksz);
			source += clen;
			break;

		case CHUNK_TYPE_DONT_CARE:
			if (chunk->total_sz != sizeof(chunk_header_t)) {
				printf("sparse: bogus DONT CARE chunk\n");
				return 1;
			}
			clen = (u64)chunk->chunk_sz * header->blk_sz;
			FBTDBG("sparse: DONT_CARE blk=%d bsz=%d:"
			       " skip(sector=%lu,clen=%llu)\n",
			       chunk->chunk_sz, header->blk_sz, sector, clen);

			outlen += clen;
			if (outlen > section_size) {
				printf("sparse: section size %llu MB limit:"
				       " exceeded\n", section_size/(1024*1024));
				return 1;
			}
			sector += (clen / blksz);
			break;

		default:
			printf("sparse: unknown chunk ID %04x\n",
			       chunk->chunk_type);
			return 1;
		}
	}

	printf("sparse: out-length %llu MB\n", outlen/(1024*1024));
	return 0;
}

unsigned long get_file_size(const char *path)
{
	unsigned long filesize = -1;	
	struct stat statbuff;

	if (stat(path, &statbuff) < 0)
		return filesize;
	else
		filesize = statbuff.st_size;
	
	return filesize;
}


int main(int argc, char* argv[])
{
	int image_fd = -1, dev_fd = -1, ret = 0;
	unsigned char* image_buf = NULL;
	unsigned long image_len;
	unsigned long size;
	
	//refer to uboot command: write.ext4 - addr partition
	if (3 != argc) {
		printf("write_ext4 image partition\n");
		return -1;
	}

	if ((image_fd = open(argv[1], O_RDONLY)) < 0) {
		perror("open");
		return -1;
	}

	if ((dev_fd = open(argv[2], O_WRONLY)) < 0) {
		perror("open");
		ret = -1;
		goto out;
	}

	/* BLKGETSIZE :  return device size /512 (long *arg) */
	if ((ioctl(dev_fd, BLKGETSIZE, &size)) < 0) {
		perror("BLKGETSIZE");
		ret = -1;
		goto out;
	}

	printf("size(blocks) = %lu   blcok size: 512\n", size);
	unsigned long long size_bytes = size;
	size_bytes = size_bytes * 512;
	printf("%s size: %lluBytes   %lluMB\n", argv[2], size_bytes, (size_bytes >> 20));

#if 0
	/* BLKSSZGET: get block device sector size */
	/* get block device logical block size */
	int BLKSSZ;
	if ((ioctl(dev_fd, BLKSSZGET, &BLKSSZ)) < 0) {
		perror("BLKSSZGET");
		ret = -1;
		goto out;
	}
	printf("BLKSSZ = %d\n", BLKSSZ);
#endif

	/* BLKPBSZGET : get block device physical block size */
	unsigned long blksz;
	if ((ioctl(dev_fd, BLKPBSZGET, &blksz)) < 0) {
		perror("BLKPBSZGET");
		ret = -1;
		goto out;
	}
	printf("block device physical block size: %lu\n", blksz);

	image_len = get_file_size(argv[1]);

	printf("write %s(%luBytes) to partition: %s\n", argv[1], image_len, argv[2]);
	
	image_buf = malloc(image_len);
	if (!image_buf) {
		printf("malloc failed. image_len = %lu\n", image_len);
		return -1;
		goto out;
	}

	if (read(image_fd, image_buf, image_len) != image_len) {
		printf("read file error.\n");
		return -1;
		goto out;
	}

	ret = unsparse(dev_fd, blksz, image_buf, size_bytes / blksz);

out:
	if (image_fd > 0)
		close(image_fd);

	if (dev_fd > 0)
		close(dev_fd);

	return ret;
}
