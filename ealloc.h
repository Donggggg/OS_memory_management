#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

//granularity of memory to mmap from OS
#define PAGESIZE 4096 
#define NODENUM 16 // maximum number of nodes
//minimum allocation size
#define MINALLOC 256
#define FREE 1 // free chunk bit
#define ALLOC 0 // allocated chunk bit
#define USED 1 // used pool bit
#define UNUSED 0 // unused pool bit

typedef struct _memory {
	int offset; // 메모리 주소 시작 위치
	int size; // 할당된 크기
	int pool; // pool의 번호
	char* data; // 데이터 정보
	struct _memory* next; // 다음 메모리 포인터
} Memory;

typedef struct _memory_page {
	int total_size; // 사용가능한 메모리 크기
	char *page; // page 포인터
	int alloc_used_list[NODENUM]; // used allocated pool list 
	int free_used_list[NODENUM]; // used free pool list
	Memory alloc_memory_pool[NODENUM]; // allocation memory pool
	Memory free_memory_pool[NODENUM]; // free memory pool
	Memory* alloc_head; // alloc의 head 포인터
	Memory* alloc_tail; // alloc의 tail 포인터
	Memory* free_head; // free의 head 포인터
	Memory* free_tail; // free의 tail 포인터
} Page;

typedef struct memory_manager {
	int page_num;
	Page pages[PAGESIZE];
} Manager;

// function declarations to support
int get_memory_pool(int, int);
void init_alloc(void);
char *alloc(int);
void dealloc(char *);
void cleanup(void);
void print_alloc();
void print_free();
