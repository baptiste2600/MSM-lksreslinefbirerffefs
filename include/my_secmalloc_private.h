#ifndef _SECMALLOC_PRIVATE_H
#define _SECMALLOC_PRIVATE_H

/*
 * Ici vous pourrez faire toutes les déclarations de variables/fonctions pour votre usage interne
 * */
void my_log(const char *fmt, ...);

//descripteur memoire
struct  descmem
{
    char *data;
    size_t size;
    size_t full_size;//avec canari
    char busy;
    char used;
};


void descmem_init(size_t idx, size_t sz);

void pool_data_init(struct descmem *dm);

void pool_metainf_init();

size_t descmem_first_free();

size_t descmem_first_notused();


size_t get_pool_metainf_size();

size_t get_pool_data_size();

size_t get_canary_size();



#if TEST
extern void *pool_data;
extern struct descmem *pool_metainf;
extern size_t size_pool_data;
extern size_t size_pool_metainf;
extern size_t size_canary;
#endif

#endif
