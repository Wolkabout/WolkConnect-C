#ifndef FEED_H
#define FEED_H

#include "size_definitions.h"
#include "types.h"
#include "utility/wolk_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[ITEM_NAME_SIZE];
    char reference[REFERENCE_SIZE];
    char unit[ITEM_UNIT_SIZE];

    feed_type_t feedType;
} feed_registration_t;

typedef struct {
    char data[FEEDS_MAX_NUMBER][FEED_ELEMENT_SIZE];
    uint16_t size;

    char reference[REFERENCE_SIZE];

    uint64_t utc;
} feed_t;

void feed_init(feed_t* feed, uint16_t feed_size, char* reference);

void feed_clear(feed_t* feed);

void feed_set_data(feed_t* feed, const char** data);
char** feed_get_data(feed_t* feed);

void feed_set_data_at(feed_t* feed, const char* data, size_t data_position);
char* feed_get_data_at(feed_t* feed, size_t data_position);

void feed_set_utc(feed_t* feed, uint64_t utc);
uint64_t feed_get_utc(feed_t* feed);

void initialize_registration_feed(feed_registration_t* feed, char* name, const char* reference, char* unit,
                                  feed_type_t feedType);

#ifdef __cplusplus
}
#endif

#endif // FEED_H
