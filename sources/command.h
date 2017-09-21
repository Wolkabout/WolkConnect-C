#ifndef COMMAND_H
#define COMMAND_H

#include "size_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_TYPE_STATUS = 0,
    COMMAND_TYPE_SET,

    COMMAND_TYPE_UNKNOWN
} command_type_t;

typedef struct {
    command_type_t type;

    char reference[MANIFEST_ITEM_MAX_REFERENCE_SIZE];
    char argument[COMMAND_MAX_ARGUMENT_SIZE];
} command_t;

void command_init(command_t* command, command_type_t type, char* reference, char* argument);

command_type_t command_get_type(command_t* command);
char* command_get_reference(command_t* command);
char* command_get_value(command_t* command);

#ifdef __cplusplus
}
#endif

#endif
