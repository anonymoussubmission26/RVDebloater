#include <stdint.h>
#include <stdio.h>
#include <uthash.h> // install sudo apt install uthash-dev

#define true 1
#define false 0
#define MAX_MODE_NAME 50
#define MAX_FUNC_NAME 50000
#define MAX_ADDR_STR_LEN 20  // To store addresses like "0x5e8b40"
#define STACK_SIZE 128  // Can be adjusted based on system constraints

/* Used for Profiling */
typedef char bool;
bool mode_to_func_mapping[29][50000] = {{0, 1, 2, 3, 4, 5}};
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

// typedef struct {
//     char name[MAX_FUNC_NAME]; // Function name (key)
//     UT_hash_handle hh_name; // Hash table handle
// } FuncEntry;


// typedef struct {
//     char addr_str_index[MAX_ADDR_STR_LEN]; // Function name (key)
//     UT_hash_handle hh_addr; // Hash table handle
// } AddrEntry;

// FuncEntry *allowed_functions = NULL;  // Hash table for function lookup
// AddrEntry *allowed_address = NULL;  // Hash table for function lookup


/* Used for tracking modes copter*/
enum Number {
    STABILIZE = 0,
    ACRO = 1,
    ALT_HOLD = 2,
    AUTO = 3,
    GUIDED = 4,
    LOITER = 5,
    RTL = 6,
    CIRCLE = 7,
    LAND = 9,
    DRIFT = 11,
    SPORT = 13,
    FLIP = 14,
    AUTOTUNE = 15,
    POSHOLD = 16,
    BRAKE = 17,
    THROW = 18,
    AVOID_ADSB = 19,
    GUIDED_NOGPS = 20,
    SMART_RTL = 21,
    FLOWHOLD = 22,
    FOLLOW = 23,
    ZIGZAG = 24,
    SYSTEMID = 25,
    AUTOROTATE = 26,
    AUTO_RTL = 27,
    TURTLE = 28
};

enum Number curr_mode_id;
enum Number mode_enum = STABILIZE;

const char* mode_to_string(enum Number mode) {
    // printf("Inside mode_to_string, mode: %d\n", (int)mode);
    switch (mode) {
        case STABILIZE: return "STABILIZE";
        case ACRO: return "ACRO";
        case ALT_HOLD: return "ALT_HOLD";
        case AUTO: return "AUTO";
        case GUIDED: return "GUIDED";
        case LOITER: return "LOITER";
        case RTL: return "RTL";
        case CIRCLE: return "CIRCLE";
        case LAND: return "LAND";
        case DRIFT: return "DRIFT";
        case SPORT: return "SPORT";
        case FLIP: return "FLIP";
        case AUTOTUNE: return "AUTOTUNE";
        case POSHOLD: return "POSHOLD";
        case BRAKE: return "BRAKE";
        case THROW: return "THROW";
        case AVOID_ADSB: return "AVOID_ADSB";
        case GUIDED_NOGPS: return "GUIDED_NOGPS";
        case SMART_RTL: return "SMART_RTL";
        case FLOWHOLD: return "FLOWHOLD";
        case FOLLOW: return "FOLLOW";
        case ZIGZAG: return "ZIGZAG";
        case SYSTEMID: return "SYSTEMID";
        case AUTOROTATE: return "AUTOROTATE";
        case AUTO_RTL: return "AUTO_RTL";
        case TURTLE: return "TURTLE";
        default: return "UNKNOWN";
    }
    return "UNKNOWN";
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

void __attribute__((noinline)) mode_entry(uint8_t new_mode) {
    // enum Number mode_enum = (enum Number)new_mode; 
    mode_enum = (enum Number)new_mode; 

    // printf("Previous mode is %d and %s\n", curr_mode_id, mode_to_string(curr_mode_id));  
    if (curr_mode_id == new_mode) return;

    // Get the home directory path
    const char* home_dir = getenv("HOME");
    if (home_dir == NULL) {
        perror("Error getting home directory");
        return;
    }

    // Construct the path to the "profiles" directory
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

    fprintf(file, "Mode: %d (%s)\n", curr_mode_id, mode_to_string(curr_mode_id));
    for (int i = 0; i < 50000; i++) {
        if (mode_to_func_mapping[curr_mode_id][i]) {
            // fprintf(file, "%d: %s\n", curr_mode_id, func_names[i]);
            fprintf(file, "%s: %s @ %p\n", mode_to_string((enum Number)curr_mode_id), func_names[i], func_addresses[i]);
        }
    }
    fprintf(file, "----------------------\n");
    fclose(file);
    printf("Writing the information for %s mode is done\n", mode_to_string(curr_mode_id));  

    curr_mode_id = new_mode;
    printf("Going to new mode: %d , %s\n", curr_mode_id, mode_to_string(mode_enum));    
    return;
}

void __attribute__((noinline)) mode_entry_runtime(uint8_t new_mode) {
    // enum Number mode_enum = (enum Number)new_mode; 

    // printf("Previous mode is %d and %s\n", curr_mode_id, mode_to_string(curr_mode_id));  
    if (curr_mode_id == new_mode) return;

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

    mode_enum = (enum Number)new_mode; 
    printf("Previous mode is %s and new mode is %s\n", mode_to_string(curr_mode_id), mode_to_string(mode_enum));  

    // Construct filename
    char filename[MAX_MODE_NAME + 100];
    snprintf(filename, sizeof(filename), "/home/mohsen/rvd-project/RVDefender/profiles/%s_functions.txt", mode_to_string(mode_enum));
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
    
    curr_mode_id = new_mode;

    mode_switching = 0;
    initialized2 = 1;
    counter = 0;
    initialized3 = 1;
    counter2 = 0;
    return;
}


void __attribute__((noinline)) log_fn(int id, char * str, int size, void *addr) {
	if (initialized == 1){
		for(int j = 0; j < 50000; j++) {			
			for(int i = 0; i < 29; i++) {
				mode_to_func_mapping[i][j] = false;
			}
            // func_names[j][0] = '\0';
            func_names[j] = NULL;
            func_addresses[j] = NULL;
		}
        printf("Initializing is finished! %d \n", initialized);
        printf("First function executed: %s at address %p\n", str, addr);
		initialized = -1;
	}
	if(mode_to_func_mapping[curr_mode_id][id]) {
        // if(curr_mode_id == 4){
        //     printf("This function has been seen in %d! \n", curr_mode_id);
        // }
        return;
    } 
	mode_to_func_mapping[curr_mode_id][id] = true;

    // if (strstr(str, "_ZNK6AP_HAL8RCOutput22get_output_mode_bannerEPch") != NULL) {
    //         printf("Matched function in mode %s: %s\n", mode_to_string(curr_mode_id), str);
    // }

    if (func_names[id] == NULL) {
        // Directly assign the string pointer to func_names
        func_names[id] = str + '\0';  // Store the pointer to the function name   
        func_addresses[id] = addr;
    }

    // if (strlen(func_names[id]) == 0) {
    //     memcpy(func_names[id], str, size);
    //     func_names[id][size] = '\0';  // Ensure the string is null-terminated
    //     func_addresses[id] = addr;
    // }

	return;
}
// __attribute__((noinline)) volatile void dummy_fn(void){
//     return;
// }

void __attribute__((noinline)) dummy_fn(void *addr){

    if(mode_switching) return;
    
    // if (indirect == 1) {  // Indirect Callsite Handling
        FuncEntry *entry;
        if (allowed_functions_address != NULL) {
           // Convert the address to a string (e.g., "0x5e8b40") for comparison
            char addr_str[20];  // Buffer to store the address string
            snprintf(addr_str, sizeof(addr_str), "%p", addr);  // Convert addr to string
            
            HASH_FIND(hh_addr, allowed_functions_address, addr_str, strlen(addr_str), entry);
        } else {
            return;  // No functions loaded yet
        }

        if (entry == NULL && mode_to_string(mode_enum) != "STABILIZE") {  // Address not found → Violation
            // if(initialized == 1){
            // Check if the address has already been logged
            LoggedAddrEntry *logged_entry;
            char addr_str[20];
            snprintf(addr_str, sizeof(addr_str), "%p", addr);  // Convert addr to string
            HASH_FIND(hh, logged_addresses, addr_str, strlen(addr_str), logged_entry);

            if (logged_entry == NULL) {
                // Address not logged yet, log it
                FILE *log_file = fopen("/home/mohsen/rvd-project/RVDefender/profiles/access_violation_indirect.log", "a");
                if (log_file != NULL) {
                    fprintf(log_file, "Indirect Call: Not allowed at address %p in mode %s\n", addr, mode_to_string(mode_enum));
                    fclose(log_file);
                } else {
                    printf("Error opening log file!\n");
                }

                // Add to hash table to track that this address has been logged
                logged_entry = malloc(sizeof(LoggedAddrEntry));
                if (logged_entry) {
                    strncpy(logged_entry->addr_str, addr_str, sizeof(logged_entry->addr_str) - 1);
                    HASH_ADD(hh, logged_addresses, addr_str, strlen(logged_entry->addr_str), logged_entry);
                } else {
                    perror("Memory allocation error");
                }
            }
        } 
    // } 
    // else{
    //     // printf("Direct call\n");
    //     FuncEntry *entry;
    //     if (allowed_functions != NULL){  // Ensure hash table is initialized
    //         // printf("Initialized: %s \n", func_name);
    //         HASH_FIND(hh_name, allowed_functions, func_name, strlen(func_name), entry);
    //     }
    //     else { 
    //         // if(mode_to_string(mode_enum) != "STABILIZE"){
    //         //     printf("Not initialized allowed functions! \n");
    //         // }
    //         return;
    //     }
    //     if (entry == NULL && mode_to_string(mode_enum) != "STABILIZE"){ // Not Allowed
    //         LoggedNameEntry *logged_name_entry;
    //         HASH_FIND(hh, logged_name, func_name, strlen(func_name), logged_name_entry);
    //         if (logged_name_entry == NULL) {  // If not logged, log it
    //             FILE *log_file = fopen("/home/mohsen/rvd-project/RVDefender/profiles/access_violation_direct.log", "a"); // Open log file in append mode
    //             if (log_file != NULL) {
    //                 fprintf(log_file, "Not allowed: %s in mode %s\n", func_name, mode_to_string(mode_enum));
    //                 fclose(log_file);
    //             } else {
    //                 printf("Error opening log file!\n");
    //             }

    //             // Add to hash table to track that this function name has been logged
    //             logged_name_entry = malloc(sizeof(LoggedNameEntry));
    //             if (logged_name_entry) {
    //                 strncpy(logged_name_entry->name, func_name, sizeof(logged_name_entry->name) - 1);
    //                 HASH_ADD(hh, logged_name, name, strlen(logged_name_entry->name), logged_name_entry);
    //             } else {
    //                 perror("Memory allocation error");
    //             }
    //         }
    //     }
    //     else{
    //         if(initialized3 == 1){ // Allowed
    //             printf("Direct Call: Allowed %s in mode %d. \n", func_name, (int)mode_enum);
    //             counter2 ++;
    //             if (counter2 == 5){
    //                 initialized3 = -1;
    //             }
    //         }
    //     }
    // }
    return;
}

typedef struct {
    void* return_addr;
} InstrumentationData;

void ret_function() {
    InstrumentationData data;
    data.return_addr = __builtin_return_address(0); // Get return address
    
    printf("Return Address: %p\n", data.return_addr);
}

void __attribute__((noinline)) log_return_address(void *addr) {
// void __attribute__((noinline)) log_return_address(void) {

    if(initialized == 1){
        printf("Return Address: %p\n", addr);
        counter4++;
        if(counter4 == 5) initialized = -1;
    }
    return;
}
