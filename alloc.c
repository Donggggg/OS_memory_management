#include "alloc.h"

static Manager manager; // 메모리 관리자 
Memory* new; // 메모리 리스트 관리용 포인터

// mmap 플래그 값
const int flag1 = PROT_READ | PROT_WRITE;
const int flag2 = MAP_ANONYMOUS | MAP_PRIVATE;

int get_memory_pool(int where) // 할당가능한 memory pool의 번호를 리턴
{
	if(where == ALLOC) { // alloc list에서 찾음
		for(int i = 0; i < NODENUM; i++)
			if(manager.alloc_used_list[i] == UNUSED) {
				manager.alloc_used_list[i] = USED;
				return i;
			}
	}
	else if(where == FREE) { // free list에서 찾음
		for(int i = 0; i < NODENUM; i++)
			if(manager.free_used_list[i] == UNUSED) {
				manager.free_used_list[i] = USED;
				return i;
			}
	}

	return -1;
}

int init_alloc() // 메모리 관리자 초기화
{
	// mmap으로 page 할당
	if((manager.page = mmap(0, PAGESIZE, flag1, flag2, -1, 0)) == MAP_FAILED) 
		return 1;
	manager.total_size = PAGESIZE;

	// 할당 메모리 초기화
	manager.alloc_head = NULL;
	manager.alloc_tail = NULL;

	for(int i = 0; i < NODENUM; i++){
		manager.alloc_used_list[i] = 0;
		manager.free_used_list[i] = 0;
	}

	// 할당되지 않은 메모리 초기화
	int pool = get_memory_pool(FREE);
	new = &(manager.free_memory_pool[pool]);
	new->offset = 0;
	new->pool = pool;
	new->size = PAGESIZE;
	new->next = NULL;
	manager.free_head = new;
	manager.free_tail = new;

	// 오류코드리턴 추가 하셈
	return 0;
}

int cleanup() // 메모리 관리자 정리
{
	Memory *cur, *next;

	// page 반납
	//memset(manager.page, 0, PAGESIZE);
	if(munmap(manager.page, PAGESIZE) == -1)
		return 1;

	// alloc list 메모리 정리
	cur = manager.alloc_head;
	while(cur != NULL)
	{
		manager.alloc_used_list[cur->pool] = UNUSED;
		next = cur->next;
		cur = NULL;
		cur = next;
	}

	// free list 메모리 정리
	cur = manager.free_head;
	while(cur != NULL)
	{
		manager.free_used_list[cur->pool] = UNUSED;
		next = cur->next;
		cur = NULL;
		cur = next;
	}

	return 0;
}

char* alloc(int size) // 메모리 할당
{
	char* mem;
	Memory* cur;

	// 오류 예외 처리
	if(size % MINALLOC != 0)
		return NULL;
	if(size > manager.total_size)
		return NULL;

	cur = manager.free_head;
	while(cur != NULL) 
	{
		if(cur->size >= size) { // 적절한 free를 찾으면 할당
			mem = manager.page + cur->offset;
			// alloc 메모리 관리
			int pool = get_memory_pool(ALLOC);
			new = &(manager.alloc_memory_pool[pool]);
			new->offset = cur->offset;
			new->pool = pool;
			new->size = size;
			new->data = mem;
			new->next = NULL;
			manager.total_size -= new->size;

			if(manager.alloc_head == NULL) {
				manager.alloc_head = new;
				manager.alloc_tail = new;
			}
			else {
				manager.alloc_tail->next = new;
				manager.alloc_tail = new;
			}

			// free 메모리 관리
			cur->offset = cur->offset + size;
			cur->size = cur->size - size;

			if(cur->size == 0) { // free가 없으면
				manager.free_head = NULL;
				manager.free_tail = NULL;
				manager.free_used_list[cur->pool] = 0;
				cur = NULL;
			}

			return mem;
		}
		cur = cur->next;
	}

	return NULL;
}

void dealloc(char* mem) // 메모리 반납
{
	Memory *cur, *cur2, *prev, *prev2;
	Memory *fore, *back, *back_prev, *fore_prev;

	cur = manager.alloc_head;
	while(cur != NULL) 
	{
		if(strcmp(cur->data, mem) == 0) { // 메모리를 찾으면 반납
			//memset(manager.page + cur->offset, 0, cur->size);
			manager.total_size += cur->size;
			fore_prev = prev2;

			// alloc 메모리가 1개인 경우 리셋 (여기서 free를 리셋을 안하고있긴함)
			if(manager.alloc_head->next == NULL) 
			{
				for(int i = 0; i < NODENUM; i++)
					manager.free_used_list[i] = UNUSED;
				int pool = get_memory_pool(FREE);
				new = &(manager.free_memory_pool[pool]);
				new->offset = 0;
				new->pool = pool;
				new->size = PAGESIZE;
				new->next = NULL;
				manager.free_head = new;
				manager.free_tail = new;
			}
			else // 그 외의 경우 합병 케이스 탐색
			{
				cur2 = manager.free_head;
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
					int pool = get_memory_pool(FREE);
					new = &(manager.free_memory_pool[pool]);
					new->offset = cur->offset;
					new->pool = pool;
					new->size = cur->size;
					new->next = NULL;

					if(manager.free_head == NULL) {
						manager.free_head = new;
						manager.free_tail = new;
					}
					else {
						manager.free_tail->next = new;
						new = manager.free_tail;
					}
				}
				else if(fore != NULL && back == NULL) { // 앞의 free와 합병인 경우
					fore->size += cur->size; 
				}
				else if(fore == NULL && back != NULL) { // 뒤의 free와 합병인 경우
					back->size += cur->size; 
					back->offset -= cur->size;
				}
				else if(fore != NULL && back != NULL) { // 쌍방향 합병인 경우
					fore->size += cur->size + back->size; 

					// back free 리스트에서 제거
					if(back == manager.free_head) {
						manager.free_head = back->next;
					}
					else if(back == manager.free_tail)  {
						manager.free_tail = back_prev;
						manager.free_tail->next = NULL;
					}
					else {
						back_prev->next = back->next;
					}
					manager.free_used_list[back->pool] = 0;
					back = NULL;
				}
			}

			// cur을 alloc  리스트에서 제거
			if(cur == manager.alloc_head){
				manager.alloc_head = cur->next;
			}
			else if(cur == manager.alloc_tail) {
				manager.alloc_tail = fore_prev;
				manager.alloc_tail->next = NULL;
			}
			else{ 
				fore_prev->next = cur->next;
			}
			manager.alloc_used_list[cur->pool] = UNUSED;
			cur = NULL;

			return ;
		}

		prev2 = cur;
		cur = cur->next;
	}
}

void print_alloc() // 디버깅용 함수(alloc 리스트 출력)
{
	Memory *cur;
	cur = manager.alloc_head;

	while(cur!=NULL){
		printf("alloc>> offset:%d size:%d\n", cur->offset,cur->size);
		cur = cur->next;
	}

}

void print_free() // 디버깅용 함수(free 리스트 출력)
{
	Memory *cur;
	cur = manager.free_head;

	while(cur!=NULL){
		printf("free>> offset:%d size:%d\n", cur->offset,cur->size);
		cur = cur->next;
	}
	printf("\n");

}
