

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <lib/stringtokenizer.h>
#include <util/linefilereader.h>


#define CREDITS_LINE "CREDITS "

// glftpd lines to add stats to.
char STAT_LINES[][20] = {
	{ "ALLUP" },
	{ "WKUP" },
	{ "DAYUP" },
	{ "MONTHUP" },
	{ 0 }
};

struct gl_section_stat {
	int section;

	long files;
	long long kbytes;
	long seconds;

	struct gl_section_stat *next;
};

typedef struct gl_section_stat gl_section_stat_t;

struct gl_stat {
	char *cmd;
	gl_section_stat_t *stats;
};

typedef struct gl_stat gl_stat_t;

gl_section_stat_t * _gl_parse_sec_stats(stringtokenizer *st, int *sec) {
	gl_section_stat_t *ss = malloc(sizeof(gl_section_stat_t));
	char *tmp;

	if (st_count(st) < 3) {
		free(ss);
		return 0;
	}

	ss->section = (*sec)++;
	ss->next = 0;

	// do files.
	tmp = st_next(st);
	sscanf(tmp, "%ld", &ss->files);

	tmp = st_next(st);
	sscanf(tmp, "%Ld", &ss->kbytes);

	tmp = st_next(st);
	sscanf(tmp, "%ld", &ss->seconds);

	return ss;
}


gl_stat_t * _gl_parse_stats(char *str) {
	gl_stat_t *s = malloc(sizeof(gl_stat_t));
	gl_section_stat_t *ss;
	stringtokenizer st;
	int i = 0;

	st_initialize(&st, str, " ");

	if (st_count(&st) < 4)
		return 0;

	s->cmd = strdup(st_next(&st));
	s->stats = 0;

	while ((ss = _gl_parse_sec_stats(&st, &i)) != 0) {
		ss->next = s->stats;
		s->stats = ss;
	}

	st_finalize(&st);

	return s;
}

gl_section_stat_t * _gl_get_sec_stats(gl_stat_t *s, int sec) {
	gl_section_stat_t *t;

	for (t = s->stats; t; t = t->next)
		if (t->section == sec)
			break;

	return t;
}

char * _gl_tostring_stats(gl_stat_t *s) {
	char *tmp = malloc(100), stbuf[100];
	gl_section_stat_t *t;
	int i = 0;

	sprintf(tmp, "%s", s->cmd);

	while (t = _gl_get_sec_stats(s, i++)) {
		sprintf(stbuf, " %ld %Ld %ld", t->files, t->kbytes, t->seconds);
		strcat(tmp, stbuf);
	}

	return tmp;
}

char * _gl_userfile_add_stats(char *str, int files, long kbytes, int seconds, int sec) {
	gl_section_stat_t *ss;
	gl_stat_t *stat;
	char *tmp;

	stat = _gl_parse_stats(str);

	if (!stat)
		return strdup(str);

	ss = _gl_get_sec_stats(stat, sec);

	if (!ss)
		return strdup(str);

	ss->files += files;
	ss->kbytes += kbytes;
	ss->seconds += seconds;

	tmp = _gl_tostring_stats(stat);

	return tmp;
}

char * _gl_userfile_add_credits(char *str, long credits, int sec) {
	char *cmd, *tmp;
	stringtokenizer st;
	long long tmpcred;
	int i = 0;
	char buf[300], credstr[300];

	st_initialize(&st, str, " ");

	cmd = st_next(&st);
	strcpy(buf, cmd);

	while (tmp = st_next(&st)) {
		if (i == sec) {
			sscanf(tmp, "%Ld", &tmpcred);
			tmpcred += credits;
			sprintf(credstr, " %Ld", tmpcred);
			strcat(buf, credstr);
		} else {
			strcat(buf, " ");
			strcat(buf, tmp);
		}

		i++;
	}

	st_finalize(&st);

	return strdup(buf);
}

int _gl_userfile_is_stat(char *s) {
	int i = 0;

	while (STAT_LINES[i][0] != 0) {
		if (!strncasecmp(s, STAT_LINES[i], strlen(STAT_LINES[i])))
			return 1;

		i++;
	}

	return 0;
}

int _gl_userfile_is_credit(char *s) {
	if (!strncasecmp(s, CREDITS_LINE, strlen(CREDITS_LINE)))
		return 1;

	return 0;
}

int gl_userfile_add_stats(char *userfile, int files, long kbytes, int seconds, long credits, int stat_sec, int cred_sec) {
	linefilereader_t lfr;
	char *ufnew, *tmp, buf[1024];
	int rc = 0;
	FILE *out;

	if (lfr_open(&lfr, userfile) < 0)
		return -1;

	ufnew = malloc(strlen(userfile) + 10);
	sprintf(ufnew, "%s.pre-tmp", userfile);
	out = fopen(ufnew, "w");

	if (!out) {
		lfr_close(&lfr);
		fclose(out);
		free(ufnew);
		return -1;
	}

	while (lfr_getline(&lfr, buf, 1024) > -1) {
		if ((stat_sec > -1) && _gl_userfile_is_stat(buf)) {
			tmp = _gl_userfile_add_stats(buf, files, kbytes, seconds, stat_sec);
			fprintf(out, "%s\n", tmp);
			free(tmp);
			rc++;
		} else if ((cred_sec > -1) && _gl_userfile_is_credit(buf)) {
			tmp = _gl_userfile_add_credits(buf, credits, cred_sec);
			fprintf(out, "%s\n", tmp);
			free(tmp);
			rc++;
		} else
			fprintf(out, "%s\n", buf);
	}

	fclose(out);
	lfr_close(&lfr);

	rename(ufnew, userfile);

	free(ufnew);

	return rc;
}


int gl_userfile_get_ratio(char *userfile, int section) {
	linefilereader_t lfr;
	stringtokenizer st;
	char buf[1024], *tmp;
	int found = 0;

	if (lfr_open(&lfr, userfile) < 0)
		return -1;

	while (lfr_getline(&lfr, buf, 1024) > -1) {
		if (!strncasecmp(buf, "RATIO ", 6)) {
			found++;
			break;
		}
	}

	lfr_close(&lfr);

	if (!found)
		return -1;

	st_initialize(&st, buf, " ");

	// skip the RATIO text.
	st_next(&st);

	if (st_count(&st) < section) {
		st_finalize(&st);
		return -1;
	}

	for (found = 0; found < section; found++)
		st_next(&st);

	tmp = st_next(&st);

	found = atoi(tmp);

	st_finalize(&st);

	return found;
}


/*
 * Stupidh test main.
 *

int main(int argc, char *argv[]) {

	char *st;
	int ratio = 0;

 	st = _gl_userfile_add_stats("WKUP 1 2345 3 443 182383 2232", 3, 940, 10, 1);

	printf("stats: %s\n", st);

	ratio = gl_userfile_get_ratio("/export/f00/ftp-data/users/flower", 0);
	printf("ratio: %d\n", ratio);

	gl_userfile_add_stats("/export/f00/ftp-data/users/flower", 10, 1024000, 120, ratio*1024000, 1, 1);

	printf("creds: %s\n", _gl_userfile_add_credits("CREDITS 123 49281 3012", 301923, 1));
}

**/