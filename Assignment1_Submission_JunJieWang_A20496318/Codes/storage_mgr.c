#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "dberror.h"

void initStorageManager (void)
{

}


RC createPageFile (char *fileName)
{
    // open file
    FILE* file = fopen(fileName,"wb");
    if(file == NULL)
    {
        return RC_FILE_NOT_FOUND;
    }
    //new zero buffer
    char* zero_buffer = (char*)malloc(PAGE_SIZE);
    if(zero_buffer == NULL)
    {
        return RC_FILE_NOT_FOUND;
    }
    memset(zero_buffer,0, PAGE_SIZE );
    int succ = fwrite(zero_buffer, PAGE_SIZE, 1, file);
    free(zero_buffer);
    fclose(file);
    if(succ == 0)
    {
        return RC_WRITE_FAILED;
    }
    else
    {
        return RC_OK;
    }
    
}
RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
    FILE* f = fopen(fileName, "rb+");
    if(f == NULL)
    {
        return RC_FILE_NOT_FOUND;
    }
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // seek to the end of the file to calculate the file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f,0,SEEK_SET);

    fHandle->fileName = fileName;
    fHandle->curPagePos = 0;
    // calculate the page number of this file
    fHandle->totalNumPages = size / PAGE_SIZE;
    // book regist the file handle
    fHandle->mgmtInfo = (void*)f;
    
    return RC_OK;
    
}
/// @brief close the page file
/// @param fHandle 
/// @return 
RC closePageFile (SM_FileHandle *fHandle)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    fHandle->curPagePos = 0;
    fHandle->fileName = NULL;
    fHandle->totalNumPages = 0;
    FILE* f = (FILE*) fHandle->mgmtInfo;
    fclose(f);
    fHandle->mgmtInfo = NULL;
    return RC_OK;
}
/// @brief remove the page file
/// @param fileName 
/// @return 
RC destroyPageFile (char *fileName)
{
    // delete the flie
    int succ = remove(fileName);
    if(succ == 0)
    {
        return RC_OK;
    }
    else
    {
        return RC_FILE_NOT_FOUND;
    }
}



RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE* f = (FILE*)fHandle->mgmtInfo;
    if(f == NULL)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }
    if(pageNum < 0 || pageNum >= fHandle->totalNumPages)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }
    int succ = fseek(f, pageNum * PAGE_SIZE,SEEK_SET);
    fHandle->curPagePos = pageNum;
    if(succ != 0)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }
    int result = fread(memPage, PAGE_SIZE, 1, f);
    if(result > 0)
    {
        return RC_OK;
    }
    else
    {
        return RC_READ_NON_EXISTING_PAGE;
    }
    
}
int getBlockPos (SM_FileHandle *fHandle)
{
    return fHandle->curPagePos;
}
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    return readBlock(0, fHandle, memPage);
}
/// @brief read previous block of file
/// @param fHandle 
/// @param memPage 
/// @return 
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int prevPage = fHandle->curPagePos - 1;
    return readBlock(prevPage, fHandle, memPage);
}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return readBlock(fHandle->curPagePos, fHandle, memPage);

}
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    
    return readBlock(fHandle->curPagePos + 1, fHandle, memPage);

}
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int lastPage=  fHandle->totalNumPages - 1;
    return readBlock(lastPage, fHandle, memPage);
    
}


RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE* f = (FILE*)fHandle->mgmtInfo;
    if(f == NULL)
    {
        return RC_WRITE_FAILED;
    }
    if(pageNum < 0 || pageNum >= fHandle->totalNumPages)
    {
        return RC_WRITE_FAILED;
    }
    //seek to the target page num
    int succ = fseek(f,pageNum * PAGE_SIZE,SEEK_SET);
    if(succ != 0)
    {
        return RC_WRITE_FAILED;
    }
    // set to the current page pos
    fHandle->curPagePos = pageNum;
    // write file
    int result = fwrite(memPage, PAGE_SIZE, 1, f);
    if(result > 0)
        return RC_OK;
    else
        return RC_WRITE_FAILED;
    
}
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}


RC appendEmptyBlock (SM_FileHandle *fHandle)
{
    // invalid fHandle
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE* file = (FILE*)fHandle->mgmtInfo;
    if(file == NULL)
    {
        // file doesn't exist return failed
        return RC_WRITE_FAILED;
    }

    //new zero buffer
    char* zero_buffer = (char*)malloc(PAGE_SIZE);
    if(zero_buffer == NULL)
    {
        return RC_WRITE_FAILED;
    }
    memset(zero_buffer,0, PAGE_SIZE );
    // append the new block at the end of the file
    fHandle->totalNumPages += 1; 
    int newPagePos = fHandle->totalNumPages - 1;
    RC result = writeBlock(newPagePos, fHandle, zero_buffer);
    free(zero_buffer);
    if(result != RC_OK)
    {
        fHandle->totalNumPages -= 1;
    }
    
    return result;
}
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
    if(fHandle == NULL)
    {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE* file = (FILE*)fHandle->mgmtInfo;
    if(file == NULL)
    {
        return RC_WRITE_FAILED;
    }
    if(fHandle->totalNumPages >= numberOfPages)
    {
        return RC_OK;
    }
    int numPageToAppend = numberOfPages - fHandle->totalNumPages;
    for(int k = 0; k < numPageToAppend;++k)
    {
        RC result = appendEmptyBlock(fHandle);
        if(result != RC_OK)
        {
            return result;
        }
    }
    return RC_OK;

}


/*
int main()
{
    initStorageManager();
    char* PageFileName = "Page.dat";
    //createPageFile(PageFileName);
    SM_FileHandle fileHandle;
    SM_PageHandle pageHandle;
    openPageFile(PageFileName, &fileHandle);
    ensureCapacity(10, &fileHandle);
    
    closePageFile(&fileHandle);
    //printf("hello,world\n");
}
*/