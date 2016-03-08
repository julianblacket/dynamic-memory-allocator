#include <stdio.h>
#include <stdlib.h>
#include "allocator.h"
#include <assert.h>

int main (int argc, char *argv[]) {

   vlad_init(512);
   void *firstRegion = vlad_malloc(240);
   assert(firstRegion != NULL);
   void *secondRegion = vlad_malloc(40);
   assert(secondRegion != NULL);
   vlad_free(firstRegion);
   



   return EXIT_SUCCESS;
}