#ifndef WOLK_H
#define WOLK_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief WOLK_ERR_T Boolean used for error handling in Wolk conenction module
 */
typedef unsigned char WOLK_ERR_T;
/**
 * @brief WOLK_ERR_T Boolean used in Wolk connection module
 */
typedef unsigned char WOLK_BOOL_T;

#define W_TRUE 1
#define W_FALSE 0

#define PAYLOAD_SIZE 256

//add actuation API


typedef struct _wolk_ctx_t wolk_ctx_t;

//Add parameters for mikroe cloud
struct _wolk_ctx_t {
    char payload[PAYLOAD_SIZE];
};

//Add parametrs for mikroe
WOLK_ERR_T wolk_connect (wolk_ctx_t *ctx); //TODO: Maybe add some parameters
WOLK_ERR_T wolk_disconnect (wolk_ctx_t *ctx); //TODO: Maybe add some parameters

WOLK_ERR_T wolk_add_reading (wolk_ctx_t *ctx,const char *reference,const char *reading);
WOLK_ERR_T wolk_clear_readings (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_publish_single (wolk_ctx_t *ctx,const char *reference,const char *reading);
WOLK_ERR_T wolk_publish (wolk_ctx_t *ctx);
WOLK_ERR_T wolk_read (wolk_ctx_t *ctx, char *message);
WOLK_ERR_T wolk_read_timeout (wolk_ctx_t *ctx, char *message, int time_out_sec);




//Ecamples - just an example:
WOLK_ERR_T wolk_set_temperature (wolk_ctx_t ctx, int temp);
WOLK_ERR_T wolk_set_pressure (wolk_ctx_t ctx, int press);
WOLK_ERR_T wolk_set_humidity (wolk_ctx_t ctx, int humidity);
WOLK_ERR_T wolk_set_light (wolk_ctx_t ctx, int light);
WOLK_ERR_T wolk_set_calories (wolk_ctx_t ctx, int calories);
WOLK_ERR_T wolk_set_steps (wolk_ctx_t ctx, int steps);
WOLK_ERR_T wolk_set_heartrate (wolk_ctx_t ctx, int heartrate);
WOLK_ERR_T wolk_set_generic (wolk_ctx_t ctx, int generic);
WOLK_ERR_T wolk_set_accelerometer (wolk_ctx_t ctx, int acc_x, int acc_y, int acc_z);
WOLK_ERR_T wolk_set_magnetometer (wolk_ctx_t ctx, int mag_x, int mag_y, int mag_z);
WOLK_ERR_T wolk_set_gyroscope (wolk_ctx_t ctx, int gyr_x, int gyr_y, int gyr_z);






#ifdef __cplusplus
}
#endif

#endif

