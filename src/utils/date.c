/* file : date.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// utility functions for dates

// checking for validity of a date
int date_check(char * date)
{
	char * day, * month, * year;
	char temp_date[12];
	strcpy(temp_date, date);

	char *str = strtok(temp_date, "-");
	int i = 1;
	while(str != NULL)
	{
		switch (i)
	    {
	    	case 1: day = str; break;
	        case 2: month = str; break;
	        case 3: year = str; break;
	    }
	    i++;
	    str = strtok(NULL, "-");
	}

	if (i != 4)
		return 0;
	if (strlen(day) > 2 || strlen(month) > 2 || strlen(year) != 4)
		return 0;
	for (int i = 0; i < strlen(day); i++)
	{
		if (day[i] < '0' || day[i] > '9')
			return 0;
	}
	for (int i = 0; i < strlen(month); i++)
	{
		if (month[i] < '0' || month[i] > '9')
			return 0;
	}

	for (int i = 0; i < strlen(year); i++)
	{
		if (year[i] < '0' || year[i] > '9')
			return 0;
	}

	if (atoi(day) < 1 || atoi(day) > 30 || atoi(month) < 1 || atoi(month) > 12 )
		return 0;

	return 1;
}

// comparing two valid dates
int date_cmp(char * date1, char * date2)
{
	int day1, month1, year1;
	int day2, month2, year2;

	char temp_date1[12];
	strcpy(temp_date1, date1);
	char temp_date2[12];
	strcpy(temp_date2, date2);
	

	char *str1 = strtok(temp_date1, "-");
	int i = 1;
	while(str1 != NULL)
	{
		switch (i)
	    {
	    	case 1: day1 = atoi(str1); break;
	        case 2: month1 = atoi(str1); break;
	        case 3: year1 = atoi(str1); break;
	    }
	    i++;
	    str1 = strtok(NULL, "-");
	}

	char *str2 = strtok(temp_date2, "-");
	i = 1;
	while(str2 != NULL)
	{
		switch (i)
	    {
	    	case 1: day2 = atoi(str2); break;
	        case 2: month2 = atoi(str2); break;
	        case 3: year2 = atoi(str2); break;
	    }
	    i++;
	    str2 = strtok(NULL, "-");
	}

	if (year2 > year1) return -1;
	else if (year2 < year1) return 1;
	else if (month2 > month1) return -1;
	else if (month2 < month1) return 1;
	else if (day2 > day1) return -1;
	else if (day2 < day1) return 1;
	else return 0;
}

int date_half_year_check(char * date1, char * date2)
{
	if (date_cmp(date1, date2) > 0) 		// make sure date1 <= date2 otherwise the half year check obviously fails
		return -1;

	int day1, month1, year1;

	char temp_date1[12];
	strcpy(temp_date1, date1);
	

	char *str1 = strtok(temp_date1, "-");
	int i = 1;
	while(str1 != NULL)
	{
		switch (i)
	    {
	    	case 1: day1 = atoi(str1); break;
	        case 2: month1 = atoi(str1); break;
	        case 3: year1 = atoi(str1); break;
	    }
	    i++;
	    str1 = strtok(NULL, "-");
	}

	/* date1 is within to 6 months prior of date2, if and only if (date1 + 6 months) >= date2 */
	if (month1 <= 6)
		month1 += 6;
	else
	{
		month1 = (month1 + 6) % 12;
		year1 += 1;
	}

	char new_date1[12];
	sprintf(new_date1, "%d-%d-%d", day1, month1, year1);

	if (date_cmp(new_date1, date2) >= 0)
		return 1;
	else
		return 0;	
}