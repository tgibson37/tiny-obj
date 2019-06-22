#ifndef fileread_h
#define fileread_h

int fileRead(char* name, char* buff, int bufflen);
#define MAX_UNIT 10

FILE* fileUnit[MAX_UNIT];

#endif  /* fileread_h */
