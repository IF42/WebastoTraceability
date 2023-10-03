#include "login.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <openssl/sha.h>


#define ACCOUNT_FILE "passwd"


typedef struct
{
	size_t length;

	struct User
	{
		char username[128];
		char password[128];
	}user[];
}Login;


Login *
load_authentication(FILE * f)
{
    char username[128];
    char password[128];

    Login * login = NULL;
    size_t length = 0;

    while(fscanf(f, "%[^:]:%[^\n]\n", username, password) > 0)
    {
        if(login == NULL)
             login = malloc(sizeof(Login) + sizeof(struct User)*2);
        else if(length >= login->length)
            login = realloc(login, sizeof(Login) + sizeof(struct User)*length*2);

        strncpy(login->user[length].username, username, 128);
        strncpy(login->user[length].password, password, 128);

        length ++;
    }

    login->length = length;

    if(login != NULL)
        login = realloc(login, sizeof(Login) + sizeof(struct User)*length);

    return login;
}


bool
system_login(
    const char * username
    , const char * password)
{
    FILE * f = fopen(ACCOUNT_FILE, "r");

    if(f == NULL) 
        return false;

    Login * login = load_authentication(f);

    fclose(f);

    if(login == NULL)
        return false;
    
    for(size_t i = 0; i < login->length; i++)
    {
        if(strcmp(username, login->user[i].username) == 0)
        {
            char digest[SHA256_DIGEST_LENGTH] = {0};
            char crypted_string[SHA256_DIGEST_LENGTH*2+1];

            SHA256((unsigned char *) password, strlen(password), (unsigned char*) &digest);            

            for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
                sprintf(&crypted_string[i*2], "%02x", (unsigned int)digest[i]);    

            if(strcmp(login->user[i].password, crypted_string) == 0)
            {
                free(login);
                return true;
            }
        }        
    }
    
    free(login);

    return false;
}


