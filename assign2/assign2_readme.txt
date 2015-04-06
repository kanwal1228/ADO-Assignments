Read me - Buffer Manager

Contents
- Buffer Manager Design and Implementation overview:
- Methods Description

Buffer Manager Design and Implementation overview:
--------------------------------------------------

The buffer Manager is implemented using a doubly linked list. The buffer created is stored using the structure declared "List"
and each element in the list is declared using the structure "List_ele". The List_ele contains three pointers, two integers and 
one boolean variable. Among the three pointers one points the data and the other two point towards the next and previous linked list.
The two integers keep track of the page number and the fix count. The Boolean parameter keeps record if the pagefile in the buffer has
been edited. 

The List to implement buffer keeps the record of the total size of the buffer, pointers for first and last elements in the list and, 
the total number of records read and written from the buffer.

The buffer manager in this assignment has been implemented using the FIFO and LRU strategies. The FIFO uses the first and last pointer
values in the List Structure to keep record for FIFO implementation. The element pointed as the first in the buffer is removed if a new 
record is to be entered in the buffer. The new record is entered at the end of the list and the second record in the list is now refered 
as the first record.

The LRU also uses the similar logic as of FIFO. The linked list of the buffer maintains the record which comes in first and last. 
In the LRU the record in the list which has been used recently is assigned the last position of the List. 
Hence the Least recently used is always at the front of the list and is removed if a new records wants to be put in the buffer.


Method Descriptions:
--------------------

1. RC updateDTrecord (BM_BufferPool *const bm,BM_PageHandle *dtyRecord):

This method is used to write the dirty record on the list onto the disk. It takes the address of the buffer pool and the page handle
of the dirty record as the arguments. The method creates a temporary file and copies all the pages into it from the original file except the page which is marked as dirty.
The dirty marked record is written from the buffer. Finally, the pointer of the original file is pointed towards the temprory file.

2. void display_List(List *list):

This method is used to used to diaply the list of elements in thebuffer. It is used for testing purpose only.

3. void push(List *const list,List_ele *element,int MAX_BUFF)

This method is used to push the page to the buffer list using FIFO Strategy. It takes the buffer list, the page and the size of the maximum buffer as the argument. The method initially checks if the buffer is full. It will append the page in the buffer list if the buffer is not full otherwise it will pop the first record and append the new record.

4. void push_LRU(List *const list,List_ele *element,int MAX_BUFF)

This method is used to push the page to the buffer list using the LRU Strategy. It takes the buffer list, the page and the size of the maximum buffer as the argument. The method initially checks if the buffer is full. It will append the page in the buffer list if the buffer is not full otherwise it will pop the first record and append the new record.

5. List_ele* search_IndexRec(List *list,int index)

This method is used to to find the an element in the list based on the index provided for the list. It is used for testing purpose only.

6. RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		  const int numPages, ReplacementStrategy strategy,
		  void *stratData)

This method is used to initialize the buffer pool. It takes buffer pool, page filename, number of pages, replacement strategy and strat data as the arguments.

7. RC shutdownBufferPool(BM_BufferPool *const bm)

This method is used to destroy the buffer pool. It takes the pointer to the buffer pool as the argument. It writes all the dirty records in the buffer pool to the disk and then destroy the pool.

8. RC forceFlushPool(BM_BufferPool *const bm)

This method is used to write the dirty pages in the buffer pool to the disk. It takes pointer to the buffer pool as the argument.

9. List_ele* search_element(List *list ,int pageNum)

This method is used to search the buffer pool for a particluar page. It takes the pointer to buffer pool and the page number as the arguments. It traverses through the list and look for the page number passed in the argument.

10. List_ele* fetchPage(BM_BufferPool *const bm, PageNumber pageNum)

This method is used to fetch the page from the disk. It takes the pointer to buffer pool and the page number as the arguments. 

11. RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)

This method is used to mark a page dirty on the buffer. It takes the pointer to buffer pool and page handle as the arguments.

12. RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)

This method unpins the page on the buffer pool. It takes the pointer to buffer pool and page handle as the arguments.

13. RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)

This method writes the page to the disk. It takes the pointer to buffer pool and page handle as the arguments.

14. RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
	    const PageNumber pageNum)

This method pins the page. It takes the pointer to buffer pool, page handle and the page number as the arguments. The method first looks for the page needed to be pinned in the buffer pool. If the page is in the pool it will look for what strategy is needs to be followed. It will then pin the page. If the pool does not have the page it will fetch the page from the disk and load it in the buffer.

15. PageNumber *getFrameContents (BM_BufferPool *const bm)

This method returns all the pages in the buffer. It takes the pointer to buffer pool as the argument.

16. bool *getDirtyFlags (BM_BufferPool *const bm)

This method returns the list of dirty pages on the buffer pool. It takes the pointer to buffer pool as the argument. 

17. int *getFixCounts (BM_BufferPool *const bm)

This method returns the list of pages with the number of hits per page. It takes the pointer to the buffer pool as the argument. 

18. int getNumReadIO (BM_BufferPool *const bm)

This method returns the number of pages read on the buffer. It takes the pointer to the buffer pool as the argument. 

19. int getNumWriteIO (BM_BufferPool *const bm)

This method returns the number of pages written on the buffer. It takes the pointer to the buffer pool as the argument. 

20.int insertAllowed(List *list,int index,int MAXBUFF)

This method is used to identify whether the insert operation is allowed at the given index.

21.int getLRUPosIns(List *list)

This is method is used to find the postion where the insert operation should be performed in LRU strategy.

22.RC FIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)

this method performs the pinning operation based on the FIFO strategy.

23.RC LRU(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
This is method is used to perfrom the pinning operation based on the LRU strategy.