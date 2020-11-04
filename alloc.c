#include "alloc.h"

Controller controller;
Memory* new;

const int flag1 = PROT_READ | PROT_WRITE;
const int flag2 = MAP_ANONYMOUS | MAP_PRIVATE;

void print_alloc()
{
	Memory *cur;
	cur = controller.alloc_head;

	while(cur!=NULL){
		printf("alloc>> offset:%d size:%d\n", cur->offset,cur->size);
		cur = cur->next;
	}
}
void print_chunk()
{
	Memory *cur;
	cur = controller.chunk_head;

	while(cur!=NULL){
		printf("chunk>> offset:%d size:%d\n", cur->offset,cur->size);
		cur = cur->next;
	}
	printf("\n");
}
int init_alloc()
{
	// 기본 데이터 구조 초기화
	controller.total_size = PAGESIZE;

	// 할당 메모리 초기화
	controller.alloc_num = 0;
	controller.alloc_head = NULL;
	controller.alloc_tail = NULL;

	// 할당되지 않은 메모리 초기화
	controller.chunk_num = 1;
	new = malloc(sizeof(Memory));
	new->offset = 0;
	new->size = PAGESIZE;
	new->next = NULL;
	controller.chunk_head = new;
	controller.chunk_tail = new;

	// 오류코드리턴 추가 하셈
	return 0;
}

int cleanup()
{

}

char* alloc(int size)
{
	char* mem;
	Memory* cur;

	// 오류 예외 처리
	if(size % 8 != 0)
		return NULL;
	if(size > controller.total_size)
		return NULL;

	cur = controller.chunk_head;

	while(cur != NULL) 
	{
		if(cur->size >= size) {
			if((mem = mmap(0, size, flag1, flag2, -1, 0)) == MAP_FAILED) {
				fprintf(stderr, "mmap error\n");
			}

			// alloc된 메모리 관리
			new = malloc(sizeof(Memory));
			new->offset = cur->offset;
			new->size = size;
			new->data = mem;
			new->next = NULL;

			if(controller.alloc_head == NULL) {
				controller.alloc_head = new;
				controller.alloc_tail = new;
			}
			else {
				controller.alloc_tail->next = new;
				controller.alloc_tail = new;
			}

			// chunk 메모리 관리
			cur->offset = cur->offset + size;
			cur->size = cur->size - size;

			if(cur->size == 0) { // chunk가 없으면
				controller.chunk_head = NULL;
				controller.chunk_tail = NULL;
				cur = NULL;
				free(cur);
			}

			return mem;
		}

		cur = cur->next;
	}

	return NULL;
}

void dealloc(char* mem)
{
	Memory *cur, *cur2, *prev, *prev2;
	Memory *fore, *back, *back_prev, *fore_prev;

	cur = controller.alloc_head;

	while(cur != NULL) 
	{
		if(strcmp(cur->data, mem) == 0) {
			munmap(mem, cur->size);
			fore_prev = prev2;

			// alloc 메모리가 1개인 경우 리셋
			if(controller.alloc_head->next == NULL) 
			{
				controller.chunk_num = 1;
				new = malloc(sizeof(Memory));
				new->offset = 0;
				new->size = PAGESIZE;
				new->next = NULL;
				controller.chunk_head = new;
				controller.chunk_tail = new;
			}
			else // 그 외의 경우 합병 케이스 탐색
			{
				cur2 = controller.chunk_head;
				fore = back = NULL;

				// 합병 케이스 탐색
				while(cur2 != NULL)
				{
					if(cur2->offset + cur2->size == cur->offset)
						fore = cur2;
					else if(cur2->offset == cur->offset + cur->size) {
						back = cur2;
						back_prev = prev;
					}

					prev= cur2;
					cur2 = cur2->next;
				}

				if(fore == NULL && back == NULL) { // 합병이 안되는 경우
					new = malloc(sizeof(Memory));
					new->offset = cur->offset;
					new->size = cur->size;
					new->next = NULL;

					if(controller.chunk_head == NULL) {
						controller.chunk_head = new;
						controller.chunk_tail = new;
					}
					else {
						controller.chunk_tail->next = new;
						new = controller.chunk_tail;
					}
				}
				else if(fore != NULL && back == NULL) { // 앞의 chunk와 합병인 경우
					//printf("fore merger\n");
					fore->size += cur->size; 
				}
				else if(fore == NULL && back != NULL) { // 뒤의 chunk와 합병인 경우
					//printf("back merge\n");
					back->size += cur->size; 
					back->offset -= cur->size;
				}
				else if(fore != NULL && back != NULL) { // 쌍방향 합병인 경우
					fore->size += cur->size + back->size; 

					// back chunk 리스트에서 제거
					if(back == controller.chunk_head) {
						controller.chunk_head = back->next;
					}
					else if(back == controller.chunk_tail)  {
						controller.chunk_tail = back_prev;
						controller.chunk_tail->next = NULL;
					}
					else {
						back_prev->next = back->next;
					}
					back = NULL;
					free(back);

				}
			}

			// cur을 alloc  리스트에서 제거
			if(cur == controller.alloc_head){
				//printf("head\n");
				controller.alloc_head = cur->next;
			}
			else if(cur == controller.alloc_tail) {
				//printf("tail\n");
				controller.alloc_tail = fore_prev;
				controller.alloc_tail->next = NULL;
			}
			else{ 
				//printf("else\n");
				fore_prev->next = cur->next;
			}
			cur = NULL;
			free(cur);

			return ;
		}

		prev2 = cur;
		cur = cur->next;
	}
}
