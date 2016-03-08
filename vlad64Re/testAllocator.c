#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "allocator.h"

int main (int argc, char * argv[]) {

   //Testing that the correct pointer has been passed to user.
   vlad_init(1000);
   u_int32_t * first = vlad_malloc(40);
   assert(first[-4] == 0xBEEFDEAD);
   vlad_free(first);
   u_int32_t * second = vlad_malloc(-1);
   assert(second == NULL);

   u_int32_t * third = vlad_malloc(1000);
   assert(third == NULL);

   //Testing freeing function
   vlad_end();
   u_int32_t * null = vlad_malloc(40);
   assert(null==NULL);

   printf("All tests passed... you are aweseome!\n");
}