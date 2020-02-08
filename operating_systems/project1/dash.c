#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

enum commands {NORMAL_COMMAND, EXIT_COMMAND, CD_COMMAND, PATH_COMMAND};

static char * SLASH_STRING = "/";
static char * HOME_STRING = "/home/";
static int HOME_STRING_LENGTH = 6;

const char * EXIT_STRING = "exit";
const char * CD_STRING = "cd";
const char * PATH_STRING = "path";

static char workingDirectory[256];
static char pathString[1024] = "/bin:/usr/bin";

void startDash();
void runDash();
void parseFile(char * fileName);
void printPrompt();
char * getUserInput();
void parseUserInput(char * userInput);
void parseCommand(char * command);
void normalCommand(char * command);
void pathCommand(char ** command);
void cdCommand(char ** command);
int matchBuiltIn(char * command);
void getWorkingDirectory();
char ** parseByToken(char * line, char token);
int indexOf(char str[], char searchingFor);
char * newEnd(char * str, int index);
char * newStart(char * str, int index);
char * cleanInput(char * str);
char * removeLeadingWhitespace(char * str);
char * removeTrailingWhitespace(char * str);

int main(int argc, char *argv[]) {
	if(argc > 1) {
		parseFile(argv[1]);
	}
	else {
		startDash();
	}
	return 0;
}

void parseFile(char * fileName) {
	FILE *fp = fopen(fileName, "r");
	char * line = NULL;
	size_t size;
	while(getline(&line, &size, fp) != -1) {
		parseUserInput(line);
	}
	fclose(fp);
}

void startDash() {
	runDash();
}

void runDash() {
	printPrompt();
	char * userInput = getUserInput();
	parseUserInput(userInput);
	runDash();
}

void printPrompt() {
	getWorkingDirectory();
	printf("dash %s > ", workingDirectory);
}

char * getUserInput() {
	char * userInput = NULL;
	size_t size;
	getline(&userInput, &size, stdin);
	return userInput;
}

void parseUserInput(char * userInput) {
	char ** commands = parseByToken(userInput, '&');
	int index = 0;
	while(**(commands + index) != '\0') {
		parseCommand(*(commands + index));
		index++;
	}
}

void parseCommand(char * command) {
	char ** commandTokens = parseByToken(command, ' ');
	switch(matchBuiltIn(*(commandTokens))) {
		case NORMAL_COMMAND:
			normalCommand(command);
			break;
		case EXIT_COMMAND:
			exit(0);
			break;
		case CD_COMMAND:
			cdCommand(commandTokens);
			break;
		case PATH_COMMAND:
			pathCommand(commandTokens);
			break;
		default:
			return;
			break;
	}
}

int matchBuiltIn(char * command) {
	if(strcmp(command, EXIT_STRING) == 0) {
		return EXIT_COMMAND;
	}
	else if(strcmp(command, PATH_STRING) == 0) {
		return PATH_COMMAND;
	}
	else if(strcmp(command, CD_STRING) == 0) {
		return CD_COMMAND;
	}
	else {
		return NORMAL_COMMAND;
	}
}

void normalCommand(char * command) {
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork error");
	}
	else if (pid == 0) {
		/* int argIndex = indexOf(command, ' '); */
		/* char * args = malloc(sizeof(char) * (argIndex)); */
		/* char args[strlen(command) - argIndex]; */
		/* strcpy(args, command + argIndex + 1); */
		/* *(command + argIndex) = '\0'; */
		char ** paths = parseByToken(pathString, ':');
		int index = 0;
		while(**(paths + index) != '\0') {
			char * path = *(paths + index);
			char commandWithPath[strlen(path) + strlen(command) + 1];
			strcpy(commandWithPath, path);
			strcpy(commandWithPath + strlen(path), SLASH_STRING);
			strcpy(commandWithPath + strlen(path + index) + 1, command);
			struct stat fileStat;
			if(stat(commandWithPath, &fileStat) >= 0) {
				execl(commandWithPath, "-l", NULL);
				exit(1);
			}
			else {
				index++;
			}
		}
	}
	else {
		waitpid(pid, NULL, 0);
	}
}

void pathCommand(char ** command) {
	if(**(command + 1) == '\0') {
		printf("%s\n", pathString);
	}
	else {
		strcpy(pathString, *(command + 1));
	}
}

void cdCommand(char ** command) {
	chdir(*(command + 1));
}

void getWorkingDirectory () {
	getcwd(workingDirectory, sizeof(workingDirectory));

	if(strstr(workingDirectory, HOME_STRING) != NULL) {
		char newwd[strlen(workingDirectory)];
		strncpy(newwd, workingDirectory + HOME_STRING_LENGTH, strlen(workingDirectory) - HOME_STRING_LENGTH);
		int nameLength = indexOf(newwd, '/');
		strncpy(workingDirectory + 1, newwd + nameLength, strlen(newwd) - nameLength);
		workingDirectory[0] = '~';
		workingDirectory[strlen(workingDirectory) - HOME_STRING_LENGTH - nameLength] = '\0';
	}
}

char ** parseByToken(char * line, char token) {
	int size = 0;
	char ** strings = malloc(sizeof(char *));
	line = cleanInput(line);
	char lineArray[strlen(line)];
	strcpy(lineArray, line);
	int nextTokenIndex = indexOf(lineArray, token);
	while(nextTokenIndex != -1) {
		size++;
		strings = (char **) realloc(strings, (sizeof(char *) * (size)));
		*(strings + size - 1) = (char *) malloc(sizeof(char) * (nextTokenIndex - 1));
		strncpy(*(strings + size - 1), lineArray, nextTokenIndex);
		*(strings + size - 1) = cleanInput(*(strings + size - 1));
		strcpy(lineArray, lineArray + nextTokenIndex + 1);
		nextTokenIndex = indexOf(lineArray, token);
	}
	strings = (char **) realloc(strings, (sizeof(char *) * (size + 2)));
	*(strings + size) = (char *) malloc(sizeof(char) * strlen(lineArray));
	strcpy(*(strings + size), lineArray);
	*(strings + size) = cleanInput(*(strings + size));
	*(strings + size + 1) = (char *) malloc(sizeof(char));
	**(strings + size + 1) = '\0';

	return strings;
}

int indexOf(char str[], char searchingFor) {
	for(int i = 0; i < strlen(str); i++) {
		if(str[i] == searchingFor) {
			return i;
		}
	}
	return -1;
}

char * newEnd(char * str, int index) {
	char newstr[index];
	strncpy(newstr, str, index);
	char *newstrp = (char *)malloc(strlen(newstr)+1);
	strcpy(newstrp, newstr);
	return newstrp;
}

char * newStart(char * str, int index) {
	char newstr[strlen(str)];
	strncpy(newstr, str + index, strlen(str) - index);
	newstr[strlen(str) - index] = '\0';
	char *newstrp = (char *)malloc(strlen(newstr)+1);
	strcpy(newstrp, newstr);
	return newstrp;
}

char * cleanInput(char * str) {
	str = removeLeadingWhitespace(str);
	str = removeTrailingWhitespace(str);
	return str;
}

char * removeLeadingWhitespace (char * str) {
	while(isspace(*str)) {
		str++;
	}
	return str;
}

char * removeTrailingWhitespace (char * str) {
	char newstr[strlen(str)];
	strcpy(newstr, str);
	int chars = 1;
	while(isspace(newstr[strlen(newstr) - chars])) {
		chars++;
	}
	newstr[strlen(newstr) - chars + 1] = 0;
	strcpy(str, newstr);
	return str;
}
