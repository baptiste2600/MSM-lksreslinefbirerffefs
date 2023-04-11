// cette définition permet d'accéder à mremap lorsqu'on inclue sys/mman.h
#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include <sys/mman.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "my_secmalloc.h"
#include "my_secmalloc_private.h"

//Variables static

/*static*/ void *pool_data = 0;

/*static*/ struct descmem *pool_metainf = 0;


/*static*/ size_t size_pool_data = 504857600;//~500Mo

/*static*/ size_t size_pool_metainf = 104857600;//~100Mo

/*static*/ size_t size_canary = 10;


//Variable d'accés (pour les tests criterion)

size_t get_pool_metainf_size()
{   return size_pool_metainf;}

size_t get_pool_data_size()
{   return size_pool_data;}

size_t get_canary_size()
{   return size_canary;}


//fonctions init (constructeur)

void    descmem_init(size_t idx, size_t sz)
{
    struct descmem *dm = &pool_metainf[idx];
    
    if(dm->used)
    {
        //rechercher un descripteur non utilier
        size_t rest =  descmem_first_notused();

        //découpage 
        pool_metainf[rest].used = 1;
        pool_metainf[rest].busy = 0;
        pool_metainf[rest].full_size = dm->full_size - (sz + get_canary_size());
        pool_metainf[rest].data = dm->data + (sz + get_canary_size());
    }
    else
    {
        /*TODO*/
    }

    dm->size = sz;
    dm->full_size = sz + size_canary;
    dm->busy = 1;

    //remplire les canaris
    for (size_t i = 0; i < get_canary_size(); i += 1)
    {
        dm->data[dm->size + i] = 'A';
    }
}


void    pool_metainf_init()
{
    if(!pool_metainf)
    {
        pool_metainf = mmap(NULL, get_pool_metainf_size(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
    }
    memset(pool_metainf, 0, sizeof (*pool_metainf));
}


void    pool_data_init(struct descmem *dm)
{
    if (!pool_data)
    {
        pool_data = mmap((void*)dm + size_pool_metainf, size_pool_data, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    }

    dm[0].data = pool_data;
    dm[0].full_size = size_pool_data;
    dm[0].busy = 0;
    dm[0].used = 1;

}




size_t descmem_first_free()
{
    for (size_t i = 0; i < size_pool_metainf; i += 1)
    {
        if (pool_metainf[i].busy == 0 && pool_metainf[i].used == 1)
        {
            return i;
        }
    }
    return -1;
}



size_t descmem_first_notused()
{
    for (size_t i = 0; i < size_pool_metainf; i += 1)
    {
        if (pool_metainf[i].used == 0)
        {
            return i;
        }
    }
    return -1;
}



//--------------------------------



void    my_log(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    size_t sz = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    char *buf = alloca(sz + 2);

    va_start(ap, fmt);
    vsnprintf(buf, sz + 1, fmt, ap);
    va_end(ap);

    write(2, buf, strlen(buf));
}

//------------------------------------------

void    *my_malloc(size_t size)
{
    (void) size;
    return NULL;
}

void    my_free(void *ptr)
{
    (void) ptr;
}

void    *my_calloc(size_t nmemb, size_t size)
{
    (void) nmemb;
    (void) size;
    return NULL;
}

void    *my_realloc(void *ptr, size_t size)
{
    (void) ptr;
    (void) size;
    return NULL;

}

#ifdef DYNAMIC
/*
 * Lorsque la bibliothèque sera compilé en .so les symboles malloc/free/calloc/realloc seront visible
 * */

void    *malloc(size_t size)
{
    return my_malloc(size);
}
void    free(void *ptr)
{
    my_free(ptr);
}
void    *calloc(size_t nmemb, size_t size)
{
    return my_calloc(nmemb, size);
}

void    *realloc(void *ptr, size_t size)
{
    return my_realloc(ptr, size);
}

#endif