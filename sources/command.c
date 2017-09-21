#include "command.h"

#include <string.h>

void command_init(command_t* command, command_type_t type, char* reference, char* argument)
{
    command->type = type;

    strcpy(command->reference, reference);
    strcpy(command->argument, argument);
}

command_type_t command_get_type(command_t* command)
{
    return command->type;
}

char* command_get_reference(command_t* command)
{
    return command->reference;
}

char* command_get_value(command_t* command)
{
    return command->argument;
}
