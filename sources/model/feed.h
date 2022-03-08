#ifndef FEED_H
#define FEED_H

#include "size_definitions.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[ITEM_NAME_SIZE];
    char reference[REFERENCE_SIZE];
    char unit[ITEM_UNIT_SIZE];

    feed_type_t feedType;
} feed_registration_t;

void initialize_feed(feed_registration_t* feed, char* name, const char* reference, char* unit, feed_type_t feedType);

#ifdef __cplusplus
}
#endif

#endif // FEED_H
