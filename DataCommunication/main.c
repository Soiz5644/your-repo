#include <mariadb/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sht40.h"
#include "sht35.h"
#include "sttsh22.h"

void finish_with_error(MYSQL *con) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
}

void insert_sensor_data(MYSQL *con, int sensor_id, double temperature, double humidity, int humidity_available) {
    char query[256];

    if (humidity_available) {
	snprintf(query, sizeof(query),
	         "INSERT INTO sensor_readings(sensor_id, temperature, humidity, reading_date) "
	         "VALUES(%d, %.2lf, %.2lf, NOW())",
	         sensor_id, temperature, humidity);
    } else {
	snprintf(query, sizeof(query),
	         "INSERT INTO sensor_readings(sensor_id, temperature, reading_date) "
		 "VALUES(%d, %.2lf, NOW())",
		 sensor_id, temperature);
    }

    if (mysql_query(con, query)) {
	finish_with_error(con);
    }
}

int main() {
    while(1) {
	MYSQL *con = mysql_init(NULL);

    	if (con == NULL) {
	    fprintf(stderr, "mysql_init() failed\n");
            exit(1);
    	}

        if (mysql_real_connect(con, "15.188.64.253", "vm", "Admin123%", "sensor_data", 0, NULL, 0) == NULL) {
	    finish_with_error(con);
   	 }

	double temperature, humidity;
    
        // Appel de chaque fonction
        sht40Func(&temperature, &humidity);
        insert_sensor_data(con, 40, temperature, humidity, 1);

        sht35Func(&temperature, &humidity);
	insert_sensor_data(con, 35, temperature, humidity, 1);

        sttsh22Func(&temperature);
        insert_sensor_data(con, 22, temperature, 0, 0);
        

	mysql_close(con);
	printf("Data inserted successsfully.\n");
        // Attendre une minute (60 secondes)
        sleep(30);
    }

    return 0;
}

