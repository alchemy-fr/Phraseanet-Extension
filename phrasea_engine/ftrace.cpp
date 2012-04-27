#include "base_header.h"
void realftrace(char *file, int line, const char *function, char *fmt, ...)
{
	return;		// !!!!!!!!!!!!!!!!!!!!!!!!!!!
	FILE * pFile;
	if((pFile = fopen("C:\\WINDOWS\\TEMP\\phrasea_extension.log", "a")))
	{
		fprintf(pFile, "%s [%d]: %s(...)\t", file, line, function);
		va_list args;
		va_start(args, fmt);
		vfprintf(pFile, fmt, args);
		va_end(args);
		fclose(pFile);
	}
}
