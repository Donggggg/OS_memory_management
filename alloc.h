#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

#define PAGESIZE 4096 //size of memory to allocate from OS
#define MINALLOC 8 //allocations will be 8 bytes or multiples of it


typedef struct _memory {
	int offset; // 메모리 주소 시작 위치
	int size; // 할당된 크기
	char* data; // 데이터 정보
	struct _memory* next;
} Memory;

typedef struct memory_controller {
	int total_size; // 사용가능한 메모리 크기
	int alloc_num; // 할당된 메모리 덩어리 개수
	int chunk_num; // 할당되지 않은 메모리 덩어리 개수 
	Memory* alloc_head;
	Memory* alloc_tail;
	Memory* chunk_head;
	Memory* chunk_tail;
} Controller;

// function declarations
void print_alloc();
void print_chunk();
int init_alloc();
int cleanup();
char *alloc(int);
void dealloc(char *);
