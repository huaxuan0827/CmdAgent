#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int grantpt(int fd);
int unlockpt(int fd);
int ptsname_r(int fd, char *buf, size_t buflen);
//int vdprintf(int fd, const char *format, va_list ap); 
int posix_openpt(int flags);

int ptsclose(int fd)
{
	return close(fd);
}

int ptsopen(const char* ptsname)
{
	struct termios term_a;
	int fd_ptmx = -1;
	char ptsdev[32] = {0};
	int retval = 0;

	if ((fd_ptmx = posix_openpt(O_RDWR | O_NOCTTY | O_NDELAY)) >= 0)
	{
		/* grant access to slave & clear slave's lock flag */
		if (grantpt(fd_ptmx) == 0 && unlockpt(fd_ptmx) == 0)
		{
			/* get slave's name */
			memset(ptsdev, 0, 32);
			if (ptsname_r(fd_ptmx, ptsdev, 32) == 0)
			{
				printf("ptsname %s\n", ptsdev);

				if	(tcgetattr(fd_ptmx, &term_a) == 0)
				{
					term_a.c_lflag = 0;
					term_a.c_cflag |= CLOCAL;//ignore modem control lines
					term_a.c_cflag |= CREAD; //enable receiver
					term_a.c_cflag &= ~CSIZE;

					term_a.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	/*Input*/
					term_a.c_oflag &= ~OPOST;	/*Output*/
					term_a.c_iflag &= ~(INLCR | IGNCR | ICRNL);
					term_a.c_oflag &= ~(ONLCR | OCRNL);

					term_a.c_cflag &= ~CRTSCTS; //???
					//??? XON/XOFF ???
					term_a.c_iflag &= ~IXOFF;//???
					//??? XON/XOFF ???
					term_a.c_iflag &= ~IXON ;//???
					//RTS/CTS (??) ????
					term_a.c_iflag &= ~IXON ;//???

					if (tcsetattr(fd_ptmx, TCSANOW, &term_a) == 0)
					{
						if (tcflush(fd_ptmx, TCIOFLUSH) == 0)
						{
							if (ptsname)
							{
								/* remove the possible link */
								retval = unlink(ptsname);
								if (retval == 0 || errno == ENOENT)
								{
									/* create link */
									printf("%s -> %s\n", ptsname, ptsdev);
									if (symlink(ptsdev, ptsname) == 0)
									{
										return fd_ptmx;
									}
									unlink(ptsname);
								}
							}
						}
					}

				}
			}
		}
	}
	close(fd_ptmx);
	return -1;
}

int ptsprintf(int ptsfd, const char* stream, ...)
{
	int retval = 0;
	va_list arg_ptr;
	
	if (ptsfd > 0) {
		va_start(arg_ptr, stream);
		retval = vdprintf(ptsfd, stream, arg_ptr);
		va_end(arg_ptr);

#ifdef __PTS_DEBUG
        va_start(arg_ptr, stream);
        retval = vprintf(stream, arg_ptr);
        va_end(arg_ptr);
#endif
	}

	return retval;

}

