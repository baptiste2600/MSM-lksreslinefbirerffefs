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
        long rest =  descmem_first_notused();
        if(rest == -1)
        {
            /*TODO*/
        }

        //association du reste de la mémoire de "dm" à ce descripteur
        pool_metainf[rest].used = 1;
        pool_metainf[rest].busy = 0;
        pool_metainf[rest].full_size = dm->full_size - sz;
        pool_metainf[rest].size = dm->size - sz;
        pool_metainf[rest].data = dm->data + ( sz + get_canary_size() );

        //my_log("rest : %d dm data : %p data : %p full_sizedm %d\n", rest, dm->data, dm->data + pool_metainf[rest].full_size, dm->full_size);
    }
    else
    {
        /*TODO*/
        my_log("non used !");
    }

    dm->size = sz;
    dm->full_size = sz + get_canary_size();
    dm->busy = 1;

    // remplis le canari

    memset(dm->data + dm->size , 'A', get_canary_size());
}

void    pool_metainf_init()
{
    if(!pool_metainf)
    {
        pool_metainf = mmap(NULL, get_pool_metainf_size(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0 );
        memset(pool_metainf, 0, sizeof (*pool_metainf));
    }
}


void    pool_data_init(struct descmem *dm)
{
    if (!pool_data)
    {
        pool_data = mmap((void*)dm + size_pool_metainf, size_pool_data, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        dm[0].data = pool_data;
        dm[0].full_size = size_pool_data;
        dm[0].size = size_pool_data - get_canary_size();
        dm[0].busy = 0;
        dm[0].used = 1;
    }
}




long descmem_first_free(size_t sz)
{
    for (unsigned int i = 0; i < (size_pool_metainf / sizeof (struct descmem)); i += 1)
    {
        if (pool_metainf[i].used == 1 )
        {
            //my_log("index : %d busy : %d used : %d size : %d \n",i , pool_metainf[i].busy, pool_metainf[i].used, pool_metainf[i].size);
            if (pool_metainf[i].busy == 0 && pool_metainf[i].size > sz)
            {
                return i;
            }
        }
    }
    return -1;
}



long descmem_first_notused()
{
    for (unsigned int i = 0; i < (size_pool_metainf / sizeof (struct descmem)); i += 1)
    {
        if (pool_metainf[i].used == 0)
        {
            return i;
        }
    }
    return -1;
}

void test()
{
    for (unsigned int i = 0; i < 1000; i += 1)
        {
            if(pool_metainf[i].used == 1)
            {
                my_log("[descmem %d] busy : %d used : %d data : %p  size : %d full size : %d \n",i , pool_metainf[i].busy, pool_metainf[i].used, pool_metainf[i].data, pool_metainf[i].size, pool_metainf[i].full_size);
            }
        }
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
    
    my_log("\n ----------malloc (%d)----------- \n", size);

    //initialisation des pools, exécutée uniquement pour le premier malloc
    pool_metainf_init();
    pool_data_init(pool_metainf);


    //récupération de l'index du premier descripteur disponible dans le pool de méta-information
    long first = descmem_first_free(size);
    //my_log("index de first : %ld \n", first );
     my_log("first : %d \n", first);
    if(first == -1)
    {
        my_log("Pas de descripteur disponible \n");
        return NULL;
    }

    descmem_init(first, size);

    /*
        for (size_t  i = 0; i < pool_metainf[first].full_size ; i += 1)
    {
        my_log("[test de la mémoire] Add : %p Valeur : %c \n", &pool_metainf[first].data[i], pool_metainf[first].data[i]);
        my_log("\n"); 
    }
    */
    //test();
    my_log("[mémoire allouée] Add debut: %p Add fin: %p \n", pool_metainf[first].data, pool_metainf[first].data + pool_metainf[first].full_size);

    return pool_metainf[first].data;
}

void    my_free(void *ptr)
{
    my_log("\n ----------Free (%p)----------- \n",ptr);

    for (unsigned int i = 0; i < (size_pool_metainf / sizeof (struct descmem)); i += 1)
    {
        if (pool_metainf[i].data == ptr && ptr != NULL)
        {
            pool_metainf[i].busy = 0;
            my_log("[mémoire libérer] Add ptr: %p \n",ptr);
            return;
        }
    }
    my_log("[bloc data non trouvée] Add ptr: %p \n",ptr);
    return;
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
