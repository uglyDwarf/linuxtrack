
#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <getopt.h>
#include "game_data.h"
#include "utils.h"

typedef struct{
  off_t length;
  unsigned char *data;
} file_buf_t;

#define CSUM_BUF_LEN 32

typedef struct{
  uint8_t buffer[CSUM_BUF_LEN];
  int index;
} csum_t;

struct spec_s{
  char *name;
  size_t length;
  uint16_t csum;
  unsigned char sha1[SHA_DIGEST_LENGTH];
  unsigned char md5[MD5_DIGEST_LENGTH];
  struct spec_s *next;
  csum_t csum_buf;
  unsigned char *buffer;
  int index;
  bool found;
};

void csum_init(csum_t *csum)
{
  csum->index = 0;
  int i;
  for(i = 0; i < CSUM_BUF_LEN; ++i){
    csum->buffer[i] = 0;
  }
}


uint16_t csum_compute(uint8_t byte, csum_t *csum)
{
  csum->buffer[csum->index] = byte;
  csum->index = (csum->index + 1) % CSUM_BUF_LEN;
  uint16_t tmp = 0;
  int i;
  for(i = 0; i < CSUM_BUF_LEN; ++i){
    if(tmp & 0x8000){
      tmp ^= 3;
    }
    tmp <<= 2;
    tmp ^= csum->buffer[(csum->index + i) % CSUM_BUF_LEN];
  }
  return tmp;
}



file_buf_t *read_contents(FILE *f, size_t len)
{
  file_buf_t *res = (file_buf_t*)malloc(sizeof(file_buf_t));
  if(res == NULL){
    printf("read_contents: Can't alloc memory for file_buf.\n");
    return NULL;
  }
  unsigned char *data = (unsigned char *)malloc(len);
  if(data == NULL){
    free(res);
    printf("read_contents: Can't alloc memory for data.\n");
    return NULL;
  }
  res->length = len;
  res->data = data;
  size_t read_in = fread(data, 1, len, f);
  if(read_in == len){
    return res;
  }else{
    printf("read_contents: Read %lu bytes instead of %lu.\n", read_in, len);
    free(res);
    free(data);
    return NULL;
  }
}

file_buf_t* read_file(const char *file)
{
  struct stat s;
  if(stat(file, &s) != 0){
    printf("Can't find file '%s'.\n", file);
    return NULL;
  }

  FILE *f = fopen(file, "rb");
  if(f == NULL){
    printf("Can't open file '%s'.\n", file);
    return NULL;
  }

  file_buf_t *res = read_contents(f, s.st_size);

  fclose(f);
  return res;
}

uint16_t hash_csum(file_buf_t *f)
{
  int i;
  csum_t csum;
  csum_init(&csum);
  uint16_t res = 0;
  for(i = 0; i < CSUM_BUF_LEN; ++i){
    res = csum_compute(f->data[i], &csum);
  }
  return res;
}

unsigned char* hash_sha1(file_buf_t *f)
{
  unsigned char *res = malloc(SHA_DIGEST_LENGTH);
  if(res == NULL){
    return NULL;
  }
  if(SHA1((unsigned char *)f->data, f->length, res) == NULL){
    free(res);
    return NULL;
  }
  return res;
}

unsigned char* hash_md5(file_buf_t *f)
{
  unsigned char *res = malloc(MD5_DIGEST_LENGTH);
  if(res == NULL){
    return NULL;
  }
  if(MD5((unsigned char *)f->data, f->length, res) == NULL){
    free(res);
    return NULL;
  }
  return res;
}

void print_hash(FILE *output, unsigned char *hash, size_t len)
{
  size_t i;
  for(i = 0; i < len; ++i){
    fprintf(output, "%02x", hash[i]);
  }
}

void process_file(FILE *output, const char *name)
{
  file_buf_t *f = read_file(name);
  unsigned char *md5sum = hash_md5(f);
  unsigned char *sha1sum = hash_sha1(f);
  char *last_slash = rindex(name, '/');
  const char *file_name;
  if(last_slash == NULL){
    file_name = name;
  }else{
    file_name = last_slash + 1;
  }
  if(md5sum && sha1sum){
    fprintf(output, "%s %llu %d ", file_name, (unsigned long long)f->length, hash_csum(f));
    print_hash(output, md5sum, MD5_DIGEST_LENGTH);
    fprintf(output, " ");
    print_hash(output, sha1sum, SHA_DIGEST_LENGTH);
    fprintf(output, "\n");
  }
  free(md5sum);
  md5sum = NULL;
  free(sha1sum);
  sha1sum = NULL;
  free(f->data);
  f->data = NULL;
  free(f);
  f= NULL;
}


int c_2_h(char c)
{
  int res = 0;
  if((c >= '0') && (c <= '9')){
    res = c - '0';
  }
  if((c >= 'A') && (c <= 'F')){
    res = c - 'A' + 10;
  }
  if((c >= 'a') && (c <= 'f')){
    res = c - 'a' + 10;
  }
  return res;
}

bool from_hex(char *str, unsigned char *hex, size_t len)
{
  if((2 * len) != strlen(str)){
    return false;
  }
  size_t p = 0;
  while(p < len){
    hex[p] = (c_2_h(str[2 * p]) * 16) + c_2_h(str[2 * p + 1]);
    p += 1;
  }
  return true;
}

struct spec_s *head = NULL;
int specs = 0;
unsigned char *tmp_buffer;
bool gamedata_found = false;

bool read_spec(const char *spec_file)
{
  char name[1024];
  int length;
  int csum;
  char md5sum[1024];
  char sha1sum[1024];
  int res;
  struct spec_s *tail = NULL;
  FILE *f = fopen(spec_file, "r");
  if(f == NULL){
    return false;
  }
  while(1){
    res = fscanf(f, "%1023s %d %d %1023s %1023s\n", name, &length, &csum, md5sum, sha1sum);
    if(res == 5){
      struct spec_s *tmp = (struct spec_s *)malloc(sizeof(struct spec_s));
      if(tmp == NULL){
        break;
      }
      tmp->buffer = (unsigned char *)malloc(length);
      if(tmp->buffer == NULL){
        break;
      }
      specs += 1;
      tmp->name = name;
      tmp->length = length;
      tmp->csum = csum;
      tmp->next = NULL;
      tmp->found = false;
      tmp->index = 0;
      csum_init(&(tmp->csum_buf));
      if(from_hex(md5sum, tmp->md5, MD5_DIGEST_LENGTH) &&
         from_hex(sha1sum, tmp->sha1, SHA_DIGEST_LENGTH)){
        if(tail == NULL){
          head = tail = tmp;
        }else{
          tail->next = tmp;
        }
        tail = tmp;
      }else{
        free(tmp);
        break;
      }
    }
    if(res != 5){
      break;
    }
  }
  fclose(f);

  //create an array from the linked list
  tail = (struct spec_s*)malloc(sizeof(struct spec_s) * specs);
  if(tail == NULL){
    return false;
  }
  int i;
  size_t max_length = 0;
  struct spec_s *ptr, *prev;
  ptr = head;
  for(i = 0; i < specs; ++i){
    tail[i] = *ptr;
    if(max_length < tail[i].length){
      max_length = tail[i].length;
    }
    prev = ptr;
    ptr = ptr->next;
    free(prev);
  }
  head = tail;
  tmp_buffer = (unsigned char *)malloc(max_length);
  return true;
}

void print_spec_list()
{
  int i;
  for(i = 0; i < specs; ++i){
    printf("%s %lu %d ", head[i].name, head[i].length, head[i].csum);
    print_hash(stdout, head[i].md5, MD5_DIGEST_LENGTH);
    printf(" ");
    print_hash(stdout, head[i].sha1, SHA_DIGEST_LENGTH);
    printf("\n");
  };
}

void free_specs()
{
  int i = 0;
  for(i = 0; i < specs; ++i){
    free(head[i].name);
    head[i].name = NULL;
    free(head[i].buffer);
    head[i].buffer = NULL;
  }
  free(head);
  head = NULL;
  free(tmp_buffer);
  tmp_buffer = NULL;
}

bool hashes_equal(unsigned char *h1, unsigned char *h2, int length)
{
  int i;
  for(i = 0; i < length; ++i){
    if(h1[i] != h2[i]){
      return false;
    }
  }
  return true;
}

bool new_char(int c, struct spec_s *spec, FILE *f)
{
  spec->buffer[spec->index++] = c;
  spec->index = (spec->index + 1) % spec->length;
  uint16_t csum = csum_compute(c, &(spec->csum_buf));
  if(csum == spec->csum){
    long tmp_pos = ftell(f);
    //printf("Have candidate for %s at %ld\n", spec->name, tmp_pos - CSUM_BUF_LEN);
    fseek(f, - CSUM_BUF_LEN, SEEK_CUR);
    if(fread(tmp_buffer, 1, spec->length, f) != spec->length){
      return false;
    }
    fseek(f, tmp_pos, SEEK_SET);
    file_buf_t tmp_fb;
    tmp_fb.data = tmp_buffer;
    tmp_fb.length = spec->length;
    unsigned char *md5 = hash_md5(&tmp_fb);
    bool hash_ok = hashes_equal(md5, spec->md5, MD5_DIGEST_LENGTH);
    free(md5);
    if(!hash_ok){
      return false;
    }
    //printf("MD5 OK!\n");
    unsigned char *sha1 = hash_sha1(&tmp_fb);
    hash_ok = hashes_equal(sha1, spec->sha1, SHA_DIGEST_LENGTH);
    free(sha1);
    if(!hash_ok){
      return false;
    }
    //printf("SHA1 OK!\n");
    char *tgt_data = ltr_int_get_default_file_name("tir_firmware/%s");
    char *full_name;
    if(asprintf(&full_name, tgt_data, spec->name) < 0){

    }
    FILE *r = fopen(full_name, "wb");
    if(r != NULL){
      if(fwrite(tmp_buffer, 1, spec->length, r) == spec->length){
        printf("  Written %s\n", spec->name);
        spec->found = true;
      }
      fclose(r);
      if(strcmp(spec->name + (strlen(spec->name) - 3), ".fw") == 0){
        const char command[] = "gzip -f -9 ";
        int command_length = strlen(full_name) + strlen(command) + 1;
        char *commandline = (char *)malloc(command_length);
        if(commandline == NULL){
          return false;
        }
        snprintf(commandline, command_length, "%s%s", command, full_name);
        printf("    Packing the firmware '%s'.\n", spec->name);
        if(system(commandline) != 0){
          printf("    Failed to pack the firmware file '%s'.\n", spec->name);
        }
        free(commandline);
        commandline = NULL;
        free(full_name);
        full_name = NULL;
        free(tgt_data);
        tgt_data = NULL;
      }
    }
  }
  return true;
}

void search_file(FILE *f)
{
  int c, i;
  for(i = 0; i < specs; ++i){
    if(!head[i].found){ //no need to search for what we found already
      csum_init(&(head[i].csum_buf));
    }
  }
  while(1){
    c = fgetc(f);
    if(c == EOF){
      break;
    }
    for(i = 0; i < specs; ++i){
      if(!head[i].found){ //no need to search for what we found already
        if(!new_char(c, &(head[i]), f)){
          break;
        }
      }
    }
  }
}

bool open_file_to_search(char *fname)
{
  if(strcmp(fname + (strlen(fname) - 7), "sgl.dat") == 0){
    printf("Decoding game data.\n");
    char *tgt_data = ltr_int_get_default_file_name("tir_firmware/gamedata.txt");
    gamedata_found = get_game_data(fname, tgt_data, false);
    free(tgt_data);
  }else{
    FILE *f = fopen(fname, "r");
    if(f == NULL){
      return false;
    }
    printf("Analyzing file %s.\n", fname);
    search_file(f);
    fclose(f);
  }
  return true;
}

bool check_missed()
{
  bool res = true;
  if(!gamedata_found){
    printf("Didn't find file sgl.dat to extract the game data from.\n");
    res = false;
  }
  int i;
  for(i = 0; i < specs; ++i){
    if(!head[i].found){
      printf("Was not able to extract '%s'.\n", head[i].name);
      res = false;
    }
  }
  return res;
}

bool update_gamedata()
{
  char *tgt_data = ltr_int_get_default_file_name("tir_firmware/gamedata.txt");
  FILE *test = fopen(tgt_data, "r");
  if(test == NULL){
    free(tgt_data);
    printf("Extract the data first, update afterwards.\n");
    return false;
  }
  fclose(test);
  char *src_data = ltr_int_get_default_file_name("tir_firmware/sgl.dat0");
  char *cmd = "wget --user-agent=\"NaturalPoint TrackIR 5.2.200\" -O \"%s\" "
              "\"http://www.naturalpoint.com/update/files/getfile.cgi?file_id=SGL&category=0\"";
  char *full_cmd;
  if(asprintf(&full_cmd, cmd, src_data) < 0){
    free(src_data);
    printf("Couldn't create command to download the gamedata update.\n");
    return false;
  }
  printf("Going to execute '%s'.\n", full_cmd);
  if(system(full_cmd)!= 0){
    free(full_cmd);
    free(src_data);
    printf("Couldn't download the gamedata update.\n");
    return false;
  }
  free(full_cmd);
  bool res = false;
  if((res = get_game_data(src_data, tgt_data, true))){
    printf("Game data updated successfully.\n");
  }else{
    printf("Problem updating game data.\n");
  }
  free(src_data);
  free(tgt_data);
  return res;
}

void print_help()
{
  printf("ltr_extractor --extract | --create | --update [--spec file] file1 [file2 ...]\n");
}

int main(int argc, char *argv[])
{
  bool extract = false;
  bool update = false;
  bool create = false;
  bool custom_spec = false;
  char *spec_path = NULL;
  int c;
  int index;
  static struct option long_opts[] = {
                   {"spec",        required_argument, NULL, 's'},
                   {"create-spec", no_argument,       NULL, 'c'},
                   {"extract",     no_argument,       NULL, 'e'},
                   {"update",      no_argument,       NULL, 'u'},
                   {"help",        no_argument,       NULL, 'h'},
                   {0,             0,                 0,    0}

  };

  ltr_int_check_root();
  while(1){
    c = getopt_long(argc, argv, "s:ceuh", long_opts, &index);
    if(c < 0){
      break;
    }
    printf("Have %c (%d)\n", c, c);
    switch(c){
      case 'c': create = true;
        break;
      case 'e': extract = true;
        break;
      case 'u': update = true;
        break;
      case 's':
        if(optarg != NULL){
          custom_spec = true;
          spec_path = optarg;
        }
        break;
      case 'h':
        print_help();
        break;
      default:
        break;
    }
  }

  if(extract){
    char *spec;
    if(custom_spec){
      spec = ltr_int_my_strdup(spec_path);
    }else{
      spec = ltr_int_get_data_path("spec.txt");
    }
    read_spec(spec);
    free(spec);
    spec = NULL;
    print_spec_list();
    int i = 2;
    while(i < argc){
      open_file_to_search(argv[i++]);
    }
    check_missed();
    free_specs();
  }else if(update){
    update_gamedata();
  }else if(create){
    FILE *f= NULL;
    if(custom_spec){
      f = fopen(spec_path, "w");
      if(f == NULL){
        printf("Can't open file '%s'.\n", spec_path);
      }
    }else{
      f = stdout;
    }
    while(optind < argc){
      process_file(f, argv[optind++]);
    }
    if(custom_spec){
      fclose(f);
    }
  }
  return 0;
}

/*
TODO:
Add directory/link to tir_firmware creation.
Create man page and add integrated help (with examples).
*/

