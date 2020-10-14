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

#define MAX_CONTINUOUS_WALK_BREAK 3600U

void app_main()
{
    parse_gps_data();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    esp_restart();
}

void parse_gps_data(void)
{
    printf("\nLength of GPS data: %d\n\n", sizeof(gps_data));

    // Get line (unsafe, to be replaced by file)
    char* gps_data_copy = (char*)malloc(sizeof(gps_data));
    strcpy(gps_data_copy, gps_data);
    char* end_data_line = NULL;
    char* end_working_data_line = NULL;
    char* gps_data_line = strtok_r(gps_data_copy, "\n", &end_data_line);
    char gps_data_line_copy[1000] = {0};
    char* gps_value = NULL;
    char gps_trip_start_date[100] = {0};
    char new_gps_trip_start_date[100] = {0};

    // Lat/lon values for incremental calc
    float current_latitude = 0.0;
    float current_longitude = 0.0;
    float previous_latitude = 0.0;
    float previous_longitude = 0.0;

    // Lat/lon values for start and end of trip
    float trip_start_latitude = 0.0;
    float trip_start_longitude = 0.0;

    // Based on time for now (TBD - base on time AND location to cover non-contiguous travel).
    bool b_trip_end_detected = false;

    // Accumulators
    float combined_walks_distance = 0.0;
    float trip_walk_distance = 0.0;
    float marginal_distance = 0.0;
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
        gps_value = strtok_r(gps_data_line_copy, ";", &end_working_data_line);

        if(gps_value != NULL)
        {
#ifdef DEBUG_PARSE
            printf("\nDate: %s\n", gps_value);
#endif // DEBUG_PARSE

            // Extract date
            if(strptime(gps_value, "%Y-%m-%d %H:%M:%S ", &tm) != NULL)
            {
                current_epoch = mktime(&tm);

                // Initialize last_epoch with first for first pass..
                // -- b_firstpass gets set at end of first pass through loop!
                if(true == b_first_pass)
                {
                    strncpy(new_gps_trip_start_date, gps_value, sizeof(new_gps_trip_start_date)-1);
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
                    strncpy(gps_trip_start_date, new_gps_trip_start_date, sizeof(new_gps_trip_start_date)-1);   
                    strncpy(new_gps_trip_start_date, gps_value, sizeof(new_gps_trip_start_date)-1);

                    // TBD - check distance between this walk and the last, to diff between rest and walk.
                    ++number_of_walks;
#ifdef DEBUG_PARSE
                    printf("\nFound walk number: %d??\n\n", number_of_walks);
#endif // DEBUG_PARSE
                    b_trip_end_detected = true;
                }

                last_epoch = current_epoch;
            }
        }

        // Extract coordinates
        current_latitude = atof(strtok_r(NULL, ";", &end_working_data_line));
        current_longitude = atof(strtok_r(NULL, ";", &end_working_data_line));

#ifdef DEBUG_PARSE
        printf("\n\nlatitude: %lfF\nlongititude: %lfF\n" ,current_latitude ,current_longitude);
#endif // DEBUG_PARSE

        if(true == b_first_pass)
        {
            // 0 distance for first pass
            previous_latitude = current_latitude;
            previous_longitude = current_longitude;

            // First trip start = first data point
            trip_start_latitude = current_latitude;
            trip_start_longitude = current_longitude;
        }

        // Earlier we detected that a trip ended:
        // - Don't take the last long/lat into account for calculating distance
        // - Report distance that we have, zero it, then set up for next trip
        if(true == b_trip_end_detected)
        {
            b_trip_end_detected = false;

            printf("\n\n******* Trip %d Results *******\n", number_of_walks);
            printf("\nTrip started @ . . . . . . . . . . . .  %s\n", gps_trip_start_date);
            printf("\nTrip miles walked . . . . . . . . . . . %lf\n", trip_walk_distance);
            printf("\nTrip origin (long./lat.) . . . . . . .  %lf, %lf\n", trip_start_longitude, trip_start_latitude);
            printf("\nTrip destination (long./lat.) . . . . . %lf, %lf\n\n", previous_longitude, previous_latitude);

            // Next trip start = current coordinates
            trip_start_latitude = current_latitude;
            trip_start_longitude = current_longitude;

            trip_walk_distance = 0;
        }

        marginal_distance = distance(previous_latitude, previous_longitude, current_latitude, current_longitude, 'M');

        trip_walk_distance += marginal_distance;
        combined_walks_distance += marginal_distance;

        previous_latitude = current_latitude;
        previous_longitude = current_longitude;

        b_first_pass = false;

        gps_data_line = strtok_r(NULL, "\n", &end_data_line);

    } // while

    printf("\n\n******* Additional Results (pertaining to total dataset) *******\n");
    printf("\nTotal number of walks . . . . . . . . . . . . .  %d\n", number_of_walks);
    printf("\nTotal miles walked . . . . . . . . . . . . . . . %lf\n", combined_walks_distance);
    // TBD: Make these cached variables..
    printf("\nTotal origin/dest straight line distance . . . . %lf\n", distance(40.702452, -73.984135, 40.736956, -73.986960, 'M'));

    printf("\n\n***NOTE:\n\n -Total number of walks based on %d second travel break.\n\n\nEnd output\n", MAX_CONTINUOUS_WALK_BREAK);
    printf("\n");
}

// TBD, separate into file..
// From lib

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
// End From lib

