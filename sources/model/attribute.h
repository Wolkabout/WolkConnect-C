#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "size_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[MANIFEST_ITEM_NAME_SIZE];
    char data_type[MANIFEST_ITEM_DATA_TYPE_SIZE];
    char value[READING_SIZE];
} attribute_t;

void attribute_init(attribute_t* attribute, char* name, char* data_type, char* value);

#ifdef __cplusplus
}
#endif

#endif // ATTRIBUTE_H