
#include <cstdio>
#include <cinttypes>
#include <iostream>
#include <deque>
#include <vector>

#include "procsim.h"

using namespace std;

uint64_t GLOBAL_CLOCK = 0;
bool RAISE_EXCEPTION = false;
bool ALL_DONE = false;

// You may define any global variables here

deque <inst_t> dispQ;       //Dispatch Queue
vector <sch_instr> schQ;    //Schedule Queue
vector <PRF_t> PRF;         //Physical Register File
vector <RAT_t> RAT;         //Register Alias Table
vector <ROB_t> ROB;         //Reorder Buffer
vector <retired> ret_instr; //retired instructions
vector <retired2> ret;      //retired instructions (used for debugging)
SB_t SB;                    //scoreboard
uint64_t avg_dispQ;
uint64_t max_dispQ = 0;
uint64_t instr_num = 0;
uint64_t F;
uint64_t R;
uint64_t P;
uint64_t J;
uint64_t K;
uint64_t L;
uint64_t schQ_size;


/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 *
 * @param conf Pointer to the configuration. Read only in this function
 */
void setup_proc(const procsim_conf *conf)
{
    GLOBAL_CLOCK = 0;
    RAISE_EXCEPTION = false;
    ALL_DONE = false;

    // Student code should go below this line
    PRF.resize(32 + conf->P);
    RAT.resize(32);
    RAT[-1].preg_tag = -1;
    RAT[-1].preg_free = 1;
    PRF[-1].preg_free = 1;
    //set RAT to Args and free
    for (int32_t i = 0; i < 32; i++) {
        PRF[i].preg_free = 1;
        RAT[i].preg_tag = i;
        RAT[i].preg_free = 1;
    }
    //set PRF to free
    for (uint64_t i = 32; i < conf->P; i++) {
        PRF[i].preg_free = 1;
    }
    //set functional units to free
    for (uint64_t i = 0; i < 4; i++) {
        SB.FU_Busy[i] = 0;
        SB.FU_units[i] = 0;
    }

    schQ_size = 2 * (conf->J + conf->K + conf->L);
    F = conf->F;
    P = conf->P;
    J = conf->J;
    K = conf->K;
    L = conf->L;
    R = conf->R;

}


// Write helper methods for each superscalar operation here

static void fetch(procsim_stats *stats)
{
    instruction *new_inst = new instruction;
    for (uint64_t i = 0; i < F; i++) {
        if (read_instruction(new_inst)) {
            instr_num++;
            new_inst->num = instr_num;
            new_inst->fetch_cycle = GLOBAL_CLOCK;

            //assign FU based on opcode
            if (new_inst->opcode == 1) {
                new_inst->FU = 4;
            } else if (new_inst->opcode == 2) {
                new_inst->FU = 0;
            } else if (new_inst->opcode == 3) {
                new_inst->FU = 1;
            } else if (new_inst->opcode == 4) {
                stats->load_instructions++;
                new_inst->FU = 2;
            } else if (new_inst->opcode == 5) {
                stats->store_instructions++;
                new_inst->FU = 3;
            } else if (new_inst->opcode == 6) {
                stats->branch_instructions++;
                new_inst->FU = 0;
            } else if (new_inst->opcode == 7) {
                new_inst->FU = 0;
            }

            dispQ.push_back(*new_inst);
        }

    }
}

static void dispatch(procsim_stats *stats)
{
    sch_instr temp_instr;
    //handle exceptions
    if (RAISE_EXCEPTION == true) {
        //for loop to figure out if exception has already been added
        bool flag = false;
        for (uint64_t i = 0; i < ROB.size(); i++) {
            if (ROB[i].exception == true) {
                flag = true;
            }
        }
        //add exception instruction
        if (flag == false) {
            ROB_t temp_rob; 
            temp_instr.dest_preg = -1;
            temp_instr.dispatch_cycle = GLOBAL_CLOCK;
            temp_instr.num = -1;
            temp_instr.curr = 1;
            temp_instr.FU = 0;
            temp_instr.retire = false;
            temp_instr.src_ready[0] = 1;
            temp_instr.src_ready[1] = 1;
            temp_instr.src_tag[0] = -1;
            temp_instr.src_tag[1] = -1;
            temp_instr.tag = 1;
            temp_rob.dest_reg = temp_instr.dest_preg;
            temp_rob.num = -1;
            temp_rob.exception = true;
            temp_rob.ready = 0;
            temp_rob.prev_reg = -1;
            schQ.push_back(temp_instr);
            ROB.push_back(temp_rob);
        }
    } else {
        
        while (dispQ.size() > 0) {
            //handle NOP
            if (dispQ[0].FU == 4) {
                dispQ.erase(dispQ.begin());
            //add instructions if schQ and ROB not empty
            } else if (schQ.size() < schQ_size && ROB.size() < R) {
                //find a free Preg
                for (uint64_t j = 32; j < P; j++) {
                    bool flag = false;
                    for (uint64_t k = 0; k < schQ.size(); k++) {
                        if (schQ[k].src_tag[0] == j || schQ[k].src_tag[1] == j) {
                            flag = true;
                        }
                    }
                    if (RAT[dispQ[0].src_reg[0]].preg_tag == j || RAT[dispQ[0].src_reg[1]].preg_tag == j) {
                        flag = true;
                    }

                    if (PRF[j].preg_free == 1 && flag == false) {
                        temp_instr.dest_preg = dispQ[0].dest_reg;
                        temp_instr.FU = dispQ[0].FU;

                        for (uint64_t k = 0; k < 2; k++) {
                            temp_instr.src_tag[k] = RAT[dispQ[0].src_reg[k]].preg_tag;
                        }


                        ROB_t temp_rob; 
                        //ROB[tail].Dest_Areg = I.Dest
                        temp_rob.dest_reg = dispQ[0].dest_reg;

                        //ROB[tail].prev_Preg = RAT[I.Dest]
                        temp_rob.prev_reg = RAT[dispQ[0].dest_reg].preg_tag;

                        if (dispQ[0].dest_reg != -1) {
                            //RAT[I.dest] = find a free preg
                            RAT[dispQ[0].dest_reg].preg_tag = j;
                            //RAT[dispQ[0].dest_reg].preg_tag = j;
                            temp_instr.dest_preg = RAT[dispQ[0].dest_reg].preg_tag;
                            //Regs[RAT[I.Dest]].Ready = False
                            PRF[RAT[dispQ[0].dest_reg].preg_tag].preg_free = 0;
                        } else {
                            RAT[dispQ[0].dest_reg].preg_tag = -1;
                            temp_instr.dest_preg = RAT[dispQ[0].dest_reg].preg_tag;
                            PRF[RAT[dispQ[0].dest_reg].preg_tag].preg_free = 1;
                        }
                        //set values for instruction in schedule queue
                        temp_instr.fetch_cycle = dispQ[0].fetch_cycle;
                        temp_instr.dispatch_cycle = GLOBAL_CLOCK;
                        temp_instr.execute_cycle = 0;
                        temp_instr.num = dispQ[0].num;
                        temp_instr.tag = 1;
                        temp_instr.retire = false;
                        //set values for instruction in ROB
                        temp_rob.num = dispQ[0].num;
                        temp_rob.ready = 0;
                        temp_rob.exception = false;
                        //push instr to ROB and schQ
                        schQ.push_back(temp_instr);
                        ROB.push_back(temp_rob);

                        
                        //delete instruction from dispQ
                        dispQ.pop_front();
                        break;
                    }
                }
            } else {
                //if scheduleing queue is full, exit loop
                break;
            }
        }
    }
}

static void schedule(procsim_stats *stats)
{   
    for (uint64_t i = 0; i < 4; i++) {
        SB.FU_units[i] = 0;
    }
    //for each RS entry
    for (uint64_t i = 0; i < schQ.size(); i++) {
        //if src regs are ready and instr has not already been scheduled, schedule instr
        if (PRF[schQ[i].src_tag[0]].preg_free == 1 && PRF[schQ[i].src_tag[1]].preg_free == 1 && schQ[i].tag == 1) {
            //if instruction FU is available
            if (SB.FU_Busy[schQ[i].FU] == 0) {
                SB.FU_units[schQ[i].FU]++;
                //if ADD FU
                if (schQ[i].FU == 0) {
                    if (SB.FU_units[0] == J) {
                        SB.FU_Busy[0] = 1;
                    }
                //if MUL FU
                } else if (schQ[i].FU == 1) {
                    if (SB.FU_units[1] == K) {
                        SB.FU_Busy[1] = 1;
                    }
                //if load FU
                } else if (schQ[i].FU == 2) {
                    if (SB.FU_units[2] == L) {
                        SB.FU_Busy[2] = 1;
                    }
                //if store FU
                } else if (schQ[i].FU == 3) {
                    if (SB.FU_units[3] == 1) {
                        SB.FU_Busy[3] = 1;
                    }
                }
                //set more values for instruction after being scheduled
                schQ[i].curr = 1;
                schQ[i].schedule_cycle = GLOBAL_CLOCK;
                schQ[i].tag = 0;
                schQ[i].execute_cycle = 10;
                
                //set ready bits
                if (schQ[i].src_tag[0] == -1) {
                    PRF[schQ[i].src_tag[0]].preg_free = 1;
                }
                if (schQ[i].src_tag[1] == -1) {
                    PRF[schQ[i].src_tag[1]].preg_free = 1;
                }
                if (schQ[i].dest_preg != -1) {
                    PRF[schQ[i].dest_preg].preg_free = 0;
                }
            }
        }
    }
}

static void execute(procsim_stats *stats)
{
    for (uint64_t i = 0; i < schQ.size(); i++) {
        if (schQ[i].execute_cycle != 0) {
            //if ADD FU
            schQ[i].execute_cycle = GLOBAL_CLOCK;
            if (schQ[i].FU == 0) {
                if (schQ[i].curr == 1) {
                    SB.FU_Busy[0] = 0;
                    schQ[i].retire = true;
                    for (uint64_t j = 0; j < ROB.size(); j++) {
                        if (ROB[j].num == schQ[i].num) {
                            ROB[j].ready = 1;
                        }
                    }
                } else {
                    schQ[i].curr++;
                    SB.FU_Busy[0] = 0;
                }
            //if MUL FU
            } else if (schQ[i].FU == 1) {
                if (schQ[i].curr == 3) {
                    SB.FU_Busy[1] = 0;
                    schQ[i].retire = true;
                    for (uint64_t j = 0; j < ROB.size(); j++) {
                        if (ROB[j].num == schQ[i].num) {
                            ROB[j].ready = 1;
                        }
                    }
                } else {
                    schQ[i].curr++;
                    SB.FU_Busy[1] = 0;
                }
            //if load FU
            } else if (schQ[i].FU == 2) {
                if (schQ[i].curr == 2) {
                    SB.FU_Busy[2] = 0;
                    schQ[i].retire = true;
                    for (uint64_t j = 0; j < ROB.size(); j++) {
                        if (ROB[j].num == schQ[i].num) {
                            ROB[j].ready = 1;
                        }
                    }
                } else {
                    schQ[i].curr++;
                    SB.FU_Busy[2] = 0;
                }
            //if store FU
            } else if (schQ[i].FU == 3) {
                if (schQ[i].curr == 1) {
                    SB.FU_Busy[3] = 0;
                    schQ[i].retire = true;
                    for (uint64_t j = 0; j < ROB.size(); j++) {
                        if (ROB[j].num == schQ[i].num) {
                            ROB[j].ready = 1;
                        }
                    }
                } else {
                    schQ[i].curr++;
                    SB.FU_Busy[3] = 0;
                }
            }
        }
    }
}

static void state_update(procsim_stats *stats)
{   
    //loop through instructions and check if it needs to be retired
    for (uint64_t i = 0; i < schQ.size(); i++) {
        if (schQ[i].retire == true) {
            stats->instructions_retired++;
            retired temp_instr;
            temp_instr.fetch_cycle = schQ[i].fetch_cycle;
            temp_instr.dispatch_cycle = schQ[i].dispatch_cycle;
            temp_instr.schedule_cycle = schQ[i].schedule_cycle;
            temp_instr.execute_cycle = schQ[i].execute_cycle;
            PRF[schQ[i].dest_preg].preg_free = 1;
            temp_instr.num = schQ[i].num;
            ret_instr.push_back(temp_instr);
            //remove instruction from schedule queue
            schQ.erase(schQ.begin() + i);
            i--;
        }
    }
    
    if (ROB.size() > 0) {
        //retire instructions from ROB
        while(ROB[0].ready == 1 && ROB.size() > 0) {
            for (uint64_t i = 0; i < ret_instr.size(); i++) {
                if (ROB[0].num == ret_instr[i].num) {
                    retired2 temp;
                    temp.num = ret_instr[i].num;
                    temp.fetch_cycle = ret_instr[i].fetch_cycle;
                    temp.dispatch_cycle = ret_instr[i].dispatch_cycle;
                    temp.schedule_cycle = ret_instr[i].schedule_cycle;
                    temp.execute_cycle = ret_instr[i].execute_cycle;
                    temp.state_update_cycle = GLOBAL_CLOCK;
                    ret.push_back(temp);
                    ret_instr.erase(ret_instr.begin() + i);
                    i--;
                }
            }
            if (ROB[0].exception == true) {
                RAISE_EXCEPTION = false;
                //ROB.erase(ROB.begin());
            } else {
                PRF[ROB[0].prev_reg].preg_free = 1;
                //ROB.erase(ROB.begin());
            }
            ROB.erase(ROB.begin());
        }
    }
}


/**
 * Subroutine that simulates the processor. The processor should fetch instructions as
 * appropriate, until all instructions have executed
 *
 * @param stats Pointer to the statistics structure
 * @param conf Pointer to the configuration. Read only in this function
 */
void run_proc(procsim_stats *stats, const procsim_conf *conf)
{
    do {
        state_update(stats);
        execute(stats);
        schedule(stats);
        dispatch(stats);
        fetch(stats);

        GLOBAL_CLOCK++; // Clock the processor
        //calculate average dispQ size
        avg_dispQ = avg_dispQ + dispQ.size();
        if (max_dispQ < dispQ.size()) {
            max_dispQ = dispQ.size();
        }

        // Raise an exception/interrupt every 'I' clock cycles
        // When the RAISE_EXCEPTION FLAG is raised -- an Interrupt instruction is added
        // to the front of the dispatch queue while the schedule queue and ROB are flushed
        // Execution resumes starting from the Interrupt Instruction. The flushed instructions
        // are re-executed
        if ( (GLOBAL_CLOCK % conf->I) == 0 ) {
            RAISE_EXCEPTION = true;
            stats->num_exceptions++;
        }
    //run until schQ and dispQ are empty
    } while (schQ.size() > 0 || dispQ.size() > 0);
}

/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 *
 * @param stats Pointer to the statistics structure
 */
void complete_proc(procsim_stats *stats)
{
    //calculate final stats
    stats->cycle_count = GLOBAL_CLOCK;
    stats->average_disp_queue_size = (double) avg_dispQ / (double) GLOBAL_CLOCK;
    stats->max_disp_queue_size = max_dispQ;
    stats->average_ipc = (double) stats->instructions_retired / (double) GLOBAL_CLOCK;

    //clear all vectors
    dispQ.clear();
    schQ.clear();
    RAT.clear();
    ROB.clear();
    PRF.clear();
    ret_instr.clear();
    ret.clear();
}
