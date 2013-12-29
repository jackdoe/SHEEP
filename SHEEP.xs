#ifdef __cplusplus
extern "C" {
#endif

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"
#undef do_open
#undef do_close

#ifdef __cplusplus
}
#endif

#include "cpp/Sheep.hpp"

MODULE = SHEEP		PACKAGE = SHEEP		

Sheep *
Sheep::new(char *_root, int _shards)

void
Sheep::DESTROY()

int
Sheep::index(AV *docs)

SV*
Sheep::search(SV *query, int n)
