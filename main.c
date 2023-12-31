#include <stdint.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "jsmn/jsmn.h"
#include "config.h"

typedef struct{
    size_t len;
    char *data;
}buf;

typedef struct string{
    char *verse;
    struct string *next;
}Node;

static size_t WriteCallback(void *data, size_t size, size_t nmemb, void *userp){
    size_t rsize = size * nmemb;
    buf *mem = (buf *) userp;

    char *ptr = realloc(mem->data, mem->len + rsize + 1);
    if(!ptr){
        perror("realloc");
        return 0;
    }
    mem->data = ptr;
    memcpy(&(mem->data[mem->len]), data, rsize);
    mem->len += rsize;
    mem->data[mem->len] = 0;

    return rsize;
}

char *getmsg(char *path){
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    buf mem;
    mem.data = malloc(1);
    mem.len = 0;
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, path);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&mem);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            free(mem.data);
            exit(EXIT_FAILURE);
        }

            curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return (mem.data);
}

char *getpath(int r_num){

    int length = snprintf( NULL, 0, "%d", r_num );
    char* str = malloc( length + 1 );
    snprintf( str, length + 1, "%d", r_num );
    int size = URL_SIZE + length + URL_END_SIZE + strlen(lang) + 1;
    char *urlP = malloc(size * sizeof(char));

    if(urlP == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(urlP, URL);
    strcat(urlP, lang);
    strcat(urlP, str);
    free(str);
    strcat(urlP, ".json");
    return urlP;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

void free_list(Node **surah){
    Node *next;
    while(*surah){
        next = (*surah)->next;
        free((*surah)->verse);
        free((*surah));
        (*surah) = next;
    }
    *surah = NULL;
}

void print_surah(Node *surah){
    while (surah) {
        puts(surah->verse);
        surah = surah->next;
    }
}

void print_rand_ayat(Node *surah, size_t ssize){
    srand(time(0));
    ssize--;
    int ind = 1;
    int r_ayat = rand() % ssize + 1;
    while(surah){
        if(ind == r_ayat) break;
        surah = surah->next;
        ind++;
    }
    printf("ayat: %d\n", ind);
    puts(surah->verse);
}

Node *parse_json(char *json, uint16_t *surah_size){
    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t tokens[2048];

    int ret = jsmn_parse(&parser, json, strlen(json), tokens, 2048);

    if(ret < 0){
        exit(EXIT_FAILURE);
    }

    Node *surah = (Node *)malloc(sizeof(Node));
    if(surah == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    Node *node = surah;
    uint8_t flag = 1;
    *surah_size = 0;

    for(int i = 1; i < ret; i++){
        if(jsoneq(json, &tokens[i], "text") == 0){
            int len = tokens[i + 1].end - tokens[i + 1].start;
            char s[len + 1];
            strncpy(s, (json + tokens[i + 1].start), len);
            s[len] = '\0';
            if(flag){
                node->verse = (char*)malloc((len + 1) * sizeof(char));
                strcpy(node->verse, s);
                node->verse[len] = '\0';
                flag = 0;
            } else {
                node->next = (Node *) malloc(sizeof(Node));
                if(node->next == NULL){
                    free_list(&surah);
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                node = node->next;
                node->verse = (char*)malloc((len + 1) * sizeof(char));
                strcpy(node->verse, s);
                node->verse[len] = '\0';
                node->next = NULL;
            }
            (*surah_size)++;
            i++;
        } 
    }
    return surah;
}

int main(int argc, char **argv) {
    if(argc > 1){
        srand(time(0));
        int r_num = rand() % 114 + 1;
        char *path = getpath(r_num);
        char *json = getmsg(path);
        free(path);
        uint16_t size = 0;
        Node *surah = parse_json(json, &size);
        free(json);

        printf("Surah: %d\n", r_num);
        if(strcmp(argv[1], "-a") == 0){
            print_rand_ayat(surah,size);
        } else if(strcmp(argv[1], "-s") == 0){
            print_surah(surah);
        }
        free_list(&surah);
        atexit(curl_global_cleanup);
    }
    

    return 0;
}
