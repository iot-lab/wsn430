#define SIZEDATA 3
#define SIZEDATABUF 4
#define PERIOD_MS 250

typedef struct {
	uint16_t seq;
	uint16_t length;
	uint16_t measures[SIZEDATABUF][SIZEDATA];
} sensor_data_t;
