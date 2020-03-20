#ifndef __PTS_H__
#define __PTS_H__

int ptsclose(int fd);
int ptsopen(const char* ptsname);
int ptsprintf(int ptsfd, const char* stream, ...);

#endif
