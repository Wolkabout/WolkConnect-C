#include "WolkConn.h"

#include "MQTTPacket.h"
#include "parser.h"

#define TIMEOUT_STEP 500000

#define NON_EXISTING "N/A"

#define BOOL_FALSE "false"
#define BOOL_TRUE "true"

#define KEEPALIVE_INTERVAL 20
#define CONFIG_PATH "config/"
#define SENSOR_PATH "sensors/"

#define SET_COMMAND "SET"
#define STATUS_COMMAND "STATUS"

const char *message2 = "STATUS S:true:READY;";

WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx, send_func snd_func, recv_func rcv_func, const char *serial, const char *password)
{
    wolk_queue_init (&ctx->actuator_queue);
    wolk_queue_init (&ctx->config_queue);
    initialize_parser(&ctx->wolk_parser, PARSER_TYPE_MQTT);
    ctx->readings_index = 0;


    memset (ctx->pub_topic, 0, TOPIC_SIZE);
    memset (ctx->sub_topic, 0, TOPIC_SIZE);

    strcpy(ctx->pub_topic,SENSOR_PATH);
    strcat(ctx->pub_topic,serial);

    strcpy(ctx->sub_topic, CONFIG_PATH);
    strcat(ctx->sub_topic, serial);

    // TODO: Danger, check memory
    MQTTPacket_connectData data_temp = MQTTPacket_connectData_initializer;

    ctx->data = data_temp;
    //ctx->data = MQTTPacket_connectData_initializer;
    int rc = 0;
    ctx->sock = 0;
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

    ctx->mytransport.sck = &ctx->sock;
    ctx->mytransport.getfn = transport_getdatanb;
    ctx->mytransport.state = 0;
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
    /* subscribe */
    topicString.cstring = ctx->sub_topic;
    len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);

    /* This is equivalent to the one above, but using the non-blocking functions. You will probably not want this in real life,
    in such a case replace this call by a scheme similar to the one you'll see in the main loop */
    transport_sendPacketBuffernb_start(ctx->sock, buf, len);
    while((rc=transport_sendPacketBuffernb(ctx->sock)) != TRANSPORT_DONE);

    return W_FALSE;
}

// TODO Rename mytransport


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
        if((rc=MQTTPacket_readnb(buf, buflen, &(ctx->mytransport))) == PUBLISH){
            unsigned char dup;
            int qos;
            unsigned char retained;
            unsigned short msgid;
            int payloadlen_in;
            unsigned char* payload_in;
            MQTTString receivedTopic;

            rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                                         &payload_in, &payloadlen_in, buf, buflen);
            if (rc == -1)
            {
                return W_TRUE;
            }



            printf("message arrived %.*s\n", payloadlen_in, payload_in);
            printf("publishing reading\n");



            size_t num_deserialized_commands = parser_deserialize_commands(&ctx->wolk_parser, payload_in, payloadlen_in, &commands_buffer[0], 128);
            printf("Received commands buffer after command deserialization: %s\n\n", payload_in);

            printf("%lu command(s) deserialized:\n", num_deserialized_commands);
            for (i = 0; i < num_deserialized_commands; ++i) {
                actuator_command_t* command = &commands_buffer[i];

                printf("Command type: ");
                switch(actuator_command_get_type(command))
                {
                case ACTUATOR_COMMAND_TYPE_STATUS:
                    printf("STATUS\n");
                    printf("  Reference:  %s\n", actuator_command_get_reference(command));
                    wolk_queue_push(&ctx->actuator_queue, STATUS_COMMAND, actuator_command_get_reference(command), NON_EXISTING);
                    break;

                case ACTUATOR_COMMAND_TYPE_SET:
                    printf("SET\n");
                    printf("  Reference:  %s\n", actuator_command_get_reference(command));
                    printf("  Value:      %s\n", actuator_command_get_value(command));
                    wolk_queue_push(&ctx->actuator_queue, SET_COMMAND, actuator_command_get_reference(command), actuator_command_get_value(command));
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

WOLK_ERR_T wolk_add_string_reading(wolk_ctx_t *ctx,const char *reference,const char *value)
{
    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_init(&ctx->readings[ctx->readings_index], &string_sensor);
    reading_set_data(&ctx->readings[ctx->readings_index], value);
    reading_set_rtc(&ctx->readings[ctx->readings_index], 798123456);

    ctx->readings_index++;

    return W_FALSE;
}

WOLK_ERR_T wolk_add_numeric_reading(wolk_ctx_t *ctx,const char *reference,double value)
{
    manifest_item_t numeric_sensor;
    manifest_item_init(&numeric_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_NUMERIC);

    char value_str[STR_64];
    memset (value_str, 0, STR_64);
    sprintf(value_str, "%f", value);

    reading_init(&ctx->readings[ctx->readings_index], &numeric_sensor);
    reading_set_data(&ctx->readings[ctx->readings_index], value_str);
    reading_set_rtc(&ctx->readings[ctx->readings_index], 123456789);

    ctx->readings_index++;

    return W_FALSE;
}

WOLK_ERR_T wolk_add_bool_reading(wolk_ctx_t *ctx,const char *reference,bool value)
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
    reading_set_rtc(&ctx->readings[ctx->readings_index], 456789123);

    ctx->readings_index++;

    return W_FALSE;
}

WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx)
{
    reading_clear_array(ctx->readings, READINGS_SIZE);

    ctx->readings_index = 0;

    return W_FALSE;
}

WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx)
{
    unsigned char buf[200];
    int len;
    int buflen = sizeof(buf);
    char readings_buffer[READINGS_BUFFER_SIZE];
    MQTTString topicString = MQTTString_initializer;
    topicString.cstring = ctx->pub_topic;
    memset (readings_buffer, 0, READINGS_BUFFER_SIZE);


    size_t serialized_readings = parser_serialize_readings(&ctx->wolk_parser, &ctx->readings[0], ctx->readings_index+1, readings_buffer, READINGS_BUFFER_SIZE);

    wolk_clear_readings (ctx);

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

WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *value)
{
    unsigned char buf[200];
    int len;
    int buflen = sizeof(buf);
    parser_t parser;
    reading_t readings;
    char readings_buffer[READINGS_BUFFER_SIZE];
    initialize_parser(&parser, PARSER_TYPE_MQTT);

    MQTTString topicString = MQTTString_initializer;
    topicString.cstring = ctx->pub_topic;

    manifest_item_t string_sensor;
    manifest_item_init(&string_sensor, reference, READING_TYPE_SENSOR, DATA_TYPE_STRING);

    reading_init(&readings, &string_sensor);
    reading_set_data(&readings, value);
    reading_set_rtc(&readings, 798123456);


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

WOLK_ERR_T wolk_publish_num_actuator_status (wolk_ctx_t *ctx,const char *reference,double value, actuator_status_t status)
{
    unsigned char buf[200];
    int len;
    int buflen = sizeof(buf);
    parser_t parser;
    reading_t readings;
    char readings_buffer[READINGS_BUFFER_SIZE];
    initialize_parser(&parser, PARSER_TYPE_MQTT);

    MQTTString topicString = MQTTString_initializer;
    topicString.cstring = ctx->pub_topic;

    char value_str[STR_64];
    memset (value_str, 0, STR_64);
    sprintf(value_str, "%f", value);

    manifest_item_t numeric_actuator;
    manifest_item_init(&numeric_actuator, reference, READING_TYPE_ACTUATOR, DATA_TYPE_NUMERIC);

    reading_init(&readings, &numeric_actuator);
    reading_set_rtc(&readings, 789789789);
    reading_set_data(&readings, value_str);
    reading_set_actuator_status(&readings, status);

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

WOLK_ERR_T wolk_publish_bool_actuator_status (wolk_ctx_t *ctx,const char *reference,bool value, actuator_status_t status)
{
    unsigned char buf[200];
    int len;
    int buflen = sizeof(buf);
    parser_t parser;
    reading_t readings;
    char readings_buffer[READINGS_BUFFER_SIZE];
    initialize_parser(&parser, PARSER_TYPE_MQTT);

    MQTTString topicString = MQTTString_initializer;
    topicString.cstring = ctx->pub_topic;

    manifest_item_t bool_actuator;
    manifest_item_init(&bool_actuator, reference, READING_TYPE_ACTUATOR, DATA_TYPE_BOOLEAN);

    reading_init(&readings, &bool_actuator);
    reading_set_rtc(&readings, 987654321);

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


