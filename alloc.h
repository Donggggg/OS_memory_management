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
	struct _memory* next; // 다음 메모리 포인터
} Memory;

typedef struct memory_manager {
	int total_size; // 사용가능한 메모리 크기
	char *page; // page 포인터
	Memory* alloc_head; // alloc의 head 포인터
	Memory* alloc_tail; // alloc의 tail 포인터
	Memory* chunk_head; // chunk의 head 포인터
	Memory* chunk_tail; // chunk의 tail 포인터
} Manager;

// function declarations
void print_alloc();
void print_chunk();
int init_alloc();
int cleanup();
char *alloc(int);
void dealloc(char *);
