//////////////////////////////////////////////////////////////////////////
//
// XMEM.H
//
// eXtended MEMory Shell
//
// Finding and exterminating C++'s memory allocation errors
// by keeping track of call's to malloc, calloc and free.
// Can swap itself from usage by controlling "memtrace"
//
// Program is public domain
// First published in Dr.Dobbs Journal, august 1989.
// Author: Randall Merilatt
// C++ Version: Edwig Huisman
//
//////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////
//
// FOR USE WITH YOUR CLASSES
//
//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
// Declare in the public part of your class (*.h)
#define DECLARE_XMEM_NEW          void* operator new(unsigned p_size,char* filename,int lineno);\
                                  void  operator delete(void* p_block);
// Include in the implementation part of your class (*.cpp)
#define DEFINE_XMEM_NEW(myclass)  void* myclass::operator new(unsigned p_size,char* filename,int lineno)\
                                  {\
                                    return xmem.x_malloc(p_size,filename,lineno);\
                                  }\
                                  void  myclass::operator delete(void* p_block)\
                                  {\
                                    xmem.x_free(p_block);\
                                  }
#else
#define DECLARE_XMEM_NEW
#define DEFINE_XMEM_NEW(myclass)
#endif

// MARKERS
#define OVHDSIZE 	2
#define FILLCHAR	'\377'

typedef	struct	alloc_entry
{
	size_t size;
	char*  ptr;
	char*  freed;
  char*  filename;
  int    lineno;
}
ALLOCATION;

typedef	struct	bucket
{
	struct bucket* next;
	short		       entries;
	ALLOCATION*    alloc;
}
BUCKET;

#define	NULL_BUCKET	((BUCKET *) 0)

class	XMem
{
public:
    XMem(const char *naam,int trace);
   ~XMem();
    // Interface - public methods
    void*  x_malloc(size_t blocksize,                       char* filename,int lineno);
    void*  x_calloc(unsigned int elements,size_t blocksize, char* filename,int lineno);
    void*  x_realloc(void *,size_t blocksize,               char* filename,int lineno);
    void   x_free(void *);
    void   x_chkfree(void);
    void*  x_strdup(void *s,char* filename,int lineno);
private:
    // Private methods
    char*  sto_ptr(char *p,size_t block,char* filename,int lineno);
    void   del_ptr(char *p);
    void   dump_block(ALLOCATION* p_alloc);
    void   trace(const char* p_format,...);
    // Data members
    int 	 m_memtrace;
    int 	 m_hashsize;
    int 	 m_bucketsize;
    char   m_naam[20];
    unsigned long m_overwrites;
    unsigned long m_freefree;
    unsigned long m_freebad;
    unsigned long m_freenul;
    unsigned long m_smaller;
    unsigned __int64 m_tot_mem;
    unsigned __int64 m_max_mem;
    BUCKET 	**m_ptrhash;
};

extern XMem xmem;

#ifndef XMEM_INTERNAL
#ifdef _DEBUG
#define malloc(size)        xmem.x_malloc(size,           __FILE__,__LINE__)
#define calloc(elem,size)   xmem.x_calloc((elem),(size),  __FILE__,__LINE__)
#define realloc(block,size) xmem.x_realloc((block),(size),__FILE__,__LINE__)
#define strdup(block)       xmem.x_strdup(block,          __FILE__,__LINE__)
#define free(block)         xmem.x_free(block)
#endif
#endif
