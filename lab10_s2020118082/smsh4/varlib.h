// *varlib.h*
// declares environment—related functions 

// keep val associated with var
int VLstore(char* name, char* val);
// return value of var
char* VLlookup(char* name);
// adds name to list of env vars
int VLexport(char* name);
// performs the shell's set command
void VLlist();
// copy from environ to table
int VLenviron2table(char* env[]);
// copy from table to environ
char** VLtable2environ(); 

