#include "utils.h"

float getUnixTime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (float)((int)tv.tv_sec % 1000000) + (float)(tv.tv_usec) / 1.0e6;
}

char *getSystemTime()
{
	char *format;

	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	format = (char *)malloc(16);

	sprintf(format, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	return format;
}

int exec(char *cmd, char *buf, int size)
{
	FILE *fp;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		return -1;
	}

	fgets(buf, size, fp);

	pclose(fp);

	return 0;
}

int getSongInfo(char *artist, char *title)
{
	char buff[256];
	char newArtist[128], newTitle[128];
	buff[0] = newArtist[0] = newTitle[0] = '\0';

	
	if (exec("playerctl metadata artist", buff, sizeof(buff))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	if (strlen(buff))
		strcpy(newArtist, buff);
	
	if (exec("playerctl metadata title", buff, sizeof(buff))) {
		printf("Playerctl doesn't exist\n");
		return -1;
	}

	printf("%s\n", buff);

	if (strlen(newArtist)) {
		strcpy(artist, newArtist);
		strcpy(title, buff);
		return 0;
	}

	if (!strlen(buff)) {
		artist[0] = title[0] = '\0';
		exec("rm --force image.jpg", buff, sizeof(buff));
		return 2;
	}

	bool dash = false;
	int artistPos = 0, titlePos = 0;
	for (int i = 0; i < strlen(buff); i++) {
		if (i && buff[i-1] == ' ' && buff[i] == '-' && buff[i+1] == ' ') {
			dash = true;
			i++;
			continue;
		}

		if (dash && (buff[i] == '(' || buff[i] == '['))
			break;

		if (!dash) {
			newArtist[artistPos++] = buff[i];
		} else {
			newTitle[titlePos++] = buff[i];
		}
	}
	newArtist[artistPos] = newTitle[titlePos] = '\0';

	if (!strlen(newTitle)) {
		strcpy(newTitle, newArtist);
		newArtist[0] = '\0';
	}

	if (strlen(newTitle) && newTitle[strlen(newTitle)-1] == ' ')
		newTitle[strlen(newTitle)-1] = '\0';

	if (strlen(newArtist) && newArtist[strlen(newArtist)-1] == ' ')
		newArtist[strlen(newArtist)-1] = '\0';

	if (!strcmp(artist, newArtist) && !strcmp(title, newTitle))
		return 0;

	strcpy(artist, newArtist);
	strcpy(title, newTitle);

	return 1;
}

void getAlbumArt(SongInfo *songInfo) {
	char *cmd;
	char buff[16];

	cmd = (char *)malloc(20 + strlen(songInfo->artist) + strlen(songInfo->title));

	sprintf(cmd, "./albumArt \"%s %s\"", songInfo->artist, songInfo->title);

	if (exec(cmd, buff, sizeof(buff))) {
		printf("albumArt file not found\n");
		return;
	}
	
	if (cfg.debug && strlen(buff))
		printf("%s\n", buff);
	if (!strlen(buff)) {
		songInfo->newAlbumArt = true;
		if (cfg.debug)
			printf("New album art\n");
	} else {
		if (cfg.debug)
			printf("No album art found\n");
	}

	free(cmd);
}

void *updateSongInfo(void *arg)
{
	while (true) {
		SongInfo *songInfo = (struct SongInfo *)arg;


		int status = getSongInfo(songInfo->artist, songInfo->title);
		if (status == -1)
			return NULL;
		
		if (cfg.debug)
			printf("%s\n%s\n\n", songInfo->artist, songInfo->title);

		if (status == 1)
			getAlbumArt(songInfo);
		else if (status == 2)
			songInfo->newAlbumArt = true;

		sleep(1);
	}

	return NULL;
}