#include "login.h"

#include <array.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <util.h>

#define ACCOUNT_FILE "passwd"


typedef struct
{
    char * username;
    char * password;
}Login;


typedef struct
{
    Array super;
    Login array[];
}Authentication;


#define AUTHENTICATION(T)((Authentication*) T)


static void
authentication_delete(void * self)
{
    if(self == NULL) return;

    for(size_t i = 0; i < AUTHENTICATION(self)->super.size; i++)
    {
        if(AUTHENTICATION(self)->array[i].username != NULL)
            free(AUTHENTICATION(self)->array[i].username);

        if(AUTHENTICATION(self)->array[i].password != NULL)
            free(AUTHENTICATION(self)->array[i].password);
    }

    free(self);
}


Authentication * 
load_authentication(FILE * f)
{
    char username[128];
    char password[128];
    size_t length = 0;
    Authentication * authentication = 
        (Authentication*) array_new(sizeof(Login), 0, authentication_delete);


    while(fscanf(f, "%[^:]:%[^\n]\n", username, password) > 0)
    {
        authentication = 
            (Authentication*) array_resize(
                                ARRAY(authentication)
                                , length+1);
        
        authentication->array[length].username = strdup(username);
        authentication->array[length].password = strdup(password);
        length ++;
    }

    return authentication;
}


bool
system_login(
    const char * username
    , const char * password)
{
    FILE * f = fopen(ACCOUNT_FILE, "r");

    if(f == NULL) 
        return false;

    Authentication * authentication = load_authentication(f);

    fclose(f);

    if(authentication == NULL) 
        return false;
    
    for(size_t i = 0; i < authentication->super.size; i++)
    {
        if(strcmp(username, authentication->array[i].username) == 0)
        {
            char digest[SHA256_DIGEST_LENGTH] = {0};
            char crypted_string[SHA256_DIGEST_LENGTH*2+1];

            SHA256((unsigned char *) password, strlen(password), (unsigned char*) &digest);            

            for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
                sprintf(&crypted_string[i*2], "%02x", (unsigned int)digest[i]);    

            if(strcmp(authentication->array[i].password, crypted_string) == 0)
            {
                array_delete(ARRAY(authentication));
                return true;
            }
        }        
    }
    
    array_delete(ARRAY(authentication));

    return false;
}


