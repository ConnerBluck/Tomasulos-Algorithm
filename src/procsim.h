#ifndef PROCSIM_H
#define PROCSIM_H

#include <cstdint>
#include <cstdio>

// Default structure
#define DEFAULT_F 4
#define DEFAULT_P 64
#define DEFAULT_J 3
#define DEFAULT_K 1
#define DEFAULT_L 2
#define DEFAULT_R 12
#define DEFAULT_I 10000

// List of opcodes
typedef enum {
    OP_NOP = 1,     // NOP
    OP_ADD = 2,     // ADD
    OP_MUL = 3,     // MUL
    OP_LOAD = 4,    // LOAD
    OP_STORE = 5,   // STORE
    OP_BR = 6       // BRANCH
} opcode_t;

struct procsim_conf {
    uint64_t F;     // Fetch rate
    uint64_t P;     // Number of PRegs
    uint64_t J;     // Number of ADD / BR Units
    uint64_t K;     // Number of MUL units
    uint64_t L;     // Number of Load/Store units
    uint64_t R;     // ROB size
    uint64_t I;     // Interrupt/Exception interval (cycles)
};

// Instruction struct partially populated by the driver file
typedef struct instruction {

    uint64_t inst_addr;             // instruction address
    int32_t opcode;                 // instruction opcode
    int32_t src_reg[2];             // Source registers
    int32_t dest_reg;               // Destination register
    uint64_t br_target;             // branch target if the instruction is a branch
    bool br_taken;                  // actual behavior of the branch instruction
    uint64_t ld_st_addr;            // Effective address of Load or Store instruction

    // clock fields - You will be filling in these for each instruction
    uint64_t fetch_cycle;           // cycle in which fetched
    uint64_t dispatch_cycle;        // cycle in which dispatched
    uint64_t schedule_cycle;        // cycle in which scheduled
    uint64_t execute_cycle;         // cycle in which executed
    uint64_t state_update_cycle;    // cycle in which retired

    // You may introduce other fields as needed

} inst_t;

// Simulation statistics tracking struct
struct procsim_stats {
    uint64_t instructions_retired;          // Total number of instructions completed/retired
    uint64_t num_exceptions;                // Total number of exceptions in the program

    uint64_t branch_instructions;           // Total branch instructions
    uint64_t load_instructions;             // Number of loads
    uint64_t store_instructions;            // Number of stores

    double average_disp_queue_size;         // Average dispatch queue size
    uint64_t max_disp_queue_size;           // Maximum dispatch queue size
    uint64_t cycle_count;                   // Total cycle count (runtime in cycles)
    double average_ipc;                     // Average instructions retired per cycle (IPC)
};

bool read_instruction(inst_t* p_inst);

void setup_proc(const procsim_conf *config);
void run_proc(procsim_stats *stats, const procsim_conf  *conf);
void complete_proc(procsim_stats *stats);

#endif // PROCSIM_H
