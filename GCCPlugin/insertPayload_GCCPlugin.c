/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
	For compiling this plugin: 
		gcc -shared -I$($(shell $(TARGET_GCC) -print-file-name=plugin))/include -fPIC -fno-rtti -O2 insertPayload_GCCPlugin.c -o InsertPayload_GCCPlugin.so

	For including it on your code:
		gcc -o <program> <program.c> -fplugin=./InsertPayload_GCCPlugin.so

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


#include <stdio.h>
#include <stdlib.h>       
#include <string.h>       

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gcc-plugin.h"

/* Includes for use of cpp_define */
#include <c-family/c-common.h>
#include <c-family/c-pragma.h>

/* Includes for save_decoded_options and save_decoded_options_counts */
#include <opts.h>
#include <toplev.h>

#include <sys/metadata_payloads.h>

int plugin_is_GPL_compatible = 1;

#define MAX_ARGUMENTS 4     // Max amount of arguments to be passed to the plugin

#define MAX_PAYLOAD_BINARY_LENGTH_LINE 2048   // Max size of the Payload_Binary.binary_data char array field
#define MAX_BINARY_PAYLOAD_VALUE_LEN MAX_PAYLOAD_BINARY_LENGTH_LINE + 56  // Max count of characters in Payload_Binary value
#define MAX_TOTAL_BINARY_PAYLOADS_VALUES MAX_PAYLOAD_BINARY_LENGTH_LINE + (MAX_ARGUMENTS * MAX_BINARY_PAYLOAD_VALUE_LEN)
                                              // Max count of characters of the whole Payload_Binary line with multiple payloads

#define METADATA_HDR_LENGTH 256               // Max count of char of Metadata_Hdr line
#define PAYLOAD_HDR_INTRO_LEN 128             // Max count of the Payload_Hdr line
#define PAYLOAD_HDR_VALUE_LEN 48              // Max count of each argument Payload_Hdr value
#define PAYLOAD_INTRO_LEN 128                 // Max count of the Payload_Binary definition line

#define FINAL_BUFFER_MAX (((MAX_ARGUMENTS+1)*MAX_BINARY_PAYLOAD_VALUE_LEN*(PAYLOAD_HDR_VALUE_LEN))+PAYLOAD_HDR_INTRO_LEN+METADATA_HDR_LENGTH)*sizeof(char)
                                              // Max count of all metadata lines in user's source code to be replaced

int count_Payload_Binary = 0;

///////////////////// STRUCTS /////////////////

struct payload_lines{
  char line[MAX_BINARY_PAYLOAD_VALUE_LEN]; 
  size_t size_payload;
};

struct payloadhdr_lines{
  char string_function_num[sizeof(int)+1];
};


/* Private declarations */
static void find_text_in_decoded_options(const char *argument, const char * text_to_find, int *variable_to_store);
static void define_payload_as_macro(char *buffer,char const *macro_name, char const *payload_def);
char** extractPayloadArgs(int , char*, const char* );
void print_plugin_info(struct plugin_name_args   *, struct plugin_gcc_version *);
void print_args_info(struct plugin_name_args   *, struct plugin_gcc_version *);
void payload_Binary_fill(int args_count, uint8_t* binary_data, char* filled, long file_size);
void replacePAYLOAD(char* );
void addPayload_line(char* , struct payload_lines* );
void create_addPayloadHdr_line(char* ,  struct payloadhdr_lines* );
void fill_payloads_line(char* , int , struct payload_lines []);
void fill_payloadHeaders_line(char* , int , struct payloadhdr_lines []);
void fill_payloadMetadataHdr_line(char* , char* );
void merge_lines(char*, char*, char*, char*);


/** 
  * @brief Plugin callback called during attribute registration 
  */
static void 
register_payload (void *event_data, void *data) {

  struct plugin_name_args   *plugin_info_regmet = (struct plugin_name_args *) data; // Cast to struct plugin_name_args *
  
  struct payload_lines pay_lines_array[plugin_info_regmet->argc] = {0}; // Array to contain each Payload line
  struct payloadhdr_lines payhdr_lines_array[plugin_info_regmet->argc] = {0}; // Array to contain each Payload_Hdr line

  char argc_string[sizeof(int)+1] = {0};  // Array to store argc (# of arguments passed to the plugin)
  for(int r = 0; r < sizeof(int)+1; r++){ // Clear the array
    argc_string[r] = '\0';
  }

  printf("\n");

  sprintf(argc_string, "%d", plugin_info_regmet->argc);    // Argc to string
  
  //printf("\n COUNT OF ARGUMENTS TO INSERT plugin_info_regmet->argc: %d\n", plugin_info_regmet->argc);

  if(plugin_info_regmet->argc > MAX_ARGUMENTS){ // If # arguments is greater than MAX_ARGUMENTS, then ERROR
    printf("ERROR - La cantidad de argumentos pasado como archivo (%d) es mayor al valor permitido: %d\n", 
      plugin_info_regmet->argc, MAX_ARGUMENTS);
    exit(EXIT_FAILURE);
  }

  for(int i = 0; i < plugin_info_regmet->argc; i++){
    //printf("\n Arg [%d]: register_metadata // Key: [%s] - Value: [%s] \n", i, plugin_info_regmet->argv[i].key, plugin_info_regmet->argv[i].value);

    // 1) Open the file mentioned in the plugin number i argument AS BINARY.
    int fdescriptor_input;
    FILE *rawbin_fp;
    uint8_t* buffer;
    struct stat stbuf;
    size_t bytes_read;

    // Open file and obtain the file descriptor
    fdescriptor_input = open(plugin_info_regmet->argv[i].value, O_RDONLY);
    if (fdescriptor_input == -1) {
        char strerrorbuf[128];
        if (strerror_r(errno, strerrorbuf, sizeof (strerrorbuf)) != 0){
            strncpy(strerrorbuf, "Error file descriptor de .bin", sizeof (strerrorbuf));
            strerrorbuf[sizeof (strerrorbuf) - 1] = '\0';
            perror(strerrorbuf);
        }
        exit(EXIT_FAILURE);

    }

    // Open file as a fstream using the file descriptor
    rawbin_fp = fdopen(fdescriptor_input, "rb");
    if (rawbin_fp == NULL) {
        char strerror_fp_buf[128];
        if (strerror_r(errno, strerror_fp_buf, sizeof (strerror_fp_buf)) != 0){
            strncpy(strerror_fp_buf, "Error apertura file .bin", sizeof (strerror_fp_buf));
            strerror_fp_buf[sizeof (strerror_fp_buf) - 1] = '\0';
            perror(strerror_fp_buf);
        }       
    }

    // Obtain the stat structure of the open file using the file descriptor
    if ((fstat(fdescriptor_input, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        char strerror_stat_buf[128];
        if (strerror_r(errno, strerror_stat_buf, sizeof (strerror_stat_buf)) != 0){
            strncpy(strerror_stat_buf, "Error stat del file .bin", sizeof (strerror_stat_buf));
            strerror_stat_buf[sizeof (strerror_stat_buf) - 1] = '\0';
            perror(strerror_stat_buf);
        }            
    }

    //// 2) CALCULAR EL TAMANIO DEL ARCHIVO 
    off_t file_size;
    file_size = stbuf.st_size;

    //printf("stbuf.st_size - file_size: %ld\n", stbuf.st_size);

    //// 3) Allocate space for the data
    buffer = (uint8_t*) xmalloc(sizeof(char) * file_size);
    if (buffer == NULL){
        perror("Error al allocar memoria para buffer al leer archivo\n");
        exit(EXIT_FAILURE);
    }

    // Check file size is eq or less than MAX_PAYLOAD_BINARY_LENGTH_LINE
    if(file_size > MAX_PAYLOAD_BINARY_LENGTH_LINE){
        perror("Error al abrir archivo para leer - size mayor que MAX_PAYLOAD_BINARY_LENGTH_LINE - rawbin_fp\n");
        exit(EXIT_FAILURE);        
    }

    uint8_t payload[MAX_PAYLOAD_BINARY_LENGTH_LINE];

    //// 4) Read input file as binary and save data into the array
    bytes_read = fread(payload, sizeof(uint8_t), (size_t) file_size, rawbin_fp);

    fclose(rawbin_fp);
    close(fdescriptor_input);

    if (bytes_read != file_size){
        perror("Error al leer bytes del archivo input\n");
        exit(EXIT_FAILURE);
    }

    // IMPRIMIR BUFFER POR PANTALLA - sizeof(uint8_t) - ASCII
    //printf("\n\tDatos leidos en ASCII - sizeof(uint8_t): \n\t");
    //fwrite(payload, sizeof(uint8_t), (size_t) file_size, stdout);
    //printf("\n");    

    //// 5) Iterate over array containing lines extracted

    uint8_t* payload_body;
    payload_body = (uint8_t*)xmalloc(MAX_BINARY_PAYLOAD_VALUE_LEN*sizeof(char));
    if (payload_body == NULL){
      perror("error malloc payload_body\n");
      exit(EXIT_FAILURE);
    }

    memset(payload_body, 0, MAX_BINARY_PAYLOAD_VALUE_LEN*sizeof(char));

    // Copiar leido raw binary data leido en payload_body
    memcpy(payload_body, payload, (size_t) file_size);

    // ALLOCATE MEMORY FOR THE CHAR LINE OF THE PAYLOAD STRUCT
    char *payload_line;
    payload_line = (char*)xmalloc(MAX_BINARY_PAYLOAD_VALUE_LEN*sizeof(char));
    if (payload_line == NULL){
      perror("error malloc payload_line\n");
      exit(EXIT_FAILURE);
    } 

    memset(payload_line, 0, MAX_BINARY_PAYLOAD_VALUE_LEN*sizeof(char));

    payload_Binary_fill(1, payload_body, payload_line, file_size);
    addPayload_line(payload_line, &pay_lines_array[i]);
    create_addPayloadHdr_line(plugin_info_regmet->argv[i].key, &payhdr_lines_array[i]);
    
    //printf("\n\t1. payload_line: %s\n", payload_line);

  }

  // ALLOCATE MEMORY FOR ALL BINARY PAYLOAD  LINES
  size_t payloads_line_complete_size = ((plugin_info_regmet->argc)+1)*MAX_BINARY_PAYLOAD_VALUE_LEN*sizeof(char); 

  char *payloads_line_complete;
  payloads_line_complete = (char*)xmalloc(payloads_line_complete_size);
  if (payloads_line_complete == NULL){
    perror("error malloc payloads_line_complete\n");
    exit(EXIT_FAILURE);
  } 

  memset(payloads_line_complete, 0, payloads_line_complete_size);
  
  fill_payloads_line(payloads_line_complete, plugin_info_regmet->argc, pay_lines_array);
    
  // ALLOCATE MEMORY FOR ALL BINARY PAYLOAD HEADER LINES
  size_t payloadsHeader_line_complete_size = ((plugin_info_regmet->argc*PAYLOAD_HDR_VALUE_LEN)+PAYLOAD_HDR_INTRO_LEN)*sizeof(char); 

  char *payloadsHeader_line_complete;
  payloadsHeader_line_complete = (char*)xmalloc(payloadsHeader_line_complete_size);
  if (payloadsHeader_line_complete == NULL){
    perror("error malloc payloadsHeader_line_complete\n");
    exit(EXIT_FAILURE);
  } 

  memset(payloadsHeader_line_complete, 0, payloadsHeader_line_complete_size);
  
  fill_payloadHeaders_line(payloadsHeader_line_complete, plugin_info_regmet->argc, payhdr_lines_array);

  // ALLOCATE MEMORY FOR THE METADATA HEADER LINE
  size_t payloadsMetadataHdr_line_complete_size = METADATA_HDR_LENGTH*sizeof(char); 

  char *payloadsMetadataHdr_line_complete;
  payloadsMetadataHdr_line_complete = (char*)xmalloc(payloadsMetadataHdr_line_complete_size);
  if (payloadsMetadataHdr_line_complete == NULL){
    perror("error malloc payloadsMetadataHdr_line_complete\n");
    exit(EXIT_FAILURE);
  } 

  memset(payloadsMetadataHdr_line_complete, 0, payloadsMetadataHdr_line_complete_size);

  fill_payloadMetadataHdr_line(payloadsMetadataHdr_line_complete, argc_string);

  // MERGE ALL LINES
  size_t macro_complete_size = payloads_line_complete_size + payloadsHeader_line_complete_size + payloadsMetadataHdr_line_complete_size + 1;

  //printf("macro_complete_size: %zu\n", macro_complete_size);

  char *macro_complete;
  macro_complete = (char*)xmalloc(macro_complete_size);
  if (macro_complete == NULL){
    perror("error malloc macro_complete\n");
    exit(EXIT_FAILURE);
  } 

  memset(macro_complete, 0, macro_complete_size);

  merge_lines(macro_complete, payloads_line_complete, payloadsHeader_line_complete, payloadsMetadataHdr_line_complete);

  /// REPLACE PAYLOAD INTO MACRO

  replacePAYLOAD(macro_complete);
  
}

void merge_lines(char* macro_complete, char* payloads_line_complete, char* payloadsHeader_line_complete, char* payloadsMetadataHdr_line_complete){

  strcpy(macro_complete, payloadsMetadataHdr_line_complete); 
  strcat(macro_complete, payloads_line_complete);  
  strcat(macro_complete, payloadsHeader_line_complete);

  //printf("\n\nFINAL macro_complete: %s\n\n", macro_complete);  

}

void fill_payloadMetadataHdr_line(char* payloadsMetadataHdr_line_complete, char* argc_string){

  strcpy(payloadsMetadataHdr_line_complete, "static Metadata_Hdr metadata_header __attribute__((__used__, __section__(\".metadata\"))) = {");
  strcat(payloadsMetadataHdr_line_complete, argc_string);
  strcat(payloadsMetadataHdr_line_complete, ", sizeof(Payload_Hdr)};");

  //printf("\nFINAL payloadsMetadataHdr_line_complete: %s\n", payloadsMetadataHdr_line_complete);  

}



void fill_payloadHeaders_line(char* payloadsHeader_line_complete, int argc, struct payloadhdr_lines payhdr_lines_array[]){

  strcpy(payloadsHeader_line_complete, "static Payload_Hdr payload_headers[] __attribute__((__used__, __section__(\".metadata\"))) = {");

  char index_string[sizeof(int)+1] = {0};

  for(int r = 0; r < sizeof(int)+1; r++){
    index_string[r] = '\0';
  }

  for(int m = 0; m < argc; m++){
    strcat(payloadsHeader_line_complete, "{");
    strcat(payloadsHeader_line_complete, payhdr_lines_array[m].string_function_num);
    strcat(payloadsHeader_line_complete, ", sizeof(payloads[");

    sprintf(index_string, "%d", m);    
    strcat(payloadsHeader_line_complete, index_string);
    strcat(payloadsHeader_line_complete, "])}");
  
    if(m < argc - 1){
      strcat(payloadsHeader_line_complete, ",");
    }

    memset(index_string, 0, sizeof(index_string));

  }

  strcat(payloadsHeader_line_complete, "};");
  
  //printf("\nFINAL payloadsHeader_line_complete: %s\n", payloadsHeader_line_complete);  

}

void fill_payloads_line(char* payloads_line_complete, int argc, struct payload_lines pay_lines_array[]){

  strcpy(payloads_line_complete, "static Payload_Binary payloads[] __attribute__((__used__, __section__(\".metadata\"), __aligned__(8))) = {");

  for(int k = 0; k < argc; k++){
    strcat(payloads_line_complete, pay_lines_array[k].line);
    if(k < argc - 1){
      strcat(payloads_line_complete, ",");
    }
  }

  strcat(payloads_line_complete, "};");
  
  //printf("\nFINAL payloads_line_complete: %s\n", payloads_line_complete);

}

void addPayload_line(char* line, struct payload_lines* payload_line_struct){

  size_t line_length = strlen(line);

  if(line_length > MAX_BINARY_PAYLOAD_VALUE_LEN){
      perror("error line_length greater than addPayload_line()\n");
      exit(EXIT_FAILURE);
  }

  void* ret_memcpy = NULL;
  ret_memcpy = memcpy(payload_line_struct->line, line, line_length);
  if(ret_memcpy == NULL){
      perror("error memcpy addPayload_line()\n");
      exit(EXIT_FAILURE);
  }

  //printf("payload_line_struct->line: \n");
  //for(int j = 0; j < MAX_BINARY_PAYLOAD_VALUE_LEN; j++){
  //  printf("%c", payload_line_struct->line[j]);
  //}
  //printf("\n");

}

void create_addPayloadHdr_line(char* funct_number, struct payloadhdr_lines* payloadHdr_line_struct){

  size_t line_length_funct = strlen(funct_number);

  if(line_length_funct > sizeof(int)){
      perror("error line_length_funct greater than create_addPayloadHdr_line()\n");
      exit(EXIT_FAILURE);
  }

  void* ret_memcpy = NULL;
  ret_memcpy = memcpy(payloadHdr_line_struct->string_function_num, funct_number, line_length_funct);
  if(ret_memcpy == NULL){
      perror("error memcpy create_addPayloadHdr_line()\n");
      exit(EXIT_FAILURE);
  }

  //printf("payloadHdr_line_struct->string_function_num: \n");
  //for(int j = 0; j < sizeof(int)+1; j++){
  //  printf("%c", payloadHdr_line_struct->string_function_num[j]);
  //}
  //printf("\n");
}



void replacePAYLOAD(char* macro_complete){
  // 
  // Checks to see if user has previously defined __SEED_UINT__ and __SEED_ULINT__ macros
  // with -D argument. If they have been defined we will keep its value, otherwise
  // we generate a seed based on current time, we assign unsigned int and unsigned long int
  // presition for the corresponding macro 
  //
  int payload_found = 0;
  unsigned int i=1;

  // Buffer for holding the string defined as a macro //
  char macro_buff[FINAL_BUFFER_MAX];

  //printf("\nstrlen(macro_complete): %ld\n", strlen(macro_complete));

  if(strlen(macro_complete) > FINAL_BUFFER_MAX){
    perror("error macro_complete is larger than FINAL_BUFFER_MAX - replacePAYLOAD()\n");
    exit(EXIT_FAILURE);
  }  

  if(!payload_found){
    //printf("\npayload_found\n");
    define_payload_as_macro(macro_buff, "__PAYLOAD__", macro_complete);
  }

}

void payload_Binary_fill(int args_count, uint8_t* binary_data, char* filled, long file_size){

    //printf("\n\t*** payload_Binary_fill() called**\n");
    //printf("\t    // payload_Binary_fill() // count_args: %d\n", args_count);
    //printf("\t");
    //for(int j = 0; j < file_size; j++){    
    //    printf(" %02X", binary_data[j]);
    //}

    char *payloads_type_Binary;
    payloads_type_Binary = (char*)xmalloc(500*sizeof(char));
    if (payloads_type_Binary == NULL){
      perror("error malloc payloads_type_Binary\n");
      exit(EXIT_FAILURE);
    }

    // 1) Save binary data into an uint8_t array
    uint8_t payload[BINPAYLOAD_MAXSIZE+1] = {0};
    
    memcpy(payload, binary_data, file_size);

    char canonical_hex_byte[3] = {0};
    canonical_hex_byte[2]='\0';

    char container[2] = {0};
    container[1]='\0';

    char file_size_string[sizeof(size_t)+1] = {0};

    for(int r = 0; r < sizeof(size_t)+1; r++){
      file_size_string[r] = '\0';
    }

    char char_count_string[sizeof(long)+1] = {0};

    for(int p = 0; p < sizeof(long)+1; p++){
      char_count_string[p] = '\0';
    }

    // 2) Create string with data
    strcpy(payloads_type_Binary, "{");

    sprintf(file_size_string, "%ld", file_size);    // First field: long bin_data_size

    strcat(payloads_type_Binary, file_size_string);
    strcat(payloads_type_Binary, ",");

    size_t char_count_prev = (file_size)*(2);
    //printf("\n\tchar_count_prev: %zu\n", char_count_prev);

    if(char_count_prev > MAX_PAYLOAD_BINARY_LENGTH_LINE){
      printf("Bytes como string %zu es mas grande que BINPAYLOAD_MAXSIZE - payload_Binary_fill\n", char_count_prev);
      exit(EXIT_FAILURE);
    }

    sprintf(char_count_string, "%zu", char_count_prev);    // Second field: size_t charArray_bin_size;
    strcat(payloads_type_Binary, char_count_string);

    strcat(payloads_type_Binary, ",");
    
    strcat(payloads_type_Binary, "\"");  // because char binary_data[] in Payload_Binary struct is a char array

    size_t char_count = 0;

    int i = 0;

    for(i = 0; i < file_size; i++){
        sprintf(canonical_hex_byte, "%02X", payload[i]);
        strcat(payloads_type_Binary, canonical_hex_byte);

        char_count = char_count + 2;
    }

    strcat(payloads_type_Binary, "\""); 
    strcat(payloads_type_Binary, "}");
    
    //printf("\n\t%s\n", payloads_type_Binary);

    strcpy(filled, payloads_type_Binary);

    // ******** INCREASE PAYLOAD_BINARY GLOBAL COUNT
    count_Payload_Binary = count_Payload_Binary + 1;

    return;

}

/** 
  * @brief Parses buffer with macro_name and defines a macro with such value	 
  */
static void
define_payload_as_macro(char *buffer, //!< Pointer to buffer where the parsed macro will be stored
  char const *macro_name,//!< name of the macro to be defined
  char const *payload_def) 
{
  sprintf(buffer, "%s=%s", macro_name, payload_def);
  
  printf("To be replaced: \n%s\n\n", buffer);
  
  cpp_define(parse_in, buffer);
}

char** extractPayloadArgs(int payload_args_count, char* arguments_line, const char* token){
    int p = 0;
    char** args_array = new char*[50];
    for(int i = 0; i < payload_args_count; i++) {
         args_array[i] = new char;
    }
    while((arguments_line != NULL) && (p < payload_args_count) ){
        arguments_line = strtok(NULL,token);
        strcpy(args_array[p], arguments_line);
        //printf(" // extractPayloadArgs() // char payload_arg[%d]: %s\n", p, args_array[p]);
        p++;
    }
    return args_array;
}

/** 
  * @brief Plugin initialization method 
  */
int plugin_init(struct plugin_name_args   *info,  //!< Holds plugin arg
                    struct plugin_gcc_version *ver)   //!< Version info of GCC
{		
  print_args_info(info, ver);  // Print arguments passed to the plugin

	register_callback ("InsertPayload_GCCPlugin", PLUGIN_START_UNIT, register_payload, info);
	 
 	return 0;
}

void print_args_info(struct plugin_name_args  *plugin_info, 
                        struct plugin_gcc_version *version){

    printf("Number of arguments of this plugin: %d\n", plugin_info->argc); 

    for (int i = 0; i < plugin_info->argc; i++){
        printf("Arg [%d]: - Key: [%s] - Value: [%s]\n", i, plugin_info->argv[i].key, plugin_info->argv[i].value);
    }

}


