/*****************************************************************************/
/** stringify.c : Convert file to c string format. ***************************/
/** Example: ./a.out quine.c >> payloadString.c ******************************/
/*****************************************************************************/
/** Author : ttrossea ********************************************************/
/*****************************************************************************/
/** sha : e43c56ac4d5c9bcd4ac4beb0b67d40a141a14945 ***************************/
/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_WIDTH		42
#define VARIABLE_NAME		"quine"
typedef unsigned char 		uchar;
typedef unsigned int 		uint;

int 			getDevice(char *dev_path, char *out_path, int *dev, int *out)
{
	mode_t		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if (!dev_path || !dev)
		return (1);
	if ((*dev = open(dev_path, O_RDONLY, mode)) == -1)
	{
		printf("Can't open %s !\n", dev_path);
		return (1);
	}
	if (out && out_path && \
		(*out = open(out_path, O_WRONLY | O_CREAT, mode)) == -1)
	{
		printf("Can't open %s !\n", out_path);
		close(*dev);
		return (1);
	}
	return (0);
}

uint 		my_strlen(const char *s)
{
	uint 		ret = 0;

	while (s[ret] != '\0')
		ret++;
	return (ret);
}

int 		say(const char *blah)
{
	uint 		slen;

	slen = my_strlen(blah);
	write(1, (const void *)blah, slen);
	return (1);
}

int 		my_abort(int *fd, int dummy)
{
	dummy = 707;
	close(fd[0]);
	close(fd[1]);
	return (say("Abort\n"));
}

void		print_len(char *buf, uint n_bytes)
{
	char		nl;

	if (!buf)
		return ;
	nl = '\n';
	while (n_bytes--)
		write(1, (const void *)(buf++), 1);
	write(1, &nl, 1);
}

int			scan(char *table, uint table_size, char cmp)
{
	int		i;

	if (!table || table_size == 0)
		return (0);
	i = -1;
	while (++i < table_size && cmp != *table)
		table++;
	if (i < table_size && cmp == *table)
		return (i);
	return (-1);
}

int 		to_cstr(int fd, char *buf, uint buf_size, uint n_bytes)
{
	char	trig[7] = {'\0', '\\', '\n', '\t', '\r', '"', '@'};
	char	escp[6] = {'\0', '\\', 'n', 't', 'r', '"'};
	int		code;
	uint	i;

	i = -1;
	while (++i < n_bytes)
	{
		escp[0] = *(buf++);
		if ((code = scan(trig, 6, escp[0])) < 1)
		{
			write(fd, &escp[0], 1);
			if (!code)
				return (1);
			continue;
		}
		if (code != 4) // Don't write Return.
		{
			write(fd, &escp[1], 1);
			write(fd, &escp[code], 1);
		}
	}
	return (n_bytes == buf_size ? 0 : 1);
}

void		openComments(int fd)
{
	char		str[] = "/** ";

	write(fd, (const void *)str, sizeof(char) * 4);
}

void		closeComments(int fd)
{
	char		str[] = " **/\n";

	write(fd, (const void *)str, sizeof(char) * 5);
}

void		cString_declareStack(int fd, const char *varName)
{
	char		vTypeA[5];
	char		vTypeB[5];

	if (!varName || fd < 0)
		return ;
	strcpy(vTypeA, "char\t");
	strcpy(vTypeB, "[] = ");
	write(fd, vTypeA, 5);
	write(fd, varName, my_strlen(varName));
	write(fd, vTypeB, 5);
}

void		cString_newlineIndent(int fd)
{
	char			sym[] = "\"\\\n\t ";

	write(fd, &sym[0], 1);
	write(fd, &sym[4], 1);
	write(fd, &sym[1], 1);
	write(fd, &sym[2], 1);
	write(fd, &sym[3], 1);
	write(fd, &sym[3], 1);
}

void		cString_close(int fd)
{
	char			sym[] = "\";\n";

	write(fd, (const void *)sym, sizeof(char) * 3);
}

void		payloadDelimiter(int fd, const char *variableName)
{
	openComments(fd);
	write(fd, variableName, sizeof(char) * my_strlen(variableName));
	closeComments(fd);
}

void		printStats(unsigned long bytes_read, char *inFile, char *outFile)
{
	char		stdin_[] = "[Standard Output]";

	if (!outFile)
		outFile = stdin_;
	printf("\n\n----------------------\n");
	printf("Total size : %lu\n", bytes_read);
	printf("Input : %s\n", inFile);
	printf("Output: %s\n", outFile);
	printf("----------------------\n");
}

int		stringify(int *fd, char *buffer, uint buf_size, const char *varName)
{
	unsigned long 		total_bytes = 0;
	uchar 				done = 0;
	ssize_t				bytes_read;
	char				sym[] = "\"\\\n\t ;";

	payloadDelimiter(fd[1], varName);
	cString_declareStack(fd[1], varName);
	while (done == 0)
	{
		write(fd[1], &sym[0], 1);
		bytes_read = read(fd[0], (void *)buffer, buf_size);
		if (bytes_read >= 0)
		{
			done = to_cstr(fd[1], buffer, buf_size, bytes_read);
			total_bytes += (unsigned long)bytes_read;
			if (done == 0) /** Newline indentation **/
				cString_newlineIndent(fd[1]);
		}
		if (bytes_read == -1)
			return (my_abort(fd, say("read error\n")));
	}
	cString_close(fd[1]);
	payloadDelimiter(fd[1], varName);
	return (0);
}

int 		main(int ac, char **av)
{
	char				buffer[BUFFER_WIDTH];
	int 				fd[2];
	char 				*input;
	char 				*output;

	if (ac != 2 && ac != 3)
	{
		printf("Usage : %s \"source.c\" \"payload.c\"\n", av[0]);
		return (0);
	}
	input = av[1];
	output = ac == 3 ? av[2] : NULL;
	if (getDevice(input, output, &fd[0], ac == 3 ? &fd[1] : NULL))
		return (say("Abort.\n"));
	fd[1] = ac == 3 ? fd[1] : 1;
	stringify(fd, buffer, BUFFER_WIDTH, VARIABLE_NAME);
	if (fd[1] > 1)
		printStats(fd[1], input, output);
	else
		my_abort(fd, say("DONE\n"));
	return (0);
}
