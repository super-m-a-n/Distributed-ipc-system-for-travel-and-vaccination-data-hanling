/* file : date.h */
#pragma once

// checking for validity of a date
int date_check(char * date);
// comparing two valid dates (returns 1 if date1 > date2, 0 if equal, -1 if date1 < date2)
int date_cmp(char * date1, char * date2);
// checks if date1 is within a 6-month interval prior to date2
// assumes date1 <= date2
// if date1 > date2 returns -1 as an error
int date_half_year_check(char * date1, char * date2);