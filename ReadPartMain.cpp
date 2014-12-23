#include <Windows.h>
#include <stdio.h>

void PrintHelpText()
{
	printf("\nReadPart v1.0 - Ryan Ries 2014, myotherpcisacloud.com\n");
	printf("อออออออออออออออออออออออออออออออออออออออออออออออออออออ\n");	
	printf("\nRead either the beginning or end part of a file.\n\n");
	printf("E.g. C:\\> readpart c:\\files\\huge.log last 1024 bytes\n");	
	printf("Reads the last 1KB of huge.log and displays it in the console.\n\n");
	printf("E.g. C:\\> readpart C:\\files\\huge.log first 10 lines\n");
	printf("Reads the first 10 lines of huge.log and displays it in the console.\n\n");
	printf("Arguments are not case sensitive. Enclose file paths with spaces\n");
	printf("in them with quotation marks.\n");
	printf("Long file paths (32000 characters) are supported.\n");
}

int main(int argc, char *argv[])
{
	LARGE_INTEGER FileSize, AmountToRead = { 0 };
	DWORD BytesRead = 0;	
	char ReadBuffer[1024] = {};
	// NTFS supports paths up to 32767. If path is greater than MAX_PATH (260,) need to prefix with \\?\.
	// It's OK if we always prefix with \\?\, even with short file paths.
	char FilePath[32000] = { '\\', '\\', '?', '\\', 0 };
	int StartFromEndOfFile = 0;
	int ReadInLines = 0;		

	if (argc != 5)
	{
		PrintHelpText();
		return(1);
	}

	// Loop through all command-line arguments, convert them to lower case,
	// then validate them.
	// E.g. C:\> readpart c:\files\huge.log last 1024 bytes
	for (int ArgNumber = 1; ArgNumber < argc; ArgNumber++)
	{
		char *arg = argv[ArgNumber];

		for (int y = 0; y < strlen(arg); y++)		
			arg[y] = (char)tolower(arg[y]);
		
		// We are on the first argument, i.e. file path
		if (ArgNumber == 1)
		{
			if (strlen(arg) > sizeof(FilePath))
			{
				printf("\nThe specified file path is too long.\n");
				return(1);
			}
			for (int z = 0; z < strlen(arg); z++)
				FilePath[z + 4] = arg[z];
		}

		// We are on the second argument, i.e. first or last
		if (ArgNumber == 2)
		{
			if (strcmp(arg, "last") == 0)
			{
				StartFromEndOfFile = 1;
			}
			else if (strcmp(arg, "first") == 0)
			{
				// Do nothing because StartFromEndOfFile is already false by default.
			}
			else
			{
				printf("\nSecond argument must be either \"first\" or \"last\"!\n");
				return(1);
			}
		}

		// We are on the third argument, i.e. a number of bytes or number of lines
		if (ArgNumber == 3)
		{
			AmountToRead.QuadPart = strtoull(arg, NULL, 10);
			if (AmountToRead.QuadPart < 1 || AmountToRead.QuadPart > 107374182400)
			{
				printf("\nNumber of bytes or lines to read was out of range!\n");
				return(1);
			}
		}

		// We are on the fourth argument, i.e. bytes or lines
		if (ArgNumber == 4)
		{
			if (strcmp(arg, "bytes") == 0)
			{
				// Do nothing because ReadInLines is already 0 by default.
			}
			else if (strcmp(arg, "lines") == 0)
			{
				ReadInLines = 1;
			}
			else
			{
				printf("\nFourth argument must be either \"bytes\" or \"lines\"!\n");
				return(1);
			}
		}		
	}

	HANDLE FileHandle = CreateFile(FilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		printf("\nCould not open file! (Error Code %d)\n", GetLastError());
		return(1);
	}

	if (GetFileSizeEx(FileHandle, &FileSize) == 0)
	{
		printf("\nUnable to get file size! (Error Code %d)\n", GetLastError());
		return(1);
	}

	if (AmountToRead.QuadPart >= FileSize.QuadPart)
	{
		AmountToRead.QuadPart = FileSize.QuadPart;
	}


	DWORD64 TotalBytesRead = 0;
	DWORD NewLinesFound = 0;

	if (StartFromEndOfFile)
	{
		if (ReadInLines)
		{
			// Read the last X lines from the file.

			LARGE_INTEGER CurrentFilePosition = FileSize;
			
			while (TotalBytesRead < (DWORD64)FileSize.QuadPart)
			{
				if (NewLinesFound >= AmountToRead.QuadPart)
					break;

				char SingleCharReadBuffer = 0;
				if (SetFilePointerEx(FileHandle, CurrentFilePosition, NULL, FILE_BEGIN) == 0)
				{
					printf("\nUnable to seek to position in file! (Error Code %d)\n", GetLastError());
					return(1);
				}
				if (ReadFile(FileHandle, &SingleCharReadBuffer, 1, &BytesRead, NULL) == 0)
				{
					printf("\nUnable to read from file! (Error Code %d)\n", GetLastError());
					return(1);
				}

				if (SingleCharReadBuffer == '\n')
					NewLinesFound++;

				TotalBytesRead++;
				CurrentFilePosition.QuadPart--;
			}

			while (ReadFile(FileHandle, ReadBuffer, sizeof(ReadBuffer), &BytesRead, NULL) && BytesRead)
			{
				for (DWORD x = 0; x < BytesRead; x++)
					printf("%c", ReadBuffer[x]);
			}
		}
		else
		{
			// Read the last X bytes from the file.
			// Set the file pointer to negative AmountToRead bytes from the end of file.
			AmountToRead.QuadPart *= -1;
			if (SetFilePointerEx(FileHandle, AmountToRead, NULL, FILE_END) == 0)
			{
				printf("\nUnable to seek to position in file! (Error Code %d)\n", GetLastError());
				return(1);
			}
			while (ReadFile(FileHandle, ReadBuffer, sizeof(ReadBuffer), &BytesRead, NULL) && BytesRead)
			{
				for (DWORD x = 0; x < BytesRead; x++)
					printf("%c", ReadBuffer[x]);				
			}
		}
	}
	else
	{			
		if (ReadInLines)
		{
			// Read the first X lines from the file.
			
			while (TotalBytesRead < (DWORD64)FileSize.QuadPart)
			{
				if (NewLinesFound >= AmountToRead.QuadPart)
					break;

				DWORD64 BytesRemaining = (DWORD64)FileSize.QuadPart - TotalBytesRead;
				if (BytesRemaining < sizeof(ReadBuffer))
					ReadFile(FileHandle, ReadBuffer, (DWORD)BytesRemaining, &BytesRead, NULL);
				else
					ReadFile(FileHandle, ReadBuffer, sizeof(ReadBuffer), &BytesRead, NULL);

				TotalBytesRead += BytesRead;

				for (DWORD x = 0; x < BytesRead; x++)
				{
					if (ReadBuffer[x] == '\n')
						NewLinesFound++;

					if (NewLinesFound >= AmountToRead.QuadPart)
						break;

					printf("%c", ReadBuffer[x]);
				}
			}
		}
		else
		{
			// Read number of bytes starting from the beginning of the file.
			while (TotalBytesRead < (DWORD64)AmountToRead.QuadPart)
			{
				DWORD64 BytesRemaining = (DWORD64)AmountToRead.QuadPart - TotalBytesRead;
				if (BytesRemaining < sizeof(ReadBuffer))				
					ReadFile(FileHandle, ReadBuffer, (DWORD)BytesRemaining, &BytesRead, NULL);				
				else				
					ReadFile(FileHandle, ReadBuffer, sizeof(ReadBuffer), &BytesRead, NULL);
				
				TotalBytesRead += BytesRead;
				for (DWORD x = 0; x < BytesRead; x++)
					printf("%c", ReadBuffer[x]);
			}			
		}
	}

	printf("\n");
	return(0);
}
