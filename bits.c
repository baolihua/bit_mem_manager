#include "bit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEN (1<<12) 
#define MEM_PAGE_SHIFT 12
#define MEM_PAGE_SIZE (1<<MEM_PAGE_SHIFT)
#define MEM_BASE_ADDR 0x2000000
#define MEM_END_ADDR  (0x2000000 + LEN * MEM_PAGE_SIZE) 


struct _Bits {
    char *bits;
    unsigned int length;
};

bits bit_new(unsigned int length)
{
    bits new_bits = (bits)malloc(sizeof(struct _Bits));
    if (new_bits == NULL)
        return NULL;

    int char_nums = sizeof(char) * (length >> 3) + 1;
    new_bits->bits = (char *)malloc(char_nums);
    if (new_bits == NULL) {
        free(new_bits);
        return NULL;
    }
    memset(new_bits->bits, 0, char_nums);
    new_bits->length = length;

    return new_bits;
}

void bit_destroy(bits bit)
{
    free(bit->bits);
    free(bit);
}

unsigned int bit_length(bits bit)
{
    return bit->length;
}

void bit_set(bits bit, unsigned int pos, unsigned char value)
{
    unsigned char mask = 0x80 >> (pos & 0x7);
    if (value) {
        bit->bits[pos>>3] |= mask;
    } else {
       bit->bits[pos>>3] &= ~mask;
    }
}

char bit_get(bits bit, unsigned int pos)
{
    unsigned char mask = 0x80 >> (pos & 0x7);

    return (mask & bit->bits[pos>>3]) == mask ? 1 : 0;
}


int test_bit(bits bit, unsigned int pos)
{
    unsigned int ret = 0;
    unsigned char mask = 0x80 >> (pos & 0x7);
    ret = bit->bits[pos>>3] & mask;
    if(ret){
	    return 1;
    }else{
	    return 0;
    }
}


int get_free_pos(bits bit)
{
	int i=0;
	int ret=0;
	for(i=0; i<LEN; i++){
		ret = test_bit(bit, i);
		if(!ret){
			return i;
	   	}
	}
	return -1;
}

unsigned int *codec_malloc(bits bit, unsigned int size)
{
	int i=0;
	int pos=0;
	unsigned int *pages = NULL;
	unsigned int page_nr = (size + MEM_PAGE_SIZE -1) >> MEM_PAGE_SHIFT;
	if(!page_nr || (page_nr > 4096)){
		printf("size err!\n");
		return NULL;
	}
	pages = (unsigned int *)malloc(page_nr * sizeof(unsigned int));
	if(!pages){
		printf("malloc failed!\n");
		return NULL;
	}
	for(i=0; i<page_nr; i++){
		pos = get_free_pos(bit);
		if(pos < 0){
			printf("get free pos failed!\n");
			free(pages);
			return NULL;
		}
		bit_set(bit, pos, 1);
		pages[i] = MEM_BASE_ADDR + pos * MEM_PAGE_SIZE;
	}
	return pages;	
}

void codec_free_one_addr(bits bit, unsigned int addr)
{
	int pos =0;
	int ret =0;
	if(addr & (MEM_PAGE_SIZE - 1)){
		printf("free addr failed!, addr = 0x%x\n", addr);
		return;
	}
	if(addr < MEM_BASE_ADDR){
		printf("addr is little than MEM_BASE_ADDR, addr = 0x%x\n", addr);
		return;
	}
	if(addr > MEM_END_ADDR){
		printf("addr is larger than MEM_END_ADDR, addr = 0x%x\n", addr);
		return;
	}
	pos = (addr - MEM_BASE_ADDR) >> MEM_PAGE_SHIFT;
	ret = test_bit(bit, pos);
	if(!ret){
		printf("err, this pos should be 1\n");
		return;
	}
	bit_set(bit, pos, 0);
}

void codec_free(bits bit, unsigned int *pages, int page_nr)
{
	int i=0;
	unsigned int addr = 0;

	if(!pages || !page_nr){
		printf("parameters err!\n");
		return;
	}
	for(i=0; i<page_nr; i++){
		addr = pages[i];
		if(addr > MEM_END_ADDR){
			printf("addr is larger than MEM_END_ADDR!\n");
			return;
		}
		if(addr < MEM_BASE_ADDR){
			printf("addr is little than MEM_BASE_ADDR!\n");
			return;
		}
		codec_free_one_addr(bit, addr);
	}
	return ;
}

void debug_show_bits(bits bit)
{
    int i;
    for (i = 0; i < LEN; i++) {
        printf("%d", bit_get(bit, i));
    }
    printf("\n");
}

int main(void)
{
    int i;
    int ret=0;
    unsigned int *pages_arr= NULL;
    bits bit = bit_new(LEN);

    printf("length: %u\n", bit_length(bit));
#if 0
    unsigned int test_value = 0x735D;
    unsigned char value;
    int i;
    for (i = LEN - 1; i >= 0; i--) {
        value = test_value & 1;
        bit_set(bit, i, value);
        test_value >>= 1;
    }
#endif
    bit_set(bit, 0, 1);
    bit_set(bit, 2, 1);
    bit_set(bit, 4095, 1);

    ret = test_bit(bit, 2);
    printf("ret = %d, test_bit 2\n", ret);


    ret = test_bit(bit, 34);
    printf("ret = %d, test_bit 34\n", ret);


    ret = test_bit(bit, 4095);
    printf("ret = %d, test_bit 4095\n", ret);

    ret = get_free_pos(bit);
    printf("ret = %d, get_free_pos\n", ret);
    bit_set(bit, ret, 1);

    ret = get_free_pos(bit);
    printf("ret = %d, get_free_pos\n", ret);
    bit_set(bit, ret, 1);

    ret = get_free_pos(bit);
    printf("ret = %d, get_free_pos\n", ret);
    bit_set(bit, ret, 1);

    debug_show_bits(bit);
    pages_arr = codec_malloc(bit, MEM_PAGE_SIZE *5);
    debug_show_bits(bit);

    codec_free(bit, pages_arr, 5);
    debug_show_bits(bit);

    bit_destroy(bit);

    return 0;
}
