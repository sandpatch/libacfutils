/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license in the file COPYING
 * or http://www.opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file COPYING.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2018 Saso Kiselkov. All rights reserved.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#if	IBM
#include <windows.h>
#include <strsafe.h>
#else	/* !IBM */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif	/* !IBM */

#include <acfutils/assert.h>
#include <acfutils/helpers.h>
#include <acfutils/log.h>
#include <acfutils/safe_alloc.h>

/*
 * The single-letter versions of the IDs need to go after the two-letter ones
 * to make sure we pick up the two-letter versions first.
 */
static const char *const icao_country_codes[] = {
	"AN",	"AY",
	"BG",	"BI",	"BK",
	"C",
	"DA",	"DB",	"DF",	"DG",	"DI",	"DN",	"DR",	"DT",	"DX",
	"EB",	"ED",	"EE",	"EF",	"EG",	"EH",	"EI",	"EK",	"EL",
	"EN",	"EP",	"ES",	"ET",	"EV",	"EY",
	"FA",	"FB",	"FC",	"FD",	"FE",	"FG",	"FH",	"FI",	"FJ",
	"FK",	"FL",	"FM",	"FN",	"FO",	"FP",	"FQ",	"FS",	"FT",
	"FV",	"FW",	"FX",	"FY",	"FZ",
	"GA",	"GB",	"GC",	"GE",	"GF",	"GG",	"GL",	"GM",	"GO",
	"GQ",	"GS",	"GU",	"GV",
	"HA",	"HB",	"HC",	"HD",	"HE",	"HH",	"HK",	"HL",	"HR",
	"HS",	"HT",	"HU",
	"K",
	"LA",	"LB",	"LC",	"LD",	"LE",	"LF",	"LG",	"LH",	"LI",
	"LJ",	"LK",	"LL",	"LM",	"LN",	"LO",	"LP",	"LQ",	"LR",
	"LS",	"LT",	"LU",	"LV",	"LW",	"LX",	"LY",	"LZ",
	"MB",	"MD",	"MG",	"MH",	"MK",	"MM",	"MN",	"MP",	"MR",
	"MS",	"MT",	"MU",	"MW",	"MY",	"MZ",
	"NC",	"NF",	"NG",	"NI",	"NL",	"NS",	"NT",	"NV",	"NW",
	"NZ",
	"OA",	"OB",	"OE",	"OI",	"OJ",	"OK",	"OL",	"OM",	"OO",
	"OP",	"OR",	"OS",	"OT",	"OY",
	"PA",	"PB",	"PC",	"PF",	"PG",	"PH",	"PJ",	"PK",	"PL",
	"PM",	"PO",	"PP",	"PT",	"PW",
	"RC",	"RJ",	"RK",	"RO",	"RP",
	"SA",	"SB",	"SC",	"SD",	"SE",	"SF",	"SG",	"SH",	"SI",
	"SJ",	"SK",	"SL",	"SM",	"SN",	"SO",	"SP",	"SS",	"SU",
	"SV",	"SW",	"SY",
	"TA",	"TB",	"TD",	"TF",	"TG",	"TI",	"TJ",	"TK",	"TL",
	"TN",	"TQ",	"TR",	"TT",	"TU",	"TV",	"TX",
	"UA",	"UB",	"UC",	"UD",	"UG",	"UK",	"UM",	"UT",
	"U",
	"VA",	"VC",	"VD",	"VE",	"VG",	"VH",	"VI",	"VL",	"VM",
	"VN",	"VO",	"VQ",	"VR",	"VT",	"VV",	"VY",
	"WA",	"WB",	"WI",	"WM",	"WP",	"WQ",	"WR",	"WS",
	"Y",
	"ZK",	"ZM",
	"Z",
	NULL
};

static const char *months[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static struct {
	int		cycle;
	const char	*start;
} airac_eff_dates[] = {
    /* year 2015 */
    {1501, "08-JAN"}, {1502, "05-FEB"}, {1503, "05-MAR"}, {1504, "02-APR"},
    {1505, "30-APR"}, {1506, "28-MAY"}, {1507, "25-JUN"}, {1508, "23-JUL"},
    {1509, "20-AUG"}, {1510, "17-SEP"}, {1511, "15-OCT"}, {1512, "12-NOV"},
    {1513, "10-DEC"},
    /* year 2016 */
    {1601, "07-JAN"}, {1602, "04-FEB"}, {1603, "03-MAR"}, {1604, "31-MAR"},
    {1605, "28-APR"}, {1606, "26-MAY"}, {1607, "23-JUN"}, {1608, "21-JUL"},
    {1609, "18-AUG"}, {1610, "15-SEP"}, {1611, "13-OCT"}, {1612, "10-NOV"},
    {1613, "08-DEC"},
    /* year 2017 */
    {1701, "05-JAN"}, {1702, "02-FEB"}, {1703, "02-MAR"}, {1704, "30-MAR"},
    {1705, "27-APR"}, {1706, "25-MAY"}, {1707, "22-JUN"}, {1708, "20-JUL"},
    {1709, "17-AUG"}, {1710, "14-SEP"}, {1711, "12-OCT"}, {1712, "09-NOV"},
    {1713, "07-DEC"},
    /* year 2018 */
    {1801, "04-JAN"}, {1802, "01-FEB"}, {1803, "01-MAR"}, {1804, "29-MAR"},
    {1805, "26-APR"}, {1806, "24-MAY"}, {1807, "21-JUN"}, {1808, "19-JUL"},
    {1809, "16-AUG"}, {1810, "13-SEP"}, {1811, "11-OCT"}, {1812, "08-NOV"},
    {1813, "06-DEC"},
    /* year 2019 */
    {1901, "03-JAN"}, {1902, "31-JAN"}, {1903, "28-FEB"}, {1904, "28-MAR"},
    {1905, "25-APR"}, {1906, "23-MAY"}, {1907, "20-JUN"}, {1908, "18-JUL"},
    {1909, "15-AUG"}, {1910, "12-SEP"}, {1911, "10-OCT"}, {1912, "07-NOV"},
    {1913, "05-DEC"},
    /* year 2020 */
    {2001, "02-JAN"}, {2002, "30-JAN"}, {2003, "27-FEB"}, {2004, "26-MAR"},
    {2005, "23-APR"}, {2006, "21-MAY"}, {2007, "18-JUN"}, {2008, "16-JUL"},
    {2009, "13-AUG"}, {2010, "10-SEP"}, {2011, "08-OCT"}, {2012, "05-NOV"},
    {2013, "03-DEC"}, {2014, "31-DEC"},
    /* year 2021 */
    {2101, "28-JAN"}, {2102, "25-FEB"}, {2103, "25-MAR"}, {2104, "22-APR"},
    {2105, "20-MAY"}, {2106, "17-JUN"}, {2107, "15-JUL"}, {2108, "12-AUG"},
    {2109, "09-SEP"}, {2110, "07-OCT"}, {2111, "04-NOV"}, {2112, "02-DEC"},
    {2113, "30-DEC"},
    /* year 2022 */
    {2201, "27-JAN"}, {2202, "24-FEB"}, {2203, "24-MAR"}, {2204, "21-APR"},
    {2205, "19-MAY"}, {2206, "16-JUN"}, {2207, "14-JUL"}, {2208, "11-AUG"},
    {2209, "08-SEP"}, {2210, "06-OCT"}, {2211, "03-NOV"}, {2212, "01-DEC"},
    {2213, "29-DEC"},
    /* year 2023 */
    {2301, "26-JAN"}, {2302, "23-FEB"}, {2303, "23-MAR"}, {2304, "20-APR"},
    {2305, "18-MAY"}, {2306, "15-JUN"}, {2307, "13-JUL"}, {2308, "10-AUG"},
    {2309, "07-SEP"}, {2310, "05-OCT"}, {2311, "02-NOV"}, {2312, "30-NOV"},
    {2313, "28-DEC"},
    /* year 2024 */
    {2401, "25-JAN"}, {2402, "22-FEB"}, {2403, "21-MAR"}, {2404, "18-APR"},
    {2405, "16-MAY"}, {2406, "13-JUN"}, {2407, "11-JUL"}, {2408, "08-AUG"},
    {2409, "05-SEP"}, {2410, "03-OCT"}, {2411, "31-OCT"}, {2412, "28-NOV"},
    {2413, "26-DEC"},
    /* year 2025 */
    {2501, "23-JAN"}, {2502, "20-FEB"}, {2503, "20-MAR"}, {2504, "17-APR"},
    {2505, "15-MAY"}, {2506, "12-JUN"}, {2507, "10-JUL"}, {2508, "07-AUG"},
    {2509, "04-SEP"}, {2510, "02-OCT"}, {2511, "30-OCT"}, {2512, "27-NOV"},
    {2513, "25-DEC"},
    /* year 2026 */
    {2601, "22-JAN"}, {2602, "19-FEB"}, {2603, "19-MAR"}, {2604, "16-APR"},
    {2605, "14-MAY"}, {2606, "11-JUN"}, {2607, "09-JUL"}, {2608, "06-AUG"},
    {2609, "03-SEP"}, {2610, "01-OCT"}, {2611, "29-OCT"}, {2612, "26-NOV"},
    {2613, "24-DEC"},
    /* year 2027 */
    {2701, "21-JAN"}, {2702, "18-FEB"}, {2703, "18-MAR"}, {2704, "15-APR"},
    {2705, "13-MAY"}, {2706, "10-JUN"}, {2707, "08-JUL"}, {2708, "05-AUG"},
    {2709, "02-SEP"}, {2710, "30-SEP"}, {2711, "28-OCT"}, {2712, "25-NOV"},
    {2713, "23-DEC"},
    /* year 2028 */
    {2801, "20-JAN"}, {2802, "17-FEB"}, {2803, "16-MAR"}, {2804, "13-APR"},
    {2805, "11-MAY"}, {2806, "08-JUN"}, {2807, "06-JUL"}, {2808, "03-AUG"},
    {2809, "31-AUG"}, {2810, "28-SEP"}, {2811, "26-OCT"}, {2812, "23-NOV"},
    {2813, "21-DEC"},
    /* year 2029 */
    {2901, "18-JAN"}, {2902, "15-FEB"}, {2903, "15-MAR"}, {2904, "12-APR"},
    {2905, "10-MAY"}, {2906, "07-JUN"}, {2907, "05-JUL"}, {2908, "02-AUG"},
    {2909, "30-AUG"}, {2910, "27-SEP"}, {2911, "25-OCT"}, {2912, "22-NOV"},
    {2912, "20-DEC"},
    /* end of list */
    { .cycle = -1 }
};

/*
 * How to turn to get from hdg1 to hdg2 with positive being right and negative
 * being left. Always turns the shortest way around (<= 180 degrees).
 */
double
rel_hdg_impl(double hdg1, double hdg2, const char *file, int line)
{
	ASSERT_MSG(is_valid_hdg(hdg1) && is_valid_hdg(hdg2),
	    "from: %s:%d %f -> %f", file, line, hdg1, hdg2);
	if (hdg1 > hdg2) {
		if (hdg1 > hdg2 + 180)
			return (360 - hdg1 + hdg2);
		else
			return (-(hdg1 - hdg2));
	} else {
		if (hdg2 > hdg1 + 180)
			return (-(360 - hdg2 + hdg1));
		else
			return (hdg2 - hdg1);
	}
}

API_EXPORT bool_t
is_valid_xpdr_code(int code)
{
	return (code >= 0 && code <= 7777 &&
	    (code % 10) <= 7 &&
	    ((code / 10) % 10) <= 7 &&
	    ((code / 100) % 10) <= 7 &&
	    ((code / 1000) % 10) <= 7);
}

bool_t
is_valid_vor_freq(double freq_mhz)
{
	unsigned freq_khz = freq_mhz * 1000;

	/* Check correct frequency band */
	if (freq_khz < 108000 || freq_khz > 117950)
		return (B_FALSE);
	/*
	 * Check the LOC band - freq must be multiple of 200 kHz or
	 * remainder must be 50 kHz.
	 */
	if (freq_khz >= 108000 && freq_khz <= 112000 &&
	    freq_khz % 200 != 0 && freq_khz % 200 != 50)
		return (B_FALSE);
	/* Above 112 MHz, frequency must be multiple of 50 kHz */
	if (freq_khz % 50 != 0)
		return (B_FALSE);

	return (B_TRUE);
}

bool_t
is_valid_loc_freq(double freq_mhz)
{
	unsigned freq_khz = freq_mhz * 1000;

	/* Check correct frequency band */
	if (freq_khz < 108100 || freq_khz > 111950)
		return (B_FALSE);
	/* Check 200 kHz spacing with 100 kHz or 150 kHz remainder. */
	if (freq_khz % 200 != 100 && freq_khz % 200 != 150)
		return (B_FALSE);

	return (B_TRUE);
}

bool_t
is_valid_tacan_freq(double freq_mhz)
{
	unsigned freq_khz = freq_mhz * 1000;

	/* this is quite a guess! */
	if (freq_khz < 133000 || freq_khz > 136000 ||
	    freq_khz % 100 != 0)
		return (B_FALSE);
	return (B_TRUE);
}

bool_t
is_valid_ndb_freq(double freq_khz)
{
	unsigned freq_hz = freq_khz * 1000;
	/* 177 kHz for an NDB is the lowest I've ever seen */
	return (freq_hz >= 177000 && freq_hz <= 1750000);
}

/*
 * Checks if a string is a valid ICAO airport code. ICAO airport codes always:
 * 1) are 4 characters long
 * 2) are all upper case
 * 3) contain only the letters A-Z
 * 4) may not start with I, J, Q or X
 */
bool_t
is_valid_icao_code(const char *icao)
{
	if (strlen(icao) != 4)
		return (B_FALSE);
	for (int i = 0; i < 4; i++)
		if (icao[i] < 'A' || icao[i] > 'Z')
			return (B_FALSE);
	if (icao[0] == 'I' || icao[0] == 'J' || icao[0] == 'Q' ||
	    icao[0] == 'X')
		return (B_FALSE);
	return (B_TRUE);
}

/*
 * Extracts the country code portion from an ICAO airport code. Because the
 * returned string pointer for any given country code is always the same, it
 * is possible to simply compare country code correspondence using a simple
 * pointer value comparison. If the ICAO country code is not known, returns
 * NULL instead.
 * If the passed string isn't a valid ICAO country code, returns NULL as well.
 */
const char *
extract_icao_country_code(const char *icao)
{
	if (!is_valid_icao_code(icao))
		return (NULL);

	for (int i = 0; icao_country_codes[i] != NULL; i++) {
		if (strncmp(icao, icao_country_codes[i],
		    strlen(icao_country_codes[i])) == 0)
			return (icao_country_codes[i]);
	}
	return (NULL);
}

/*
 * Checks a runway ID for correct formatting. Runway IDs are
 * 2- or 3-character strings conforming to the following format:
 *	*) Two-digit runway heading between 01 and 36. Headings less
 *	   than 10 are always prefixed by a '0'. "00" is NOT valid.
 *	*) An optional parallel runway discriminator, one of 'L', 'R' or 'C'.
 */
bool_t
is_valid_rwy_ID(const char *rwy_ID)
{
	int hdg;
	int len = strlen(rwy_ID);

	if (len < 2 || len > 3 || !isdigit(rwy_ID[0]) || !isdigit(rwy_ID[1]))
		return (B_FALSE);
	hdg = atoi(rwy_ID);
	if (hdg == 0 || hdg > 36)
		return (B_FALSE);
	if (len == 3) {
		if (rwy_ID[2] != 'R' && rwy_ID[2] != 'L' && rwy_ID[2] != 'C')
			return (B_FALSE);
	}

	return (B_TRUE);
}

/*
 * Copies a runway identifier from an unknown source to a 4-character runway
 * identifier buffer. This also performs the following transformations on the
 * runway identifier:
 * 1) if the runway identifier had a trailing 'T', it is stripped
 * 2) if the runway was a US single-digit runway number, a '0' is prepended
 * The function performs no validity checking other than making sure that the
 * output buffer is not overflown. The caller is responsible for validating
 * the result after copy_rwy_ID() returns using is_valid_rwy_ID().
 */
void
copy_rwy_ID(const char *src, char dst[4])
{
	int len;

	/* make sure dst is populated with *something* */
	memset(dst, 0, 4);
	lacf_strlcpy(dst, src, 4);

	/*
	 * Strip an optional trailing true hdg indicator. If the runway had
	 * a suffix (e.g. '08LT'), the previous copy step will have already
	 * stripped it.
	 */
	len = strlen(dst);
	if (len > 0 && dst[len - 1] == 'T') {
		dst[len - 1] = '\0';
		len--;
	}

	/* check if the identifier is a US runway number and translate it */
	if (dst[0] >= '1' && dst[0] <= '9' &&
	    (len == 1 || (len == 2 && !isdigit(dst[1])))) {
		memmove(dst + 1, dst, len + 1);
		dst[0] = '0';
	}
}

static int
month2nr(const char *month)
{
	for (int i = 0; i < 12; i++) {
		if (strcmp(month, months[i]) == 0)
			return (i);
	}
	VERIFY_MSG(0, "Invalid month specified: \"%s\"", month);
}

static const char *
nr2month(unsigned tm_mon)
{
	VERIFY3S(tm_mon, <, 12);
	return (months[tm_mon]);
}

const char *
airac_cycle2eff_date(int cycle)
{
	for (int i = 0; airac_eff_dates[i].cycle != -1; i++) {
		if (airac_eff_dates[i].cycle == cycle)
			return (airac_eff_dates[i].start);
	}
	return (NULL);
}

static time_t
cycle2start(int i)
{
	struct tm cyc_tm = { .tm_year = 0 };
	char day[4], month[4];
	int year = (airac_eff_dates[i].cycle / 100) + 100;
	const char *start = airac_eff_dates[i].start;

	snprintf(day, sizeof (day), "%c%c", start[0], start[1]);
	snprintf(month, sizeof (month), "%c%c%c", start[3], start[4], start[5]);

	cyc_tm.tm_year = year;
	cyc_tm.tm_mday = atoi(day);
	cyc_tm.tm_mon = month2nr(month);
	cyc_tm.tm_wday = -1;
	cyc_tm.tm_yday = -1;
	cyc_tm.tm_isdst = -1;

	return (mktime(&cyc_tm));
}

bool_t
airac_cycle2exp_date(int cycle, char buf[16], time_t *cycle_end_p)
{
	for (int i = 0; airac_eff_dates[i].cycle != -1; i++) {
		if (airac_eff_dates[i].cycle == cycle &&
		    airac_eff_dates[i + 1].cycle != -1) {
			/*
			 * Grab the effective date of the next cycle and
			 * subtract one day second. The end time will be
			 * the previous day at 23:59:59.
			 */
			time_t cycle_end_t = cycle2start(i + 1) - 1;
			const struct tm *end_tm = gmtime(&cycle_end_t);

			VERIFY(end_tm != NULL);
			snprintf(buf, 16, "%02d-%s", end_tm->tm_mday,
			    nr2month(end_tm->tm_mon));
			if (cycle_end_p != NULL)
				*cycle_end_p = cycle_end_t;

			return (B_TRUE);
		}
	}
	return (B_FALSE);
}

int
airac_time2cycle(time_t t)
{
	for (int i = 0; airac_eff_dates[i].cycle != -1; i++) {
		if (cycle2start(i) > t && i > 0)
			return (airac_eff_dates[i - 1].cycle);
	}

	return (-1);
}

/*
 * Grabs the next non-empty, non-comment line from a file, having stripped
 * away all leading and trailing whitespace. Any tab characters are also
 * replaced with spaces.
 *
 * @param fp File from which to retrieve the line.
 * @param linep Line buffer which will hold the new line. If the buffer pointer
 *	is set to NULL, it will be allocated. If it is not long enough, it
 *	will be expanded.
 * @param linecap The capacity of *linep. If set to zero a new buffer is
 *	allocated.
 * @param linenum The current line number. Will be advanced by 1 for each
 *	new line read.
 *
 * @return The number of characters in the line (after stripping whitespace)
 *	without the terminating NUL.
 */
ssize_t
parser_get_next_line(FILE *fp, char **linep, size_t *linecap, unsigned *linenum)
{
	for (;;) {
		ssize_t len = getline(linep, linecap, fp);
		if (len == -1)
			return (-1);
		(*linenum)++;
		strip_space(*linep);
		if (**linep == 0 || **linep == '#')
			continue;
		len = strlen(*linep);
		/* substitute spaces for tabs */
		for (ssize_t i = 0; i < len; i++) {
			if ((*linep)[i] == '\t')
				(*linep)[i] = ' ';
		}
		return (len);
	}
}

char *
parser_get_next_quoted_str(FILE *fp)
{
	return (parser_get_next_quoted_str2(fp, NULL));
}

char *
parser_get_next_quoted_str2(FILE *fp, int *linep)
{
	char c;
	char *str = safe_calloc(1, 1);
	size_t len = 0, cap = 0;

	for (;;) {
		do {
			c = fgetc(fp);
			if (c == '\n' && linep != NULL)
				(*linep)++;
		} while (isspace(c));
		if (c == EOF)
			break;
		if (c != '"') {
			ungetc(c, fp);
			break;
		}
		while ((c = fgetc(fp)) != EOF) {
			if (c == '"')
				break;
			if (c == '\\') {
				c = fgetc(fp);
				if (c == 'n') {
					c = '\n';
				} else if (c == 'r') {
					c = '\r';
				} else if (c == 't') {
					c = '\t';
				} else if (c == '\r') {
					/* skip the next LF char as well */
					c = fgetc(fp);
					if (c != '\n' && c != EOF)
						ungetc(c, fp);
					if (linep != NULL)
						(*linep)++;
					continue;
				} else if (c == '\n') {
					/* skip LF char */
					if (linep != NULL)
						(*linep)++;
					continue;
				} else if (c >= '0' && c <= '7') {
					/* 1-3 letter octal codes */
					char num[4];
					int val = 0;

					memset(num, 0, sizeof (num));
					num[0] = c;
					for (int i = 1; i < 3; i++) {
						c = fgetc(fp);
						if (c < '0' || c > '7') {
							ungetc(c, fp);
							break;
						}
						num[i] = c;
					}
					VERIFY(sscanf(num, "%o", &val) == 1);
					c = val;
				}
			}
			if (len == cap) {
				str = realloc(str, cap + 1 + 128);
				cap += 128;
			}
			str[len++] = c;
		}
		if (c == EOF)
			break;
	}

	str[len] = 0;

	return (str);
}

/*
 * Breaks up a line into components delimited by a character.
 *
 * @param line The input line to break up. The buffer will be modified
 *	to insert NULs in between the components so that they can each
 *	be treated as a separate string.
 * @param delim The delimiter character (e.g. ',').
 * @param comps A list of pointers that will be set to point to the
 *	start of each substring.
 * @param capacity The component capacity of `comps'. If more input
 *	components are encountered than is space in `comps', the array
 *	is not overflown.
 *
 * @return The number of components in the input string (at least 1).
 *	If the `comps' array was too small, returns the number of
 *	components that would have been needed to process the input
 *	string fully as a negative value.
 */
ssize_t
explode_line(char *line, char delim, char **comps, size_t capacity)
{
	ssize_t	i = 1;
	bool_t	toomany = B_FALSE;

	ASSERT(capacity != 0);
	comps[0] = line;
	for (char *p = line; *p != 0; p++) {
		if (*p == delim) {
			*p = 0;
			if (i < (ssize_t)capacity)
				comps[i] = p + 1;
			else
				toomany = B_TRUE;
			i++;
		}
	}

	return (toomany ? -i : i);
}

/*
 * Removes all leading & trailing whitespace from a line.
 */
void
strip_space(char *line)
{
	char	*p;
	size_t	len = strlen(line);

	/* strip leading whitespace */
	for (p = line; *p != 0 && isspace(*p); p++)
		;
	if (p != line) {
		memmove(line, p, (len + 1) - (p - line));
		len -= (p - line);
	}

	for (p = line + len - 1; p >= line && isspace(*p); p--)
		;
	p[1] = 0;
}

/*
 * Splits up an input string by separator into individual components.
 * The input string is `input' and the separator string is `sep'. The
 * skip_empty flag indicates whether to skip empty components (i.e.
 * two occurences of the separator string are next to each other).
 * Returns an array of pointers to the component strings and the number
 * of components in the `num' return parameter. The array of pointers
 * as well as the strings themselves are safe_malloc'd and should be freed
 * by the caller. Use free_strlist for that.
 */
char **
strsplit(const char *input, char *sep, bool_t skip_empty, size_t *num)
{
	char **result;
	size_t i = 0, n = 0;
	size_t seplen = strlen(sep);

	for (const char *a = input, *b = strstr(a, sep);
	    a != NULL; a = b + seplen, b = strstr(a, sep)) {
		if (b == NULL) {
			b = input + strlen(input);
			if (a != b || !skip_empty)
				n++;
			break;
		}
		if (a == b && skip_empty)
			continue;
		n++;
	}

	result = safe_calloc(n, sizeof (char *));

	for (const char *a = input, *b = strstr(a, sep);
	    a != NULL; a = b + seplen, b = strstr(a, sep)) {
		if (b == NULL) {
			b = input + strlen(input);
			if (a != b || !skip_empty) {
				result[i] = safe_calloc(b - a + 1, sizeof (char));
				memcpy(result[i], a, b - a);
			}
			break;
		}
		if (a == b && skip_empty)
			continue;
		ASSERT(i < n);
		result[i] = safe_calloc(b - a + 1, sizeof (char));
		memcpy(result[i], a, b - a);
		i++;
	}

	if (num != NULL)
		*num = n;

	return (result);
}

/*
 * Frees a string array returned by strsplit.
 */
void
free_strlist(char **comps, size_t len)
{
	if (comps == NULL)
		return;
	for (size_t i = 0; i < len; i++)
		free(comps[i]);
	free(comps);
}

/*
 * Appends a printf-like formatted string to the end of *str, reallocating
 * the buffer as needed to contain it. The value of *str is modified to
 * point to the appropriately reallocated buffer.
 */
void
append_format(char **str, size_t *sz, const char *format, ...)
{
	va_list ap;
	int needed;

	va_start(ap, format);
	needed = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	va_start(ap, format);
	*str = realloc(*str, *sz + needed + 1);
	(void) vsnprintf(*str + *sz, *sz + needed + 1, format, ap);
	*sz += needed;
	va_end(ap);
}

/*
 * Unescapes a string that uses '%XX' escape sequences, where 'XX' are two
 * hex digits denoting the ASCII code of the character to be inserted in
 * place of the escape sequence. The argument 'str' is shortened appropriately.
 */
void
unescape_percent(char *str)
{
	for (int i = 0, n = strlen(str); i + 2 < n; i++) {
		if (str[i] == '%') {
			char dig[3] = { str[i + 1], str[i + 2], 0 };
			unsigned val;
			sscanf(dig, "%x", &val);
			str[i] = val;
			memmove(&str[i + 1], &str[i + 3], (n - i) - 2);
			n -= 2;
		}
	}
}

#if	IBM
/*
 * C getline is a POSIX function, so on Windows, we need to roll our own.
 */
ssize_t
getline(char **line_p, size_t *cap_p, FILE *fp)
{
	ASSERT(line_p != NULL);
	ASSERT(cap_p != NULL);
	ASSERT(fp != NULL);

	char *line = *line_p;
	size_t cap = *cap_p, n = 0;

	do {
		if (n + 1 >= cap) {
			cap += 256;
			line = realloc(line, cap);
		}
		ASSERT(n < cap);
		if (fgets(&line[n], cap - n, fp) == NULL) {
			if (n != 0) {
				break;
			} else {
				*line_p = line;
				*cap_p = cap;
				return (-1);
			}
		}
		n = strlen(line);
	} while (n > 0 && line[n - 1] != '\n');

	*line_p = line;
	*cap_p = cap;

	return (n);
}
#endif	/* IBM */

API_EXPORT void
strtolower(char *str)
{
	for (int i = 0, n = strlen(str); i < n; i++)
		str[i] = tolower(str[i]);
}

API_EXPORT void
strtoupper(char *str)
{
	for (int i = 0, n = strlen(str); i < n; i++)
		str[i] = toupper(str[i]);
}

API_EXPORT size_t
utf8_charlen(const char *str)
{
	if ((str[0] & 0xe0) == 0xc0 && str[1] != 0)
		return (2);
	if ((str[0] & 0xf0) == 0xe0 && str[1] != 0 && str[2] != 0)
		return (3);
	if ((str[0] & 0xf8) == 0xf0 && str[1] != 0 && str[2] != 0 &&
	    str[3] != 0)
		return (4);
	return (1);
}

API_EXPORT size_t
utf8_strlen(const char *str)
{
	const char *s;
	for (s = str; s[0] != 0;)
		s += utf8_charlen(str);
	return (s - str);
}

/*
 * Creates a file path string from individual path components. The
 * components are provided as separate filename arguments and the list needs
 * to be terminated with a NULL argument. The returned string can be freed
 * via free().
 */
char *
mkpathname(const char *comp, ...)
{
	char *res;
	va_list ap;

	va_start(ap, comp);
	res = mkpathname_v(comp, ap);
	va_end(ap);

	return (res);
}

char *
mkpathname_v(const char *comp, va_list ap)
{
	size_t n = 0, len = 0;
	char *str;
	va_list ap2;

	ASSERT(ap != NULL);

	va_copy(ap2, ap);
	len = strlen(comp);
	for (const char *c = va_arg(ap2, const char *); c != NULL;
	    c = va_arg(ap2, const char *)) {
		len += 1 + strlen(c);
	}
	va_end(ap2);

	str = safe_malloc(len + 1);
	n += snprintf(str, len + 1, "%s", comp);
	for (const char *c = va_arg(ap, const char *); c != NULL;
	    c = va_arg(ap, const char *)) {
		ASSERT(n < len);
		n += snprintf(&str[n], len - n + 1, "%c%s", DIRSEP, c);
		/* kill a trailing directory separator */
		if (str[n - 1] == DIRSEP) {
			str[n - 1] = 0;
			n--;
		}
	}

	return (str);
}

/*
 * For some inexplicable reason, on Windows X-Plane can return paths via the
 * API which are a mixture of Windows- & Unix-style path separators. This
 * function fixes that by flipping all path separators to the appropriate
 * type used on the host OS.
 */
void
fix_pathsep(char *str)
{
	for (int i = 0, n = strlen(str); i < n; i++) {
#if	IBM
		if (str[i] == '/')
			str[i] = '\\';
#else	/* !IBM */
		if (str[i] == '\\')
			str[i] = '/';
#endif	/* !IBM */
	}
}

char *
file2str(const char *comp, ...)
{
	va_list ap;
	char *filename, *str;
	long len;

	va_start(ap, comp);
	filename = mkpathname_v(comp, ap);
	va_end(ap);
	str = file2str_name(&len, filename);
	free(filename);

	return (str);
}

char *
file2str_ext(long *len_p, const char *comp, ...)
{
	va_list ap;
	char *filename, *str;

	va_start(ap, comp);
	filename = mkpathname_v(comp, ap);
	va_end(ap);
	str = file2str_name(len_p, filename);
	free(filename);

	return (str);
}

char *
file2str_name(long *len_p, const char *filename)
{
#define	MAX_FILESIZE	(256 << 20)	/* 256MB */
	char *contents;
	FILE *fp;
	long len;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return (NULL);

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	if (len < 0 || len > MAX_FILESIZE) {
		fclose(fp);
		return (NULL);
	}
	fseek(fp, 0, SEEK_SET);
	contents = safe_malloc(len + 1);
	contents[len] = 0;
	if (fread(contents, 1, len, fp) != (size_t)len) {
		fclose(fp);
		free(contents);
		return (NULL);
	}
	fclose(fp);
	*len_p = len;

	return (contents);
}

API_EXPORT void *
file2buf(const char *filename, size_t *bufsz)
{
	ssize_t s;
	void *buf;
	FILE *fp = fopen(filename, "rb");

	if (fp == NULL) {
		*bufsz = 0;
		return (NULL);
	}
	fseek(fp, 0, SEEK_END);
	s = ftell(fp);
	if (s <= 0) {
		*bufsz = 0;
		fclose(fp);
		return (NULL);
	}
	fseek(fp, 0, SEEK_SET);

	buf = safe_malloc(s);
	if (fread(buf, 1, s, fp) < (size_t)s) {
		*bufsz = 0;
		free(buf);
		fclose(fp);
		return (NULL);
	}

	fclose(fp);
	*bufsz = s;

	return (buf);
}

ssize_t
filesz(const char *filename)
{
	ssize_t s;
	FILE *fp = fopen(filename, "rb");

	if (fp == NULL)
		return (-1);
	fseek(fp, 0, SEEK_END);
	s = ftell(fp);
	fclose(fp);

	return (s);
}

#if	IBM

void
win_perror(DWORD err, const char *fmt, ...)
{
	va_list ap;
	LPSTR win_msg = NULL;
	char *caller_msg;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	caller_msg = safe_malloc(len + 1);
	va_start(ap, fmt);
	vsnprintf(caller_msg, len + 1, fmt, ap);
	va_end(ap);

	(void) FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    (LPSTR)&win_msg, 0, NULL);
	logMsg("%s: %s", caller_msg, win_msg);

	free(caller_msg);
	LocalFree(win_msg);
}

#endif	/* IBM */

/*
 * Returns true if the file exists. If `isdir' is non-NULL, if the file
 * exists, isdir is set to indicate if the file is a directory.
 */
bool_t
file_exists(const char *filename, bool_t *isdir)
{
#if	IBM
	int len = strlen(filename);
	TCHAR filenameT[len + 1];
	DWORD attr;

	MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenameT, len + 1);
	attr = GetFileAttributes(filenameT);
	if (isdir != NULL)
		*isdir = !!(attr & FILE_ATTRIBUTE_DIRECTORY);
	if (attr == INVALID_FILE_ATTRIBUTES) {
		errno = ENOENT;	/* fake an errno value */
		return (B_FALSE);
	}
	return (B_TRUE);
#else	/* !IBM */
	struct stat st;

	if (isdir != NULL)
		*isdir = B_FALSE;
	if (stat(filename, &st) < 0) {
		if (errno != ENOENT)
			logMsg("Error checking if file %s exists: %s",
			    filename, strerror(errno));
		return (B_FALSE);
	}
	if (isdir != NULL)
		*isdir = S_ISDIR(st.st_mode);
	return (B_TRUE);
#endif	/* !IBM */
}

/*
 * Creates an empty directory at `dirname' with default permissions.
 */
bool_t
create_directory(const char *dirname)
{
	ASSERT(dirname != NULL);
#if	IBM
	DWORD err;
	int len = strlen(dirname);
	WCHAR dirnameW[len + 1];

	MultiByteToWideChar(CP_UTF8, 0, dirname, -1, dirnameW, len + 1);
	if (!CreateDirectory(dirnameW, NULL) &&
	    (err = GetLastError()) != ERROR_ALREADY_EXISTS) {
		win_perror(err, "Error creating directory %s", dirname);
		return (B_FALSE);
	}
#else	/* !IBM */
	if (mkdir(dirname, 0777) != 0 && errno != EEXIST) {
		logMsg("Error creating directory %s: %s", dirname,
		    strerror(errno));
		return (B_FALSE);
	}
#endif	/* !IBM */
	return (B_TRUE);
}

/*
 * Same as create_directory, but creates all intermediate directories
 * on the way as well.
 */
bool_t
create_directory_recursive(const char *dirname)
{
	char *partname = safe_calloc(1, strlen(dirname) + 1);

	for (const char *start = dirname, *end = strchr(&dirname[1], DIRSEP);
	    end != NULL; start = end, end = strchr(&start[1], DIRSEP)) {
		strncat(partname, start, end - start);
		if (!create_directory(partname))
			return (B_FALSE);
	}

	free(partname);

	return (create_directory(dirname));
}

#if	IBM

static bool_t
win_rmdir(const LPTSTR dirnameT)
{
	WIN32_FIND_DATA find_data;
	HANDLE h_find;
	int dirname_len = wcslen(dirnameT);
	TCHAR srchnameT[dirname_len + 4];

	StringCchPrintf(srchnameT, dirname_len + 4, TEXT("%s\\*"), dirnameT);
	h_find = FindFirstFile(srchnameT, &find_data);
	if (h_find == INVALID_HANDLE_VALUE) {
		int err = GetLastError();
		char dirname[MAX_PATH];
		WideCharToMultiByte(CP_UTF8, 0, dirnameT, -1, dirname,
		    sizeof (dirname), NULL, NULL);
		win_perror(err, "Error listing directory %s", dirname);
		return (B_FALSE);
	}
	do {
		TCHAR filepathT[MAX_PATH];
		DWORD attrs;

		if (wcscmp(find_data.cFileName, TEXT(".")) == 0 ||
		    wcscmp(find_data.cFileName, TEXT("..")) == 0)
			continue;

		StringCchPrintf(filepathT, MAX_PATH, TEXT("%s\\%s"), dirnameT,
		    find_data.cFileName);
		attrs = GetFileAttributes(filepathT);

		if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			if (!win_rmdir(filepathT))
				goto errout;
		} else {
			if (!DeleteFile(filepathT)) {
				char filepath[MAX_PATH];
				WideCharToMultiByte(CP_UTF8, 0, filepathT, -1,
				    filepath, sizeof (filepath), NULL, NULL);
				win_perror(GetLastError(), "Error removing "
				    "file %s", filepath);
				goto errout;
			}
		}
	} while (FindNextFile(h_find, &find_data));
	FindClose(h_find);

	if (!RemoveDirectory(dirnameT)) {
		char dirname[MAX_PATH];
		WideCharToMultiByte(CP_UTF8, 0, dirnameT, -1,
		    dirname, sizeof (dirname), NULL, NULL);
		win_perror(GetLastError(), "Error removing directory %s",
		    dirname);
		return (B_FALSE);
	}

	return (B_TRUE);
errout:
	FindClose(h_find);
	return (B_FALSE);
}

#endif	/* IBM */

/*
 * Recursive directory removal, including all its contents.
 */
bool_t
remove_directory(const char *dirname)
{
#if	IBM
	TCHAR dirnameT[strlen(dirname) + 1];

	MultiByteToWideChar(CP_UTF8, 0, dirname, -1, dirnameT,
	    sizeof (dirnameT));
	return (win_rmdir(dirnameT));
#else	/* !IBM */
	DIR *dp;
	struct dirent *de;

	if ((dp = opendir(dirname)) == NULL) {
		logMsg("Error removing directory %s: %s", dirname,
		    strerror(errno));
		return (B_FALSE);
	}
	while ((de = readdir(dp)) != NULL) {
		char filename[FILENAME_MAX];
		int err;
		struct stat st;

		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;

		if (snprintf(filename, sizeof (filename), "%s/%s", dirname,
		    de->d_name) >= (ssize_t)sizeof (filename)) {
			logMsg("Error removing directory %s: path too long",
			    dirname);
			goto errout;
		}
		if (lstat(filename, &st) < 0) {
			logMsg("Error removing directory %s: cannot stat "
			    "file %s: %s", dirname, de->d_name,
			    strerror(errno));
			goto errout;
		}
		if (S_ISDIR(st.st_mode)) {
			if (!remove_directory(filename))
				goto errout;
			err = 0;
		} else {
			err = unlink(filename);
		}
		if (err != 0) {
			logMsg("Error removing %s: %s", filename,
			    strerror(errno));
			goto errout;
		}
	}
	closedir(dp);
	if (rmdir(dirname) != 0) {
		logMsg("Error removing %s: %s", dirname, strerror(errno));
		goto errout;
	}
	return (B_TRUE);
errout:
	closedir(dp);
	return (B_FALSE);
#endif	/* !IBM */
}

bool_t
remove_file(const char *filename, bool_t notfound_ok)
{
#if	IBM
	TCHAR filenameT[strlen(filename) + 1];
	DWORD error;

	MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenameT,
	    sizeof (filenameT));
	if (!DeleteFile(filenameT) && ((error = GetLastError()) !=
	    ERROR_FILE_NOT_FOUND || !notfound_ok)) {
		win_perror(error, "Cannot remove file %s", filename);
		return (B_FALSE);
	}
	return (B_TRUE);
#else	/* !IBM */
	if (unlink(filename) < 0 && (errno != ENOENT || !notfound_ok)) {
		logMsg("Cannot remove file %s: %s", filename, strerror(errno));
		return (B_FALSE);
	}
	return (B_TRUE);
#endif	/* !IBM */
}

API_EXPORT char *
lacf_dirname(const char *filename)
{
	int l = strlen(filename);
	char *p;
	char *str;

	if (l == 0)
		return (strdup(""));

	p = strrchr(filename, DIRSEP);
	if (p == NULL)
		return (strdup(""));
	str = malloc((p - filename) + 1);
	lacf_strlcpy(str, filename, (p - filename) + 1);

	return (str);
}

#if	IBM

DIR *
opendir(const char *path)
{
	DIR	*dirp = safe_calloc(1, sizeof (*dirp));
	TCHAR	pathT[strlen(path) + 3];	/* For '\*' at the end */

	MultiByteToWideChar(CP_UTF8, 0, path, -1, pathT, sizeof (pathT));
	StringCchCat(pathT, sizeof (pathT), TEXT("\\*"));
	dirp->handle = FindFirstFile(pathT, &dirp->find_data);
	if (dirp->handle == INVALID_HANDLE_VALUE) {
		win_perror(GetLastError(), "Cannot open directory %s", path);
		free(dirp);
		return (NULL);
	}
	dirp->first = B_TRUE;
	return (dirp);
}

struct dirent *
readdir(DIR *dirp)
{
	/* First call to readdir retrieves what we got from FindFirstFile */
	if (!dirp->first) {
		if (FindNextFile(dirp->handle, &dirp->find_data) == 0)
			return (NULL);
	} else {
		dirp->first = B_FALSE;
	}
	WideCharToMultiByte(CP_UTF8, 0, dirp->find_data.cFileName, -1,
	    dirp->de.d_name, sizeof (dirp->de.d_name), NULL, NULL);
	return (&dirp->de);
}

void
closedir(DIR *dirp)
{
	FindClose(dirp->handle);
	free(dirp);
}

int
stat(const char *pathname, struct stat *buf)
{
	FILETIME	wtime, atime;
	HANDLE		fh;
	TCHAR		pathnameT[strlen(pathname) + 1];
	LARGE_INTEGER	sz;
	ULARGE_INTEGER	ftime;
	bool_t		isdir;

	if (!file_exists(pathname, &isdir)) {
		logMsg("Cannot stat %s: no such file or directory", pathname);
		errno = ENOENT;
		return (-1);
	}

	MultiByteToWideChar(CP_UTF8, 0, pathname, -1, pathnameT,
	    sizeof (pathnameT));
	fh = CreateFile(pathnameT, (!isdir ? FILE_READ_ATTRIBUTES : 0) |
	    GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (fh == INVALID_HANDLE_VALUE ||
	    !GetFileTime(fh, NULL, &atime, &wtime) ||
	    (!isdir && !GetFileSizeEx(fh, &sz))) {
		win_perror(GetLastError(), "Cannot stat %s", pathname);
		errno = EACCES;
		return (-1);
	}
	if (isdir)
		buf->st_size = 0;
	else
		buf->st_size = sz.QuadPart;

	ftime.LowPart = atime.dwLowDateTime;
	ftime.HighPart = atime.dwHighDateTime;
	buf->st_atime = ftime.QuadPart / 10000000ull - 11644473600ull;

	ftime.LowPart = wtime.dwLowDateTime;
	ftime.HighPart = wtime.dwHighDateTime;
	buf->st_mtime = ftime.QuadPart / 10000000ull - 11644473600ull;

	CloseHandle(fh);

	return (0);
}

#endif	/* IBM */
