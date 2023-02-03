// Program is public domain
// First published in Dr.Dobbs Journal, august 1989.
// Author: Randall Merilatt
// C++ Version: Edwig Huisman
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strsafe.h>

#define XMEM_INTERNAL
#include "xmem.h"

#pragma warning (disable: 4996)
#pragma warning (disable: 4995)

// The one-and-only object
// Default for debug-builds is to have memtrace = 1
XMem xmem("SQLComponents",1);

XMem::XMem(const char *naam,int trace)
	   :m_hashsize(311) // Must be a prime number!!
	   ,m_bucketsize(10)
	   ,m_overwrites(0)
	   ,m_freefree(0)
	   ,m_freebad(0)
	   ,m_freenul(0)
	   ,m_smaller(0)
     ,m_tot_mem(0)
     ,m_max_mem(0)
{
  // MEMtrace controls the whole eXtended MEMory shell
  // m_memtrace=0 : Just make calls to malloc and free from library
  // m_memtrace=1 : traces allocations and deallocations
  // m_memtrace=2 : Alsoo trace accessing of free memory via mem-copies
  //                USE WITH EXTREME CARE (NEEDS LOTS OF MEMORY!!!!!)
	m_memtrace = trace;
	m_ptrhash  = (BUCKET **) 0;
  strncpy_s(m_naam,20,naam,18);
}

XMem::~XMem()
{
  x_chkfree();
}

char *
XMem::sto_ptr(char *p,size_t b,char* filename,int lineno)
{
	BUCKET *bp,*bq;

	if(!m_ptrhash)
	{
		m_ptrhash = (BUCKET **) calloc(m_hashsize, sizeof(BUCKET *));
		if(!m_ptrhash)
		{
			return NULL;
		}
	}
	DWORD_PTR bno = ((DWORD_PTR) p % m_hashsize);
	for(bq = bp = m_ptrhash[bno];bp && bp->entries == m_bucketsize; bp = bp->next)
	{
		bq= bp;
	}
	if(bp == NULL_BUCKET) 
	{
		if((bp=(BUCKET *)malloc(sizeof(BUCKET)))==NULL)
		{
			return(NULL);
		}
		bp->next = NULL_BUCKET;
		bp->entries = 0;
		if(bq)
		{
			bq->next = bp;
		}
		else
		{
			m_ptrhash[bno] = bp;
		}
		bp->alloc = (ALLOCATION *)calloc(m_bucketsize,sizeof(ALLOCATION));
	}
	bno = bp->entries++;
  if(bp->alloc)
  {
	  bp->alloc[bno].ptr      = p;
	  bp->alloc[bno].freed    = NULL;
	  bp->alloc[bno].size     = b;
    bp->alloc[bno].filename = filename;
    bp->alloc[bno].lineno   = lineno;
  }
  m_tot_mem += b;
  if(m_tot_mem > m_max_mem)
  {
    m_max_mem = m_tot_mem;
  }
	return p;
}

void 
XMem::del_ptr(char *p)
{
	unsigned int gap;
	BUCKET *bp,*bq;
	DWORD_PTR bno = (int)((DWORD_PTR)p % m_hashsize);
	for(bq=NULL_BUCKET,bp=m_ptrhash[bno];bp;bp=bp->next) 
	{
		for(int i=0;i<bp->entries;++i) 
		{
			if(bp->alloc[i].ptr == p) 
			{
        m_tot_mem -= bp->alloc[i].size;
				for(gap=(unsigned int)(bp->alloc[i].size-OVHDSIZE); gap < bp->alloc[i].size;++gap) 
				{
					if(p[gap] != FILLCHAR) 
					{
						if(m_memtrace > 0)
						{
							trace("Overwrite: %s\n",p);
						}
						++m_overwrites;
						break;
					}
				}
				if(m_memtrace == 1) 
				{
					if(--bp->entries==0) 
					{
						if(bq)
						{
							bq->next = bp->next;
						}
						else
						{
							m_ptrhash[bno] = bp->next;
						}
						free(bp->alloc);
						free(bp);
					}
					else 
					{
						if(i < bp->entries) 
						{
							bp->alloc[i] = bp->alloc[bp->entries];
						}
					}
					free(p);
				}
				else 
				{
					if(bp->alloc[i].freed)
					{
						++m_freefree;
					}
					else 
					{
						if((bp->alloc[i].freed = (char *)malloc(bp->alloc[i].size))!=NULL)
						{
							memcpy(bp->alloc[i].freed,bp->alloc[i].ptr,
							bp->alloc[i].size);
						}
					}
				}
				return;	/* normal return */
			}
		}
		bq = bp;
	}
	if(!bp)
	{
		++m_freebad;
	}
}

void *
XMem::x_malloc(size_t b,char* filename,int lineno)
{
	char *mptr;
	if(m_memtrace) 
	{
		b += OVHDSIZE;
    if((mptr = (char *)::malloc((size_t) b)) != NULL)
		{
			memset(mptr+b-OVHDSIZE,FILLCHAR,OVHDSIZE);
			mptr = sto_ptr(mptr,b,filename,lineno);
		}
	}
	else
	{
    mptr= (char *) ::malloc((size_t) b);
	}
	return (mptr);
}

void *
XMem::x_calloc(unsigned int elements,size_t blocksize,char* filename,int lineno)
{
	size_t amt;
	char *mptr;

	if((mptr = (char *)x_malloc(amt = (elements * blocksize),filename,lineno)) != NULL) 
	{
		memset(mptr,0,amt);
	}
	return (mptr);
}

void 
XMem::x_free(void *p)
{
	if(p==NULL)
	{
		++m_freenul;
	}
	else 
	{
		if(m_memtrace)
		{
			del_ptr((char *)p);
		}
		else
		{
			free(p);
		}
	}
}

void*
XMem::x_realloc(void *p,size_t s,char* filename,int lineno)
{
	BUCKET *bp;
	DWORD_PTR size = 0;
	void *pnt;

	if(m_memtrace)
	{
		DWORD_PTR bno = ((DWORD_PTR)p % m_hashsize);
		for(bp=m_ptrhash[bno];bp;bp=bp->next) 
		{
			for(int i=0;i<bp->entries;++i) 
			{
				if(bp->alloc[i].ptr == p) 
				{
					size = bp->alloc[i].size;
				}
			}
		}
		pnt = x_malloc(s,filename,lineno);
		if(size > s)
		{
			size = s;
			m_smaller++;
		}
		memcpy(pnt,p,size); 	// Copy the smallest block 
		x_free(p);
		return pnt;
	}
	return (void *)realloc(p,s);
}

void *
XMem::x_strdup(void *s,char* filename,int lineno)
{
	unsigned int l = (unsigned int)strlen((char *)s);
	void    *p = x_malloc((size_t)l+1,filename,lineno);
	
	strcpy((char *)p,(char *)s);
	return p;
}

void
XMem::dump_block(ALLOCATION* p_alloc)
{
	trace("XMEM Dump size: %d\n",p_alloc->size);
  trace("XMEM Pointer  : %X\n",p_alloc->ptr);
  trace("XMEM Location : %s [%d]\n",p_alloc->filename,p_alloc->lineno);
  trace("XMEM Dump data: ");
  size_t size  = p_alloc->size;
  char*  block = p_alloc->ptr;
  while(size--)
  {
    trace("0%02X",*block++);
  }
  trace("\n");
}

void 
XMem::x_chkfree(void)
{
	int bno,i;
	ALLOCATION *ap;
	BUCKET *bp,*bq;
	unsigned int changed = 0;
	unsigned int unfreed = 0;
	long error = 0;

	if(m_memtrace && m_ptrhash) 
	{
		for(bno=0;bno<m_hashsize;++bno) 
		{
			for(bp=m_ptrhash[bno];bp;bp=bq) 
			{
				for(i=0;i<bp->entries;++i) 
				{
					ap = &bp->alloc[i];
					if(m_memtrace==2 && ap->freed) 
					{
						if(memcmp(ap->ptr,ap->freed,ap->size))
						{
							++changed;
						}
					}
					++unfreed;
					if(m_memtrace > 0)
					{
						dump_block(ap);
					}
					free(ap->ptr);
				}
				bq = bp->next;
				free(bp->alloc);
				free(bp);
			}
		}
		free(m_ptrhash);
		m_ptrhash = (BUCKET **) 0;
	}
	/* Runtime error reports	*/
	if(m_memtrace > 0)
	{
    if(m_overwrites || m_freefree || m_freebad || m_freenul || m_smaller || changed || unfreed)
    {
      trace("XMEM: Runtime eXtended MEMory allocation shell\n");
      trace("XMEM: Maximum memory used: %I64u\n",m_max_mem);
      trace("XMEM: Check: %s\n",m_naam);
		  if(m_overwrites)
		  {
			  trace("XMEM: Overwritten memory areas   [%lu]\n",m_overwrites);
			  error += m_overwrites;
		  }
		  if(m_freefree)
		  {
			  trace("XMEM: Free already free pointers [%lu]\n",m_freefree);
			  error += m_freefree;
		  }
		  if(m_freebad)
		  {
			  trace("XMEM: Tried to free bad pointers [%lu]\n",m_freebad);
			  error += m_freebad;
		  }
		  if(m_freenul)
		  {
			  trace("XMEM: Tried to free NULL pointer [%lu]\n",m_freenul);
			  error += m_freenul;
		  }
		  if(m_smaller)
		  {
			  trace("XMEM: reallocs toke m_smaller space [%lu]\n",m_smaller);
			  error += m_smaller;
		  }
		  /* Clearup errors		*/
		  if(changed)
		  {
			  trace("XMEM: Blocks changed after free  [%u]\n",changed);
			  error += changed;
		  }
		  if(unfreed)
		  {
			  trace("XMEM: Blocks released (not free) [%u]\n",unfreed);
			  error += unfreed;
		  }
		  if(error)
		  {
			  trace("XMEM: Total number of errors [%lu]\n",error);
		  }
    }
	}
}

void
XMem::trace(const char* p_format,...)
{
  char buffer[1024];

  va_list argptr;
  va_start(argptr, p_format);
  StringCchVPrintf(buffer,1024,p_format, argptr);
  va_end(argptr);

  OutputDebugString(buffer);
}