#include "model/attribute.h"
#include <string.h>

void attribute_init(attribute_t* attribute, char* name, char* data_type, char* value)
{
    strncpy(attribute->name, name, MANIFEST_ITEM_NAME_SIZE);
    strncpy(attribute->data_type, data_type, MANIFEST_ITEM_DATA_TYPE_SIZE);
    strncpy(attribute->value, value, ATTRIBUTE_VALUE_SIZE);
}
