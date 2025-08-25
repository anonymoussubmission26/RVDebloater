#include <stdint.h>
#include <stdio.h>
#include <uthash.h> // install sudo apt install uthash-dev
#include <stdatomic.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>


#define true 1
#define false 0
#define MAX_MODE_NAME 50
#define MAX_FUNC_NAME 50000
#define MAX_ADDR_STR_LEN 20  // To store addresses like "0x5e8b40"
#define STACK_SIZE 128  // Can be adjusted based on system constraints
#define MAX_PIDS     10    // Max unique PID indices (from hash map)
#define MAX_MODES 16
#define MAX_Funcs 50000
#define MAX_MODE_NAME_LEN 32

/* Used for Profiling */
typedef char bool;

typedef struct {
    pid_t pid;
    atomic_bool in_use;
} pid_slot_t;
static pid_slot_t pid_table[MAX_PIDS];
// const char* mode_names[MAX_MODES] = {"initialized","takeoff", "land", "manual", "acro", "offboard", "stabilized", 
//     "altctl", "posctl", "position:slow", "auto:mission", "auto:loiter",
//     "auto:rtl","auto:takeoff","auto:land","auto:precland"};
// Step 1: Define enum for mode IDs
typedef enum {
    MODE_INITIALIZED = 0,
    MODE_TAKEOFF,
    MODE_LAND,
    MODE_MANUAL,
    MODE_ACRO,
    MODE_OFFBOARD,
    MODE_STABILIZED,
    MODE_ALTCTL,
    MODE_POSCTL,
    MODE_POSITION_SLOW,
    MODE_AUTO_MISSION,
    MODE_AUTO_LOITER,
    MODE_AUTO_RTL,
    MODE_AUTO_TAKEOFF,
    MODE_AUTO_LAND,
    MODE_AUTO_PRECLAND,
    MODE_UNKNOWN = -1
} ModeID;
// Step 2: Define mode names array (same order as enum)
const char* mode_names[] = {
    "initialized",
    "takeoff",
    "land",
    "manual",
    "acro",
    "offboard",
    "stabilized",
    "altctl",
    "posctl",
    "position:slow",
    "auto:mission",
    "auto:loiter",
    "auto:rtl",
    "auto:takeoff",
    "auto:land",
    "auto:precland"
};
#define NUM_MODES (sizeof(mode_names)/sizeof(mode_names[0]))
bool mode_to_func_mapping[MAX_PIDS][NUM_MODES][MAX_Funcs] = {};




int initialized = 1;
int initialized2 = 1;
int initialized3 = 1;
volatile int mode_switching = 0;

int counter = 0;
int counter2 = 0;
int counter3 , counter4 = 0;

// char func_names[30000][200];
char* func_names[50000];
void *func_addresses[50000] = {NULL};  // Array to store function addresses

typedef struct {
    char name[MAX_FUNC_NAME];         // Function name (key)
    char addr_str_index[MAX_ADDR_STR_LEN]; // Function address (key)
    UT_hash_handle hh_name;           // Hash table handle for function name lookup
    UT_hash_handle hh_addr;           // Hash table handle for function address lookup
} FuncEntry;

FuncEntry *allowed_functions = NULL;           // Hash table for function name lookup
FuncEntry *allowed_functions_address = NULL;   // Hash table for function address lookup

typedef struct {
    char addr_str[MAX_ADDR_STR_LEN];  // Address as a string
    UT_hash_handle hh;                // Hash table handle
} LoggedAddrEntry;

LoggedAddrEntry *logged_addresses = NULL;  // Hash table to track logged addresses

typedef struct {
    char name[MAX_FUNC_NAME];         // Function name (key)
    UT_hash_handle hh;                // Hash table handle
} LoggedNameEntry;

LoggedNameEntry *logged_name = NULL;  // Hash table to track logged addresses


typedef struct ReturnEntry {
    void *addr;       // Key: Function address
    void *next_addr;  // Value: Expected return address
    UT_hash_handle hh;
} ReturnEntry;

ReturnEntry *return_table = NULL;  // Hash table

uint32_t shadow_stack[STACK_SIZE]; // Simulated shadow stack
int stack_top = -1; // Stack pointer


int curr_mode_id = 0;

char curr_mode_buf[MAX_MODE_NAME_LEN] = "initialized";
char new_mode_buf[MAX_MODE_NAME_LEN] = "initialized";
char *curr_mode = curr_mode_buf;
char *new_mode = new_mode_buf;


// Step 3: Lookup function
ModeID get_mode_id(const char *mode_name) {
    for (int i = 0; i < NUM_MODES; ++i) {
        if (strcmp(mode_name, mode_names[i]) == 0) {
            return (ModeID)i;
        }
    }
    return MODE_UNKNOWN;
}

void __attribute__((noinline)) __attribute__((used)) mode_entry(int argc, char **argv) {
// void __attribute__((noinline)) __attribute__((used)) mode_entry(uint8_t base_mode) {

    // printf("Inside mode_entry\n");
    // printf("argc = %d\n", argc);
    // for (int i = 0; i < argc; ++i) {
    //     if (argv[i]) {
    //         printf("argv[%d] = %s\n", i, argv[i]);
    //     } else {
    //         printf("argv[%d] = (null)\n", i);
    //     }
    // }
    // fflush(stdout);

    if (curr_mode && argc == 1 && !strcmp(argv[0], curr_mode)){
        printf("New mode is equal to current mode!\n");
        return;
    }else if (curr_mode && argc == 2 && !strcmp(argv[1], curr_mode)){
        printf("New mode is equal to current mode!\n");
        return;
    }else if (argc == 2) {
        printf("Argc==2 From %s to %s\n", curr_mode ? curr_mode : "NULL", argv[1]);
        strncpy(new_mode_buf, argv[1], MAX_MODE_NAME_LEN - 1);
        new_mode_buf[MAX_MODE_NAME_LEN - 1] = '\0';
    } else {
        printf("Argc==1 From %s to %s\n", curr_mode ? curr_mode : "NULL", argv[0]);
        strncpy(new_mode_buf, argv[0], MAX_MODE_NAME_LEN - 1);
        new_mode_buf[MAX_MODE_NAME_LEN - 1] = '\0';
    }

    // // Get the home directory path
    const char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        perror("Error getting home directory");
        return;
    }

    // // Construct the path to the "profiles" directory
    char profiles_dir[512];
    snprintf(profiles_dir, sizeof(profiles_dir), "%s/rvd-project/RVDefender/profiles", home_dir);

    // Check if the "profiles" directory exists
    struct stat statbuf;
    if (stat(profiles_dir, &statbuf) != 0 || !S_ISDIR(statbuf.st_mode)) {
        // Directory does not exist, so create it
        if (mkdir(profiles_dir, 0755) != 0) {
            perror("Error creating profiles directory");
            return;
        }
        printf("Created profiles directory: %s\n", profiles_dir);
    }

    // Construct the full file path inside the "profiles" directory
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/mode_profile", profiles_dir);

    // Open the file for appending
    FILE* file = fopen(filepath, "a");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    curr_mode_id = get_mode_id(curr_mode);
    fprintf(file, "Mode: %d\n", curr_mode_id);
    // for(int i = 0; i< MAX_PIDS; i++){
        for (int j = 0; j < MAX_Funcs; j++) {
            if (mode_to_func_mapping[0][curr_mode_id][j]) {
                fprintf(file, "%s: %s @ %p\n", curr_mode, func_names[j], func_addresses[j]);
            }
        }
    // }
    fprintf(file, "----------------------\n");
    fclose(file);
    printf("Writing the information for %s mode is done\n", curr_mode);  

    strncpy(curr_mode_buf, new_mode_buf, MAX_MODE_NAME_LEN - 1);
    curr_mode_buf[MAX_MODE_NAME_LEN - 1] = '\0';
    curr_mode_id = get_mode_id(curr_mode);

    printf("Going to new mode: %s (id: %d), %s\n", curr_mode, curr_mode_id, new_mode);    
    return;
}


void __attribute__((noinline)) mode_entry_runtime(int argc, char **argv) {
    // printf("Previous mode is %d and %s\n", curr_mode_id, mode_to_string(curr_mode_id)); 

    if (curr_mode && argc == 1 && !strcmp(argv[0], curr_mode)){
        printf("New mode is equal to current mode!\n");
        return;
    }else if (curr_mode && argc == 2 && !strcmp(argv[1], curr_mode)){
        printf("New mode is equal to current mode!\n");
        return;
    }else if (argc == 2) {
        printf("Argc==2 From %s to %s\n", curr_mode ? curr_mode : "NULL", argv[1]);
        strncpy(new_mode_buf, argv[1], MAX_MODE_NAME_LEN - 1);
        new_mode_buf[MAX_MODE_NAME_LEN - 1] = '\0';
    } else {
        printf("Argc==1 From %s to %s\n", curr_mode ? curr_mode : "NULL", argv[0]);
        strncpy(new_mode_buf, argv[0], MAX_MODE_NAME_LEN - 1);
        new_mode_buf[MAX_MODE_NAME_LEN - 1] = '\0';
    }

    mode_switching = 1;

    // printf("Removing Hash Tables.\n");
    if(allowed_functions){
        //// Clear previous functions from name-based hash table
        FuncEntry *current, *tmp;
        HASH_ITER(hh_name, allowed_functions, current, tmp) {  // Correct hash handle
            HASH_DELETE(hh_name, allowed_functions, current);  // Use correct hash handle
            HASH_DELETE(hh_addr, allowed_functions_address, current);

            // printf("Freeing function: %s\n", current->name);
            free(current);
        }
        allowed_functions = NULL; // Reset pointer after deletion
        allowed_functions_address = NULL;
    }
    // if(allowed_functions_address){
    //     // Clear previous functions from address-based hash table
    //     FuncEntry *current2, *tmp2;
    //     HASH_ITER(hh_addr, allowed_functions_address, current2, tmp2) {  // Correct hash handle
    //         HASH_DELETE(hh_addr, allowed_functions_address, current2);  // Use correct hash handle
    //         printf("Freeing function2: %s\n", current2->name);

    //         free(current2);
    //     }
    //     allowed_functions_address = NULL; // Reset pointer after deletion
    // }

    printf("Previous mode is %s and new mode is %s\n", curr_mode, new_mode_buf);  

    // Construct filename
    char filename[MAX_MODE_NAME + 100];
    snprintf(filename, sizeof(filename), "/home/mohsen/rvd-project/RVDefender/profiles/%s_functions.txt", new_mode_buf);
    // FILE *log_file = fopen("/home/mohsen/debug_log.txt", "a");

    // Open file
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening function list file");
        return;
    }


    // Read function names and addresses line by line
    char line[10000];  // Increase buffer size to prevent truncation
    // Read function names line by line
    char func_name[MAX_FUNC_NAME];
    // unsigned long addr;
    char addr_str[MAX_ADDR_STR_LEN]; // To store the address with "0x" prefix

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline

        // Trim leading spaces
        char *ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        // Parse function name and address
        // if (sscanf(ptr, "%499s %lx", func_name, &addr) != 2) {
        if (sscanf(ptr, "%499s %19s", func_name, addr_str) != 2) {
            fprintf(stderr, "Error parsing function: '%s'\n", ptr);
            continue;
        }

        // Allocate and initialize entry
        FuncEntry *entry = malloc(sizeof(FuncEntry));
        if (!entry) {
            perror("Memory allocation error");
            fclose(file);
            return;
        }
        memset(entry, 0, sizeof(FuncEntry));

        strncpy(entry->name, func_name, MAX_FUNC_NAME - 1);
        strncpy(entry->addr_str_index, addr_str, sizeof(entry->addr_str_index) - 1);

        // Add to hash table
        HASH_ADD(hh_name, allowed_functions, name, strlen(entry->name), entry);
        HASH_ADD(hh_addr, allowed_functions_address, addr_str_index, strlen(entry->addr_str_index), entry);


        // printf("Added function: %s at %s\n", entry->name, entry->addr_str);
    }

    fclose(file);
    // fclose(log_file);
    // printf("Loaded %u functions for mode %s\n", HASH_CNT(hh_name,allowed_functions), mode_to_string(mode_enum));
    // printf("Loaded %u functions for mode %s\n", HASH_CNT(hh_addr,allowed_functions_address), mode_to_string(mode_enum));

    mode_switching = 0;
    initialized2 = 1;
    counter = 0;
    initialized3 = 1;
    counter2 = 0;
    return;
}

// int get_or_insert_pid_index(pid_t pid) {
//     for (int i = 0; i < MAX_PIDS; ++i) {
//         if (atomic_load(&pid_table[i].in_use) && pid_table[i].pid == pid)
//             return i;

//         // Found unused slot, try to claim it
//         bool expected = false;
//         if (atomic_compare_exchange_strong(&pid_table[i].in_use, &expected, true)) {
//             pid_table[i].pid = pid;
//             return i;
//         }
//     }
//     return -1; // Table full
// }

void __attribute__((noinline)) log_fn(int id, char * str, int size, void *addr) {
    // pid_t pid = getpid();
    // int index = (pid * 2654435761u) % MAX_PIDS;

    // if(!curr_mode){
    //     printf("Mode is NULL\n");
    //     return;
    // }

    if (id < 0 || id >= MAX_Funcs) {
        fprintf(stderr, "Invalid function ID: %d\n", id);
        return;
    }

    // ModeID curr_mode_id = get_mode_id(curr_mode);
    // printf("Mode '%s' has ID %d\n", curr_mode, curr_mode_id);

    if (curr_mode_id == -1) {
        printf("Invalid mode: %s\n", curr_mode);
        return;
    }

    // else{
        // printf("The mode id is %d \n", curr_mode_id);
    // }

    // printf("Value of curr_mode_id is %d for %s", curr_mode_id, curr_mode);
    // fflush(stdout);

    // if (index < 0 || index >= MAX_PIDS || mode_to_func_mapping[index][curr_mode_id][id]) return;
    if (mode_to_func_mapping[0][curr_mode_id][id]) return;

    if (mode_to_func_mapping[0][curr_mode_id][id]) return;

    mode_to_func_mapping[0][curr_mode_id][id] = true;
    
    if (func_names[id] == NULL) {
        func_names[id] = str + '\0';  // Store the pointer to the function name   
        func_addresses[id] = addr;
    }

	return;
}

void __attribute__((noinline)) dummy_fn(void *addr){

    if(mode_switching) return;
    
        FuncEntry *entry;
        if (allowed_functions_address != NULL) {
            char addr_str[20];  
            snprintf(addr_str, sizeof(addr_str), "%p", addr); 
            
            HASH_FIND(hh_addr, allowed_functions_address, addr_str, strlen(addr_str), entry);
        } else {
            return;  
        }

        if (entry == NULL && new_mode_buf != "initialized") {  
            LoggedAddrEntry *logged_entry;
            char addr_str[20];
            snprintf(addr_str, sizeof(addr_str), "%p", addr);  
            HASH_FIND(hh, logged_addresses, addr_str, strlen(addr_str), logged_entry);

            if (logged_entry == NULL) {
                FILE *log_file = fopen("/home/mohsen/rvd-project/RVDefender/profiles/access_violation_indirect.log", "a");
                if (log_file != NULL) {
                    fprintf(log_file, "Indirect Call: Not allowed at address %p in mode %s\n", addr, new_mode_buf);
                    fclose(log_file);
                } else {
                    printf("Error opening log file!\n");
                }

                logged_entry = malloc(sizeof(LoggedAddrEntry));
                if (logged_entry) {
                    strncpy(logged_entry->addr_str, addr_str, sizeof(logged_entry->addr_str) - 1);
                    HASH_ADD(hh, logged_addresses, addr_str, strlen(logged_entry->addr_str), logged_entry);
                } else {
                    perror("Memory allocation error");
                }
            }
            
        } else {  
            if (initialized2 == 1) {
                printf("Indirect Call: Allowed at address %p in mode %s\n", addr, new_mode_buf);
                counter++;
                if (counter == 5) {
                    initialized2 = -1;
                }
            }
        }
    return;
    
}

typedef struct {
    void* return_addr;
} InstrumentationData;

void ret_function() {
    InstrumentationData data;
    data.return_addr = __builtin_return_address(0); // Get return address
    
    // printf("Return Address: %p\n", data.return_addr);
}

void __attribute__((noinline)) log_return_address(void *addr) {
// void __attribute__((noinline)) log_return_address(void) {

    if(initialized == 1){
        // printf("Return Address: %p\n", addr);
        counter4++;
        if(counter4 == 5) initialized = -1;
    }
    return;
}
