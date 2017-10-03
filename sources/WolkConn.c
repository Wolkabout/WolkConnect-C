#include <stdarg.h>
#include "WolkConn.h"
#include "MQTTPacket.h"
#include "parser.h"

#define TIMEOUT_STEP 500000

#define NON_EXISTING "N/A"

#define BOOL_FALSE "false"
#define BOOL_TRUE "true"

#define KEEPALIVE_INTERVAL 60
#define CONFIG_PATH "config/"
#define SENSOR_PATH "sensors/"

#define RADINGS_PATH "readings/"

#define ACTUATORS_STATUS_PATH "actuators/status/"
#define ACTUATORS_COMMANDS "actuators/commands/"


#define SET_COMMAND "SET"
#define STATUS_COMMAND "STATUS"

static WOLK_ERR_T wolk_subscribe (wolk_ctx_t *ctx, const char *topic);

WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *serial, const char *password)
{
    wolk_queue_init (&ctx->actuator_queue);
    wolk_queue_init (&ctx->config_queue);
    initialize_parser(&ctx->wolk_parser, ctx->parser_type);
    ctx->readings_index = 0;

    memset (ctx->serial, 0, SERIAL_SIZE);
    strcpy (ctx->serial, serial);

    memset (ctx->sub_topic, 0, TOPIC_SIZE);

    strcpy(ctx->sub_topic, CONFIG_PATH);
    strcat(ctx->sub_topic, serial);

    MQTTPacket_connectData data_temp = MQTTPacket_connectData_initializer;

    ctx->data = data_temp;
    //ctx->data = MQTTPacket_connectData_initializer;
    int rc = 0;
    ctx->sock = 0;
    // ToDo no Hardcoded values
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int msgid = 1;
    MQTTString topicString = MQTTString_initializer;
    int req_qos = 0;

    int len = 0;
    ctx->iof.send = snd_func;
    ctx->iof.recv = rcv_func;

    ctx->sock = transport_open(&ctx->iof);
    if(ctx->sock < 0)
        return ctx->sock;

    ctx->mqtt_transport.sck = &ctx->sock;
    ctx->mqtt_transport.getfn = transport_getdatanb;
    ctx->mqtt_transport.state = 0;
    ctx->data.clientID.cstring = serial;
    ctx->data.keepAliveInterval = KEEPALIVE_INTERVAL;
    ctx->data.cleansession = 1;
    ctx->data.username.cstring = serial;
    ctx->data.password.cstring = password;


    len = MQTTSerialize_connect(buf, buflen, &ctx->data);
    /* This one blocks until it finishes sending, you will probably not want this in real life,
    in such a case replace this call by a scheme similar to the one you'll see in the main loop */
    rc = transport_sendPacketBuffer(ctx->sock, buf, len);


    printf("MQTT connected\n");

    if (ctx->parser_type == PARSER_TYPE_MQTT)
    {
        /* subscribe */
        topicString.cstring = ctx->sub_topic;
        len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);

        transport_sendPacketBuffernb_start(ctx->sock, buf, len);
        while((rc=transport_sendPacketBuffernb(ctx->sock)) != TRANSPORT_DONE);
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_set_parser (wolk_ctx_t *ctx, parser_type_t parser_type)
{
    ctx->parser_type = parser_type;

    return W_FALSE;

}

WOLK_ERR_T wolk_subscribe (wolk_ctx_t *ctx, const char *topic)
{
    unsigned char buf[READINGS_MQTT_SIZE];
    int msgid = 1;
    int len=0;
    int req_qos = 0;
    int rc = 0;
    MQTTString topicString = MQTTString_initializer;

    memset (buf, 0, READINGS_MQTT_SIZE);


    /* subscribe */
    topicString.cstring = topic;
    len = MQTTSerialize_subscribe(buf, READINGS_MQTT_SIZE, 0, msgid, 1, &topicString, &req_qos);

    transport_sendPacketBuffernb_start(ctx->sock, buf, len);
    while((rc=transport_sendPacketBuffernb(ctx->sock)) != TRANSPORT_DONE);

    return W_FALSE;
}

WOLK_ERR_T wolk_set_actuator_references (wolk_ctx_t *ctx, int num_of_items,  const char *item, ...)
{
    va_list argptr;
    char pub_topic[TOPIC_SIZE];
    int i;
    if (ctx->parser_type == PARSER_TYPE_JSON)
    {


        va_start( argptr, item );

        const char* str = item;
        //while ( str != NULL ) // terminate if end of argument list
        for (i=0;i<num_of_items;i++)
        {
            printf( "%s\n", str );
            memset (pub_topic, 0, TOPIC_SIZE);

            strcpy(pub_topic,ACTUATORS_COMMANDS);
            strcat(pub_topic,ctx->serial);
            strcat(pub_topic,"/");
            strcat(pub_topic,str);

            if (wolk_subscribe(ctx,pub_topic)!=W_FALSE)
            {
                printf ("Error in subscribing\n");
                return W_TRUE;
            }

            str = va_arg( argptr, const char* );
        }
        va_end( argptr );
    }

    return W_FALSE;
}



WOLK_ERR_T wolk_receive (wolk_ctx_t *ctx, unsigned int timeout)
{
    int rc = 0;
    int i;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int timeout_counter = 0;
    int timeout_microseconds = timeout * TIMEOUT_STEP * 2;
    actuator_command_t commands_buffer[128];

    while (timeout_counter != timeout_microseconds)
    {
        if((rc=MQTTPacket_readnb(buf, buflen, &(ctx->mqtt_transport))) == PUBLISH){
            unsigned char dup;
            int qos;
            unsigned char retained;
            unsigned short msgid;
            int payloadlen_in;
            unsigned char* payload_in;
            MQTTString receivedTopic;
            char reference[STR_64];
            memset (reference, 0, STR_64);

            rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                                         &payload_in, &payloadlen_in, buf, buflen);
            if (rc == -1)
            {
                return W_TRUE;
            }


            if (ctx->parser_type == PARSER_TYPE_JSON)
            {
                char *end_ptr = strstr (receivedTopic.lenstring.data, "{");
                if (end_ptr != NULL)
                {
                    char *start_ptr = strrchr(receivedTopic.lenstring.data, '/');
                    if (start_ptr != NULL)
                    {

                        strncpy(reference, start_ptr+1, end_ptr-start_ptr-1);
                    }

                }
            }



            size_t num_deserialized_commands = parser_deserialize_commands(&ctx->wolk_parser, payload_in, payloadlen_in, &commands_buffer[0], 128);
            printf("Received commands buffer after command deserialization: %s\n\n", payload_in);

            for (i = 0; i < num_deserialized_commands; ++i) {
                actuator_command_t* command = &commands_buffer[i];

                printf("Command type: ");
                switch(actuator_command_get_type(command))
                {
                case ACTUATOR_COMMAND_TYPE_STATUS:
                    if (ctx->parser_type==PARSER_TYPE_MQTT)
                    {
                        wolk_queue_push(&ctx->actuator_queue, actuator_command_get_reference(command), STATUS_COMMAND, NON_EXISTING);
                    } else if (ctx->parser_type==PARSER_TYPE_JSON)
                    {
                        wolk_queue_push(&ctx->actuator_queue, reference, STATUS_COMMAND, NON_EXISTING);
                    }
                    break;

                case ACTUATOR_COMMAND_TYPE_SET:
                    if (ctx->parser_type==PARSER_TYPE_MQTT)
                    {
                        wolk_queue_push(&ctx->actuator_queue,  actuator_command_get_reference(command), SET_COMMAND, actuator_command_get_value(command));
                    } else if (ctx->parser_type==PARSER_TYPE_JSON)
                    {
                        wolk_queue_push(&ctx->actuator_queue,  reference, SET_COMMAND, actuator_command_get_value(command));
                    }
                    break;

                case ACTUATOR_COMMAND_TYPE_UNKNOWN:
                    printf("UNKNOWN\n");
                    break;
                }
            }

        }
        usleep(TIMEOUT_STEP);
        timeout_counter += TIMEOUT_STEP;
        //printf ("Timeout counter value %d\n", timeout_counter);
    }
    return W_FALSE;
}

WOLK_ERR_T wolk_read_actuator (wolk_ctx_t *ctx, char *command, char *reference, char *value)
{
    if (wolk_queue_pop(&ctx->actuator_queue, reference, command, value) != Q_FALSE)
    {
        return W_TRUE;
    }

    return W_FALSE;
}

WOLK_ERR_T wolk_read_config (wolk_ctx_t *ctx, char *command, char *reference, char *value)
{
    return W_FALSE;
}

// ToDo: Set RTC

WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value, uint32_t utc_time)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_init(&ctx->readings[ctx->readings_index], &string_sensor);
    reading_set_data(&ctx->readings[ctx->readings_index], value);
    reading_set_rtc(&ctx->readings[ctx->readings_index], utc_time);

    ctx->readings_index++;

    return W_FALSE;
}

WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value, uint32_t utc_time)
{
    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    char value_str[STR_64];
    memset (value_str, 0, STR_64);
    sprintf(value_str, "%f", value);

    reading_init(&ctx->readings[ctx->readings_index], &numeric_sensor);
    reading_set_data(&ctx->readings[ctx->readings_index], value_str);
    reading_set_rtc(&ctx->readings[ctx->readings_index], utc_time);

    ctx->readings_index++;

    return W_FALSE;
}

WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t *ctx,const char *reference,bool value, uint32_t utc_time)
{
    manifest_item_t bool_sensor;
    manifest_item_init(&bool_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);


    reading_init(&ctx->readings[ctx->readings_index], &bool_sensor);
    if (value == true)
    {
        reading_set_data(&ctx->readings[ctx->readings_index], BOOL_TRUE);
    } else
    {
        reading_set_data(&ctx->readings[ctx->readings_index], BOOL_FALSE);
    }
    reading_set_rtc(&ctx->readings[ctx->readings_index], utc_time);

    ctx->readings_index++;

    return W_FALSE;
}

WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx)
{
    reading_clear_array(ctx->readings, ctx->readings_index);

    ctx->readings_index = 0;

    return W_FALSE;
}

WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx)
{
    unsigned char buf[READINGS_MQTT_SIZE];
    int len;
    char readings_buffer[READINGS_BUFFER_SIZE];
    memset (readings_buffer, 0, READINGS_BUFFER_SIZE);
    memset (buf, 0, READINGS_MQTT_SIZE);

    if (ctx->parser_type == PARSER_TYPE_MQTT )
    {
        MQTTString topicString = MQTTString_initializer;

        char pub_topic[TOPIC_SIZE];
        memset (pub_topic, 0, TOPIC_SIZE);

        strcpy(pub_topic,SENSOR_PATH);
        strcat(pub_topic,ctx->serial);

        topicString.cstring = pub_topic;

        size_t serialized_readings = parser_serialize_readings(&ctx->wolk_parser, &ctx->readings[0], ctx->readings_index, readings_buffer, READINGS_BUFFER_SIZE);

        wolk_clear_readings (ctx);

        printf("%lu reading(s) serialized:\n%s\n\n", serialized_readings, readings_buffer);

        len = MQTTSerialize_publish(buf, READINGS_MQTT_SIZE, 0, 0, 0, 0, topicString, (unsigned char*)readings_buffer, strlen(readings_buffer));
        transport_sendPacketBuffernb_start(ctx->sock, buf, len);

        switch(transport_sendPacketBuffernb(ctx->sock)){
        case TRANSPORT_DONE:
            break;
        case TRANSPORT_ERROR:
            return W_TRUE;
        case TRANSPORT_AGAIN:
            return W_TRUE;
        }
    } else if (ctx->parser_type == PARSER_TYPE_JSON)
    {
        int i=0;
        for (i=0; i<ctx->readings_index; i++)
        {

            char* value = reading_get_data(&ctx->readings[i]);

            if (wolk_publish_single (ctx,ctx->readings[i].manifest_item.reference,value, ctx->readings[i].manifest_item.data_type, ctx->readings[i].rtc) != W_FALSE)
            {
                return W_TRUE;
            }
        }

        wolk_clear_readings (ctx);


    }

    return W_FALSE;
}

WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *value, data_type_t type, uint32_t utc_time)
{
    unsigned char buf[READINGS_MQTT_SIZE];
    int len;
    int buflen = sizeof(buf);
    parser_t parser;
    reading_t readings;
    char readings_buffer[READINGS_BUFFER_SIZE];
    memset (readings_buffer, 0, READINGS_BUFFER_SIZE);
    memset (buf, 0, READINGS_MQTT_SIZE);
    initialize_parser(&parser, ctx->parser_type);

    MQTTString topicString = MQTTString_initializer;
    char pub_topic[TOPIC_SIZE];
   // topicString.cstring = ctx->pub_topic;
    memset (pub_topic, 0, TOPIC_SIZE);


    strcpy(pub_topic,RADINGS_PATH);
    strcat(pub_topic,ctx->serial);
    strcat(pub_topic,"/");
    strcat(pub_topic,reference);

    topicString.cstring = pub_topic;

    reading_set_rtc(&readings, utc_time);

    if (type==DATA_TYPE_STRING)
    {
        manifest_item_t string_sensor;
        manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

        reading_init(&readings, &string_sensor);
        reading_set_data(&readings, value);
    } else if (type==DATA_TYPE_BOOLEAN)
    {
        manifest_item_t bool_sensor;
        manifest_item_init(&bool_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_BOOLEAN);
        reading_init(&readings, &bool_sensor);
        reading_set_data(&readings, value);
    } else if (type==DATA_TYPE_NUMERIC)
    {
        manifest_item_t numeric_sensor;
        manifest_item_init(&numeric_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);
        reading_init(&readings, &numeric_sensor);
        reading_set_data(&readings, value);
    }


    // ToDo: What if reading buffer is already full?
    size_t serialized_readings = parser_serialize_readings(&parser, &readings, 1, readings_buffer, READINGS_BUFFER_SIZE);

    printf("%lu reading(s) serialized:\n%s\n\n", serialized_readings, readings_buffer);

    len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)readings_buffer, strlen(readings_buffer));
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    switch(transport_sendPacketBuffernb(ctx->sock)){
        case TRANSPORT_DONE:
            printf("Published\n");
            break;
        case TRANSPORT_ERROR:
            return W_TRUE;
        case TRANSPORT_AGAIN:
            return W_TRUE;
    }

    return W_FALSE;
}

// ToDo maybe add function that appends actuator to curent reading

WOLK_ERR_T wolk_publish_num_actuator_status (wolk_ctx_t *ctx,const char *reference,double value, actuator_status_t status, uint32_t utc_time)
{
    unsigned char buf[READINGS_MQTT_SIZE];
    int len;
    parser_t parser;
    reading_t readings;
    char readings_buffer[READINGS_BUFFER_SIZE];
    memset (readings_buffer, 0, READINGS_BUFFER_SIZE);
    memset (buf, 0, READINGS_MQTT_SIZE);
    initialize_parser(&parser, ctx->parser_type);

    MQTTString topicString = MQTTString_initializer;

    char pub_topic[TOPIC_SIZE];
    memset (pub_topic, 0, TOPIC_SIZE);

    strcpy(pub_topic,ACTUATORS_STATUS_PATH);
    strcat(pub_topic,ctx->serial);
    strcat(pub_topic,"/");
    strcat(pub_topic,reference);

    topicString.cstring = pub_topic;

    char value_str[STR_64];
    memset (value_str, 0, STR_64);
    sprintf(value_str, "%lf", value);

    manifest_item_t numeric_actuator;
    manifest_item_init(&numeric_actuator, reference, READING_TYPE_ACTUATOR, DATA_TYPE_NUMERIC);

    reading_init(&readings, &numeric_actuator);
    reading_set_rtc(&readings, utc_time);
    reading_set_data(&readings, value_str);
    reading_set_actuator_status(&readings, status);

    size_t serialized_readings = parser_serialize_readings(&parser, &readings, 1, readings_buffer, READINGS_BUFFER_SIZE);

    printf("%lu reading(s) serialized:\n%s\n\n", serialized_readings, readings_buffer);

    len = MQTTSerialize_publish(buf, READINGS_MQTT_SIZE, 0, 0, 0, 0, topicString, (unsigned char*)readings_buffer, strlen(readings_buffer));
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    switch(transport_sendPacketBuffernb(ctx->sock)){
        case TRANSPORT_DONE:
            break;
        case TRANSPORT_ERROR:
            return W_TRUE;
        case TRANSPORT_AGAIN:
            return W_TRUE;
    }


    return W_FALSE;
}

WOLK_ERR_T wolk_publish_bool_actuator_status (wolk_ctx_t *ctx,const char *reference,bool value, actuator_status_t status, uint32_t utc_time)
{
    unsigned char buf[READINGS_MQTT_SIZE];
    int len;
    parser_t parser;
    reading_t readings;
    char readings_buffer[READINGS_BUFFER_SIZE];
    memset (readings_buffer, 0, READINGS_BUFFER_SIZE);
    memset (buf, 0, READINGS_MQTT_SIZE);
    initialize_parser(&parser, ctx->parser_type);

    MQTTString topicString = MQTTString_initializer;

    char pub_topic[TOPIC_SIZE];
    memset (pub_topic, 0, TOPIC_SIZE);

    strcpy(pub_topic,ACTUATORS_STATUS_PATH);
    strcat(pub_topic,ctx->serial);
    strcat(pub_topic,"/");
    strcat(pub_topic,reference);

    topicString.cstring = pub_topic;

    manifest_item_t bool_actuator;
    manifest_item_init(&bool_actuator, reference, READING_TYPE_ACTUATOR, DATA_TYPE_BOOLEAN);

    reading_init(&readings, &bool_actuator);
    reading_set_rtc(&readings, utc_time);

    if (value == true)
    {
        reading_set_data(&readings, BOOL_TRUE);
    } else
    {
        reading_set_data(&readings, BOOL_FALSE);
    }
    reading_set_actuator_status(&readings, status);

    size_t serialized_readings = parser_serialize_readings(&parser, &readings, 1, readings_buffer, READINGS_BUFFER_SIZE);

    printf("%lu reading(s) serialized:\n%s\n\n", serialized_readings, readings_buffer);

    len = MQTTSerialize_publish(buf, READINGS_MQTT_SIZE, 0, 0, 0, 0, topicString, (unsigned char*)readings_buffer, strlen(readings_buffer));
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    switch(transport_sendPacketBuffernb(ctx->sock)){
        case TRANSPORT_DONE:
            break;
        case TRANSPORT_ERROR:
            return W_TRUE;
        case TRANSPORT_AGAIN:
            return W_TRUE;
    }


    return W_FALSE;
}

WOLK_ERR_T wolk_keep_alive (wolk_ctx_t *ctx)
{
    unsigned char buf[READINGS_MQTT_SIZE];
    int len;
    MQTTString topicString = MQTTString_initializer;
    char pub_topic[TOPIC_SIZE];
    memset (pub_topic, 0, TOPIC_SIZE);


    strcpy(pub_topic,RADINGS_PATH);
    strcat(pub_topic,ctx->serial);
   // strcat(pub_topic,"/");

    topicString.cstring = pub_topic;
    memset (buf, 0, READINGS_MQTT_SIZE);


    len = MQTTSerialize_pingreq(buf, READINGS_MQTT_SIZE);

    transport_sendPacketBuffernb_start(ctx->sock, buf, len);

    switch(transport_sendPacketBuffernb(ctx->sock)){
        case TRANSPORT_DONE:
            break;
        case TRANSPORT_ERROR:
            return W_TRUE;
        case TRANSPORT_AGAIN:
            return W_TRUE;
    }

    return W_FALSE;
}



WOLK_ERR_T wolk_disconnect(wolk_ctx_t *ctx)
{
    int rc = 0;
    int len = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    len = MQTTSerialize_disconnect(buf, buflen);
    rc = transport_sendPacketBuffer(ctx->sock, buf, len);

    if (rc == -1)
    {
        return W_TRUE;
    }


    return W_FALSE;
}


