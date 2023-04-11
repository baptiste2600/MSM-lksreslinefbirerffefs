#include <criterion/criterion.h>
#include <criterion/redirect.h>
#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "my_secmalloc.h"
#include "my_secmalloc_private.h"

/// 2H25

Test(mmap, simple) {
    // Question: Est-ce que printf fait un malloc ?
    printf("Ici on fait un test simple de mmap\n");
    char *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    cr_expect(ptr != NULL);
    int res = munmap(ptr, 4096);
    cr_expect(res == 0);
}

Test(log, test_log, .init=cr_redirect_stderr)
{
    my_log("coucou %d\n", 12);
    cr_assert_stderr_eq_str("coucou 12\n");

}

Test(canary, alloc)
{   
    // appel de malloc pour un bloc de data d'une taille de :
    size_t szdata = 12;
    // déterminer la quantité de ressource mémoire à réserver
    size_t size = szdata + get_canary_size();

    // allocation de la mémoire demandée avec le canari
    char *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    // nettoiyage de la memoire demandée
    memset(ptr, 0, szdata);

    // remplissage du canari
    for (size_t  i = 0; i < get_canary_size(); i += 1)
    {
        ptr[szdata + i] = 'A';
    }

    // test de la mémoire
    for (size_t  i = 0; i < size; i += 1)
    {
        if (i >= szdata)
        {
            cr_expect(ptr[i] == 'A');
        }
        else
        {
            cr_expect_null(ptr[i]);
        }

        //my_log("[test de la mémoire] Add : %p Valeur : %c \n", &ptr[szdata + i], ptr[szdata + i]);
        //my_log("\n");    
    }
}

Test(metainf, alloc)
{
    // initialisation du pool de meta information
    pool_metainf_init();

    // initialisation du pool de data
    pool_data_init(pool_metainf);

    //instanciation du premier descripteur
    long first = descmem_first_free();

    if(first == -1)
    {
        //pas de descripteur disponible
    }

    cr_expect(pool_metainf[first].used == 1);
    cr_expect(pool_metainf[first].busy == 0);
    cr_expect(pool_metainf[first].data == pool_data);

    descmem_init(first, 12);

    cr_assert(pool_metainf[first].size == 12);
    cr_expect(pool_metainf[first].full_size == 12 + get_canary_size());
    cr_expect(pool_metainf[first].data == pool_data);
    cr_expect(pool_metainf[first].busy == 1);

    // test de la mémoire du pool de data (control canari + data)
    for (size_t  i = 0; i < pool_metainf[first].full_size ; i += 1)
    {
        //canari
        if (i >= pool_metainf[first].size)
        {
            cr_expect(pool_metainf[first].data[i] == 'A');
        }
        else
        {
            cr_expect_null(pool_metainf[first].data[i]);
        } 

        //my_log("[test de la mémoire] Add : %p Valeur : %c \n", &pool_metainf[first].data[i], pool_metainf[first].data[i]);
        //my_log("\n"); 
    }

    size_t rest = descmem_first_free();

    cr_assert(pool_metainf[rest].data == pool_data + pool_metainf[first].full_size);
}
