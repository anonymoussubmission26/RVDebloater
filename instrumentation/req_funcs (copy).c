#include <string.h>
//#include <stdint.h>
//#include <stdbool.h>  // Include this for true/false in C
#include <stdio.h>

#define true 1
#define false 0

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

enum Number curr_mode_id = STABILIZE;

/* Used for tracking modes (should be added to mode manager) */
void __attribute__((noinline)) mode_entry(enum Number mode_id) {
    curr_mode_id = mode_id;
    // START_TIMER();
    // __asm__("ud2\n\t");
    // END_TIMER(syscall_entry_time);
}

void __attribute__((noinline)) mode_exit(enum Number mode_id) {
    curr_mode_id = mode_id;
    // START_TIMER();
    // __asm__("ud2\n\t");
    // END_TIMER(syscall_exit_time);
}

/* Used for Profiling */
typedef char bool;
//bool mode_to_func_mapping[29][30000] = {{0, 1, 2, 3, 4, 5}};
bool log_fn_initialized = false;

//char curr_fn_name[100] = "helloworld";
//int are_we_tracking = -1;

//void __attribute__((noinline)) log_fn(int id, char* str, int size) {
void __attribute__((noinline)) log_fn(int fn_id) {
    //if (are_we_tracking <= 0) return;
    bool log_fn_initialized = false;
    if (!log_fn_initialized) {
        int j, i;  // Declare variables outside of the for loops
        for (j = 0; j < 30000; j++) {
            for (i = 0; i < 29; i++) {  // Adjusted for 29 modes
                //mode_to_func_mapping[i][j] = false;
            }
        }
        log_fn_initialized = true;
    }
    //printf("function is %s , id %d\n", str, curr_mode_id);
    //printf("function id is %d\n", id);
    //if (mode_to_func_mapping[curr_mode_id][id]) return;
    //mode_to_func_mapping[curr_mode_id][id] = true;

    //memcpy(curr_fn_name, str, size);
    //curr_fn_name[size] = '\0';
    //__asm__("nop\n\t");
    //__asm__("nop\n\t");
    return;
}

