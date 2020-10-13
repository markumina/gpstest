#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gps_data.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

void parse_gps_data(void);

// From lib
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  Function prototypes                                           :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#define pi 3.14159265358979323846
double deg2rad(double);
double rad2deg(double);
double distance(double lat1, double lon1, double lat2, double lon2, char unit);
// End From lib

#define MAX_CONTINUOUS_WALK_BREAK 3000U

void app_main()
{
    parse_gps_data();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    esp_restart();
}

void parse_gps_data(void)
{
    printf("\nLength of GPS data: %d\n\n", sizeof(gps_data));

    // Get line
    char* gps_data_copy = (char*)malloc(sizeof(gps_data));
    strcpy(gps_data_copy, gps_data);
    char* end_data_line = NULL;
    char* end_date = NULL;
    char* gps_data_line = strtok_r(gps_data_copy, "\n", &end_data_line);
    char gps_data_line_copy[1000] = {0};
    char* gps_date = NULL;

    // Lat/lon strings
    float current_latitude = 0.0;
    float current_longitude = 0.0;
    float previous_latitude = 0.0;
    float previous_longitude = 0.0;

    // Accumulators
    float accumuated_trip_distance = 0.0;
    uint32_t number_of_walks = 0;

    // For timestamp
    struct tm tm = {0};
    time_t current_epoch = 0;
    time_t last_epoch = 0;

    // Flags
    bool b_first_pass = true;

    // Process line
    while(gps_data_line != NULL)
    {
#ifdef DEBUG_PARSE
        printf("%s\n", gps_data_line);
#endif // DEBUG_PARSE
        memcpy(gps_data_line_copy, gps_data_line, strlen(gps_data_line));

        // + or ; here? ; doesn't hurt..
        gps_date = strtok_r(gps_data_line_copy, ";", &end_date);

        if(gps_date != NULL)
        {
#ifdef DEBUG_PARSE
            printf("\n%s\n", gps_date);
#endif // DEBUG_PARSE

            if(strptime(gps_date, "%Y-%m-%d %H:%M:%S ", &tm) != NULL)
            {
                current_epoch = mktime(&tm);

                // Initialize last_epoch with first for first pass..
                // -- b_firstpass gets set at end of first pass through loop!
                if(true == b_first_pass)
                {
                    last_epoch = current_epoch;
                }

#ifdef DEBUG_PARSE
                printf("Epoch time current: %jd\nEpoch time last: %jd\nEpoch time diff: %jd\n"
                     ,(intmax_t)current_epoch
                     ,(intmax_t)last_epoch
                     ,(intmax_t)current_epoch - (intmax_t)last_epoch);
#endif // DEBUG_PARSE

                if(current_epoch - last_epoch > MAX_CONTINUOUS_WALK_BREAK)
                {
                    // TBD - check distance between this walk and the last, to diff between rest and walk.
#ifdef DEBUG_PARSE
                    printf("\nFound walk number: %d??\n\n", ++number_of_walks);
#endif // DEBUG_PARSE
                }

                last_epoch = current_epoch;
            }
        }

        // Extract coordinates from line..
        current_latitude = atof(strtok_r(NULL, ";", &end_date));
        current_longitude = atof(strtok_r(NULL, ";", &end_date));

#ifdef DEBUG_PARSE
        printf("\n\nlatitude: %lfF\nlongititude: %lfF\n" ,current_latitude ,current_longitude);
#endif // DEBUG_PARSE

        if(true == b_first_pass)
        {
            previous_latitude = current_latitude;
            previous_longitude = current_longitude;
        }

        accumuated_trip_distance += distance(previous_latitude, previous_longitude, current_latitude, current_longitude, 'M');

        previous_latitude = current_latitude;
        previous_longitude = current_longitude;

        b_first_pass = false;

        gps_data_line = strtok_r(NULL, "\n", &end_data_line);

    } // while

    printf("\n\n******* Results *******\n\n");
    printf("\nTotal number of walks . . . . . . . . .  %d\n", number_of_walks);
    printf("\nTotal miles walked . . . . . . . . . . . %lf\n", accumuated_trip_distance);
    // TBD: Make these cached variables..
    printf("\nStart/End straight line distance . . . . %lf\n", distance(40.702452, -73.984135, 40.736956, -73.986960, 'M'));
    printf("\nTime spent traveling. . . . . . . . . .  TBD\n");
    printf("\nTime spent at rest . . . . . . . . . . . TBD\n");

    printf("\n\nNOTE:\nTotal number of walks based on %d second break.", MAX_CONTINUOUS_WALK_BREAK);
    printf("\n");
}

// TBD, separate into file..

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::                                                                         :*/
/*::  This routine calculates the distance between two points (given the     :*/
/*::  latitude/longitude of those points). It is being used to calculate     :*/
/*::  the distance between two locations using GeoDataSource(TM) products.   :*/
/*::                                                                         :*/
/*::  Definitions:                                                           :*/
/*::    South latitudes are negative, east longitudes are positive           :*/
/*::                                                                         :*/
/*::  Passed to function:                                                    :*/
/*::    lat1, lon1 = Latitude and Longitude of point 1 (in decimal degrees)  :*/
/*::    lat2, lon2 = Latitude and Longitude of point 2 (in decimal degrees)  :*/
/*::    unit = the unit you desire for results                               :*/
/*::           where: 'M' is statute miles (default)                         :*/
/*::                  'K' is kilometers                                      :*/
/*::                  'N' is nautical miles                                  :*/
/*::  Worldwide cities and other features databases with latitude longitude  :*/
/*::  are available at https://www.geodatasource.com                         :*/
/*::                                                                         :*/
/*::  For enquiries, please contact sales@geodatasource.com                  :*/
/*::                                                                         :*/
/*::  Official Web site: https://www.geodatasource.com                       :*/
/*::                                                                         :*/
/*::           GeoDataSource.com (C) All Rights Reserved 2018                :*/
/*::                                                                         :*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  if ((lat1 == lat2) && (lon1 == lon2)) {
    return 0;
  }
  else {
    theta = lon1 - lon2;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    switch(unit) {
      case 'M':
        break;
      case 'K':
        dist = dist * 1.609344;
        break;
      case 'N':
        dist = dist * 0.8684;
        break;
    }
    return (dist);
  }
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}
