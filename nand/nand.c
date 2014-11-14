#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <ctype.h>
#include <mtd/mtd-abi.h>
#include <asm-generic/errno-base.h>

#define PART_NAME "/dev/mtd8"

#define nand_dbg(fmt, arg...)	printf("%s(%d): " fmt, __FUNCTION__, __LINE__, ##arg)

/*
 * Print data buffer in hex and ascii form to the terminal.
 *
 * data reads are buffered so that each memory address is only read once.
 * Useful when displaying the contents of volatile registers.
 *
 * parameters:
 *    addr: Starting address to display at start of line
 *    data: pointer to data buffer
 *    width: data value width.  May be 1, 2, or 4.
 *    count: number of values to display
 *    linelen: Number of values to print per line; specify 0 for default length
 */
#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer (unsigned long addr, void* data, unsigned int width,
	unsigned int count, unsigned int linelen)
{
	/* linebuf as a union causes proper alignment */
	union linebuf {
		unsigned int ui[MAX_LINE_LENGTH_BYTES/sizeof(unsigned int) + 1];
		unsigned short us[MAX_LINE_LENGTH_BYTES/sizeof(unsigned short) + 1];
		unsigned char  uc[MAX_LINE_LENGTH_BYTES/sizeof(unsigned char) + 1];
	} lb;
	int i;

	if (linelen*width > MAX_LINE_LENGTH_BYTES)
		linelen = MAX_LINE_LENGTH_BYTES / width;
	if (linelen < 1)
		linelen = DEFAULT_LINE_LENGTH_BYTES / width;

	while (count) {
		printf("%08lx:", addr);

		/* check for overflow condition */
		if (count < linelen)
			linelen = count;

		/* Copy from memory into linebuf and print hex values */
		for (i = 0; i < linelen; i++) {
			unsigned int x;
			if (width == 4)
				x = lb.ui[i] = *(volatile unsigned int *)data;
			else if (width == 2)
				x = lb.us[i] = *(volatile unsigned short *)data;
			else
				x = lb.uc[i] = *(volatile unsigned char *)data;
			printf(" %0*x", width * 2, x);
			data += width;
		}

		/* Print data in ASCII characters */
		for (i = 0; i < linelen * width; i++) {
			if (!isprint(lb.uc[i]) || lb.uc[i] >= 0x80)
				lb.uc[i] = '.';
		}
		lb.uc[i] = '\0';
		printf("    %s\n", lb.uc);

		/* update references */
		addr += linelen * width;
		count -= linelen;
	}

	return 0;
}

/**
 * check_skip_len
 *
 * Check if there are any bad blocks, and whether length including bad
 * blocks fits into device
 *
 * @return 0 if the image fits and there are no bad blocks
 *         1 if the image fits, but there are bad blocks
 *        -1 if the image does not fit
 */
static int check_skip_len(int fd, struct mtd_info_user *nand,
	unsigned long offset, unsigned int length)
{
	unsigned int len_excl_bad = 0;
	int ret = 0;
	unsigned int block_len, block_off;
	unsigned long long block_start;

	nand_dbg("fd:%d offset:0x%lx length:0x%x\n", fd, offset, length);
	while (len_excl_bad < length) {
		if (offset >= nand->size)
			return -1;

		block_start = offset & ~(unsigned long)(nand->erasesize - 1);
		block_off = offset & (nand->erasesize - 1);
		block_len = nand->erasesize - block_off;

		/*
		  * Check if an eraseblock is bad.
		  * Ioctl return 1 means bad block, 0 means good block.
		  */
		if (0 == ioctl(fd, MEMGETBADBLOCK, &block_start)) {
			nand_dbg("Check block: 0x%llx ... Good.\n", block_start);
			len_excl_bad += block_len;
		}
		else {
			nand_dbg("Check block: 0x%llx ... Bad.\n", block_start);
			ret = 1;
		}
		
		offset += block_len;
	}

	return ret;
}


/**
 * nand_write_skip_bad:
 *
 * Write image to NAND flash.
 * Blocks that are marked bad are skipped and the is written to the next
 * block instead as long as the image is short enough to fit even after
 * skipping the bad blocks.
 */
int nand_write_skip_bad(int fd, struct mtd_info_user *nand, 
	unsigned long offset, unsigned int length, unsigned char *buffer)
{
	int rval = 0;
	unsigned int left_to_write = length;
	unsigned char *p_buffer = buffer;
	int need_skip;
	unsigned long long off;
	unsigned int block_offset;
	unsigned int write_size;
	
	if ((offset & (nand->writesize - 1)) != 0) {
		printf ("Attempt to write non page aligned data\n");
		return -EINVAL;
	}

	need_skip = check_skip_len(fd, nand, offset, length);
	if (need_skip < 0) {
		printf ("Attempt to write outside the flash area\n");
		return -EINVAL;
	}

	if (!need_skip) {
		printf("nand write: No bad blocks, NO skip!!\n");
		/* no bad blocks, so write directly */
		off = offset;
		lseek(fd, off, SEEK_SET);
		rval = write(fd, (void *)buffer, length);
		if (rval < 0)
			printf("NAND write to offset 0x%lx failed %d\n",
				offset, rval);
		return rval;
	}

	while (left_to_write > 0) {
		block_offset = offset & (nand->erasesize - 1);
		
		/*
		  * Check if an eraseblock is bad.
		  * Ioctl return 1 means bad block, 0 means good block.
		  */
		off = offset;
		if (1 == ioctl(fd, MEMGETBADBLOCK, &off)) {
			printf ("Skip bad block 0x%lx\n",
				offset & ~(nand->erasesize - 1));
			offset += nand->erasesize - block_offset;
			continue;
		}

		if (left_to_write < (nand->erasesize - block_offset))
			write_size = left_to_write;
		else
			write_size = nand->erasesize - block_offset;

		nand_dbg("offset:0x%lx	write_size:0x%x\n", offset, write_size);
		off = offset;
		lseek(fd, off, SEEK_SET);
		rval = write(fd, p_buffer, write_size);
		if (rval < 0) {
			printf ("NAND write to offset 0x%lx failed %d\n", offset, rval);
			return rval;
		}

		offset += write_size;
		p_buffer += write_size;
		left_to_write -= write_size;
	}

	length -= left_to_write;
	return length;
}

/**
 * nand_read_skip_bad:
 *
 * Read image from NAND flash.
 * Blocks that are marked bad are skipped and the next block is readen
 * instead as long as the image is short enough to fit even after skipping the
 * bad blocks.
 */
int nand_read_skip_bad(int fd, struct mtd_info_user *nand,
	unsigned long offset, unsigned int length, unsigned char *buffer)
{
	int rval;
	unsigned int left_to_read = length;
	unsigned char *p_buffer = buffer;
	int need_skip;
	unsigned int block_offset;
	unsigned int read_length;
	unsigned long long off;

	if ((offset & (nand->writesize - 1)) != 0) {
		printf ("Attempt to read non page aligned data\n");
		return -EINVAL;
	}

	need_skip = check_skip_len(fd, nand, offset, length);
	if (need_skip < 0) {
		printf ("Attempt to read outside the flash area\n");
		return -EINVAL;
	}

	if (!need_skip) {
		printf("nand read: No bad blocks, NO skip!!\n");
		/* no bad blocks, so read directly */
		off = offset;
		lseek(fd, off, SEEK_SET);
		if ((rval = read(fd, (void *)buffer, length)) < 0)
			printf ("NAND read from offset %lx failed %d\n",
				offset, rval);
		return rval;
	}

	while (left_to_read > 0) {
		block_offset = offset & (nand->erasesize - 1);

		/*
		  * Check if an eraseblock is bad.
		  * Ioctl return 1 means bad block, 0 means good block.
		  */
		off = offset;
		if (1 == ioctl(fd, MEMGETBADBLOCK, &off)) {
			printf ("Skipping bad block at: 0x%lx\n",
				offset & ~(nand->erasesize - 1));
			offset += nand->erasesize - block_offset;
			continue;
		}
		if (left_to_read < (nand->erasesize - block_offset))
			read_length = left_to_read;
		else
			read_length = nand->erasesize - block_offset;

		nand_dbg("offset:0x%lx	read_length:0x%x\n", offset, read_length);
		off = offset;
		lseek(fd, off, SEEK_SET);
		rval = read(fd, p_buffer, read_length);
		if (rval < 0) {
			printf ("NAND read from offset 0x%lx failed %d\n", offset, rval);
			return rval;
		}

		left_to_read -= read_length;
		offset       += read_length;
		p_buffer     += read_length;
	}

	length -= left_to_read;
	return length;
}
int nand_write(const char *part_name, unsigned long long of,
	void *buffer, unsigned int size)
//int nand_write(const char *part_name, unsigned long offset,
//	void *buffer, unsigned int size)
{
	int fd;
	int ret = 0;
	unsigned int length = size;
	unsigned long offset = of;
	struct mtd_info_user nand;
	
	nand_dbg("part_name:%s offset:0x%lx length:0x%lx\n", part_name, offset, size);

	if ((ret = fd = open(part_name, O_SYNC | O_RDWR)) < 0) {
		printf("nand_write: open nand device fail!\n");
		return ret;
	}

	if ((ret = ioctl(fd, MEMGETINFO, &nand)) < 0) {
		printf("nand_write: ioctl MEMGETINFO fail!\n");
		goto exit;
	}

	ret = nand_write_skip_bad(fd, &nand, offset, length, buffer);

exit:
	close(fd);
	return ret;
}

int nand_read(const char *part_name, unsigned long long of,
	void *buffer, unsigned int size)
//int nand_read(const char *part_name, unsigned long offset,
//	void *buffer, unsigned int size)
{
	int fd;
	int ret = 0;
	unsigned int length = size;
	unsigned long offset = of;
	struct mtd_info_user nand;
	
	nand_dbg("part_name:%s offset:0x%lx length:0x%lx\n", part_name, offset, size);

	if ((ret = fd = open(part_name, O_SYNC | O_RDONLY)) < 0) {
		printf("nand_read: open nand device fail!\n");
		return ret;
	}

	if ((ret = ioctl(fd, MEMGETINFO, &nand)) < 0) {
		printf("nand_read: ioctl MEMGETINFO fail!\n");
		goto exit;
	}

	ret = nand_read_skip_bad(fd, &nand, offset, length, buffer);

exit:
	close(fd);
	return ret;
}

int nand_erase(const char *part_name, unsigned long long of, unsigned int size)
//int nand_erase(const char *part_name, unsigned long offset, unsigned long size)
{
	int ret = 0, fd;
	struct mtd_info_user nand;
	struct erase_info_user erase;
	unsigned long offset = of;
	unsigned long long off;

	nand_dbg("part_name:%s offset:0x%lx length:0x%lx\n", part_name, offset, size);
	
	if ((ret = fd = open(part_name, O_SYNC | O_RDWR)) < 0) {
		printf("nand_erase: open nand device fail!\n");
		return ret;
	}

	if ((ret = ioctl(fd, MEMGETINFO, &nand)) < 0) {
		printf("nand_erase: ioctl MEMGETINFO fail!\n");
		goto exit;
	}

	if ((offset & (nand.erasesize - 1)) != 0) {
		ret = -EINVAL;
		printf ("nand_erase: offset[0x%lx] should be block align\n", offset);
		goto exit;
	}

	if ((size & (nand.erasesize - 1)) != 0) {
		ret = -EINVAL;
		printf ("nand_erase: size[0x%lx] should be block align\n", size);
		goto exit;
	}

	/* erase blocks one by one */
	while (size > 0) {
		if (offset >= nand.size) {
			printf ("Attempt to erase outside the flash area\n");
			return -EINVAL;
		}
		
		erase.start = offset;
		erase.length = nand.erasesize;
		nand_dbg("start erase ... 0x%x\n", erase.start);

		/*
		  * Check if an eraseblock is bad.
		  * Ioctl return 1 means bad block, 0 means good block.
		  */
		off = erase.start;
		if (1 == ioctl(fd, MEMGETBADBLOCK, &off)) {
			printf ("Skipping bad block at: 0x%x\n",
				erase.start & ~(nand.erasesize - 1));
			offset += nand.erasesize;
			continue;
		}
		
		if ((ret = ioctl(fd, MEMERASE, &erase)) < 0) {
			printf("nand erase fail, partition: %s  offset: 0x%x\n",			
				part_name, erase.start);
				
			/* Mark an eraseblock as bad */
			if (-EIO == ret) {
				printf("mark bad block at partition: %s  offset: 0x%x\n",
					part_name, erase.start);
				off = erase.start;
				ioctl(fd, MEMSETBADBLOCK, &off);
				offset += nand.erasesize;
				continue;
			}
			else
				return ret;
		}

		size -= nand.erasesize;
		offset += nand.erasesize;
	}

	ret = 0;
exit:
	close(fd);
	return ret;
}

unsigned int nand_get_info(const char *part_name, unsigned int* page_size,
	unsigned int* block_size)
{
	int ret, fd;
	char* part = (char*)part_name;
	struct mtd_info_user nand;

	/*
	  * Usally all nand partitions' page size and block size are the same,
	  * default use /dev/mtd/mtd7 as an example.
	 */
	if (!part_name)
		part = PART_NAME;

	if ((ret = fd = open(part, O_RDONLY)) < 0) {
		printf("nand_get_info: open nand device fail!\n");
		return ret;
	}

	if ((ret = ioctl(fd, MEMGETINFO, &nand)) < 0) {
		printf("nand_get_info: ioctl MEMGETINFO fail!\n");
		goto exit;
	}

	nand_dbg("mtd_info_user(%s):\n", part);
	nand_dbg("    writesize:0x%x\n", nand.writesize);
	nand_dbg("    oobsize:0x%x\n", nand.oobsize);
	nand_dbg("    erasesize:0x%x\n", nand.erasesize);
	nand_dbg("    size:0x%x\n", nand.size);

	if (page_size)
		*page_size = nand.writesize;

	if (block_size)
		*block_size = nand.erasesize;

exit:
	close(fd);
	return ret;
}

unsigned int nand_get_page_size()
{
	unsigned int page_size;
	nand_get_info(NULL, &page_size, NULL);
	return page_size;
}

unsigned int nand_get_block_size()
{
	unsigned int block_size;
	nand_get_info(NULL, NULL, &block_size);
	return block_size;
}

void nand_mark_bad(const char *part_name, unsigned long offset)
{
	int fd, ret;
	unsigned long long off;
	
	if ((ret = fd = open(part_name, O_SYNC | O_RDONLY)) < 0) {
		printf("nand_read: open nand device fail!\n");
		return;
	}	

	off = offset;
	printf("nand_erase: mark bad block at partition: %s  offset: 0x%lx\n",
		part_name, offset);	
	ioctl(fd, MEMSETBADBLOCK, &off);
	close(fd);
}


/* BUFFER_SIZE: 16MB */
#define BUFFER_SIZE 0xc00000

unsigned char buf1[BUFFER_SIZE];
unsigned char buf2[BUFFER_SIZE];	

int main(int argc, char* argv[])
{
	int len, fd;
	unsigned long offset = 0;
	unsigned int block_size;
	unsigned long i, length, bo, temp_o, temp_l;
	int ret;
//	unsigned int page_size = nand_get_page_size();
	block_size = nand_get_block_size();
	
	memset(buf1, 0x59, BUFFER_SIZE);
	memset(buf2, 0xaa, BUFFER_SIZE);

//	nand_mark_bad(PART_NAME, block_size * 2);
//	nand_mark_bad(PART_NAME, block_size * 5);
#if 1
	while(1) {
#if 0

		temp_o = (rand() % BUFFER_SIZE) & ~(page_size - 1);
		memset(buf1, 0x59, BUFFER_SIZE);
		memset(buf2, 0xaa, BUFFER_SIZE);
		temp_l = (rand() % BUFFER_SIZE) & ~(page_size - 1);

//		temp_o = 0x400000;
		temp_o = 0x4e1000;
//		temp_l = 0x587000;
		temp_l = 0x600000;
#endif

		temp_o = (rand() % BUFFER_SIZE) & ~(block_size - 1);
		memset(buf1, 0x59, BUFFER_SIZE);
		memset(buf2, 0xaa, BUFFER_SIZE);
		temp_l = (rand() % BUFFER_SIZE) & ~(block_size - 1);

		fd = open("test.bin", O_RDONLY);
		printf("read test.bin: len = 0x%x\n\n", read(fd, buf1, BUFFER_SIZE));
		close(fd);

		printf("erase ret = %d\n", (ret = nand_erase(PART_NAME, temp_o,temp_l)));
		printf("\n\n");

		if (ret < 0)
			continue;

		offset = temp_o;
		length = temp_l;

		printf("write ret = %d\n", nand_write(PART_NAME, offset, buf1, length));
		printf("\n\n");
//		sleep(2);
		printf("read ret = %d\n", nand_read(PART_NAME, offset, buf2, length));

		
		printf("\n\nBegin compare ... \n");

		for(i = 0; i < length; i++) {
			if(buf1[i] != buf2[i]) {
				printf("\nbuffff buf1[0x%lx] = 0x%x\n", i, buf1[i]);
				print_buffer(i, buf1 + i, 1, 16, 0);
				printf("\nbuffff buf2[0x%lx] = 0x%x\n", i, buf2[i]);
				print_buffer(i, buf2 + i, 1, 16, 0);
			}
		}

		if (memcmp(buf1, buf2, length)) {
			printf("not equal ... \n");
			exit(1);
		}
		else
			printf("the same...\n");

//		break;
	}
#else
	nand_read(PART_NAME, 0x4EB000, buf2, page_size);
	print_buffer(0x1eeb000, buf2, 1, page_size, 0);
#endif

	return 0;
}

