#include <cstdio>
#include <cinttypes>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <unistd.h>

#include "procsim.h"

FILE* inFile = stdin;

// Print help message and exit
static void print_help_and_exit(void)
{
    fprintf(stderr, "procsim [OPTIONS]\n");
    fprintf(stderr, "  -f F\t\tNumber of instructions to fetch\n");
    fprintf(stderr, "  -p P\t\tNumber of PRegs\n");
    fprintf(stderr, "  -j J\t\tNumber of ADD/BR FUs\n");
    fprintf(stderr, "  -k K\t\tNumber of MUL FUs\n");
    fprintf(stderr, "  -l L\t\tNumber of LOAD/STORE FUs\n");
    fprintf(stderr, "  -r R\t\tNumber of entries in the ROB\n");
    fprintf(stderr, "  -i I\t\tInterrupt frequency (cycles)\n");
    fprintf(stderr, "  -t <Path to trace> Input trace file\n");
    fprintf(stderr, "  -h\t\tThis helpful output\n");
    exit(EXIT_SUCCESS);
}

// Print configuration
static void print_config(procsim_conf *config)
{
    printf("Processor Settings\n");
    printf("F: %" PRIu64 "\n", config->F);
    printf("P: %" PRIu64 "\n", config->P);
    printf("J: %" PRIu64 "\n", config->J);
    printf("K: %" PRIu64 "\n", config->K);
    printf("L: %" PRIu64 "\n", config->L);
    printf("R: %" PRIu64 "\n", config->R);
    printf("I: %" PRIu64 "\n", config->I);
    printf("\n");
}

// Prints statistics at the end of the simulation
static void print_statistics(procsim_stats* stats) {
    printf("\n\n");
    printf("Processor stats:\n");
    printf("Total instructions retired:     %" PRIu64 "\n", stats->instructions_retired);
    printf("Number of exceptions:           %" PRIu64 "\n", stats->num_exceptions);
    printf("Total Branch instructions:      %" PRIu64 "\n", stats->branch_instructions);
    printf("Total load instructions:        %" PRIu64 "\n", stats->load_instructions);
    printf("Total store instructions:       %" PRIu64 "\n", stats->store_instructions);
    printf("Average Dispatch Queue size:    %f\n", stats->average_disp_queue_size);
    printf("Maximum Dispatch Queue size:    %" PRIu64 "\n", stats->max_disp_queue_size);
    printf("Average instructions retired:   %f\n", stats->average_ipc);
    printf("Final Cycle count:              %" PRIu64 "\n", stats->cycle_count);
}

/* Function to read instruction from the input trace. Populates the inst struct
 *
 * returns true if an instruction was read successfully. Returns false at end of trace
 *
 */
bool read_instruction(inst_t* inst)
{
    int ret;
    if (inst == NULL) {
        fprintf(stderr, "Fetch requires a valid pointer to populate\n");
        return false;
    }

    // Don't modify this line. Instruction fetch might break otherwise!
    ret = fscanf(inFile, "%" PRIx64 " %d %d %d %d %" PRIx64 " %" PRIx64 " %d\n", &inst->inst_addr, &inst->opcode, &inst->dest_reg,
                    &inst->src_reg[0], &inst->src_reg[1], &inst->ld_st_addr, &inst->br_target, (int *) &inst->br_taken);

    if (ret != 8) { // Check if something went really wrong
        if (!feof(inFile)) { // Check if end of file has been reached
            fprintf(stderr, "Something went wrong and we could not parse the instruction\n");
        }
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage: Missing input file trace\n");
        print_help_and_exit();
    }


    int opt;

    printf("Number of arguments when files passed %d\n", argc);

    // Default configuration setup
    procsim_conf conf = {   .F = DEFAULT_F,
                            .P = DEFAULT_P,
                            .J = DEFAULT_J,
                            .K = DEFAULT_K,
                            .L = DEFAULT_L,
                            .R = DEFAULT_R,
                            .I = DEFAULT_I
                        };

    while(-1 != (opt = getopt(argc, argv, "f:p:j:k:l:r:y:i:t:h"))) {
        switch(opt) {
        case 'f':
            conf.F = atoi(optarg);
            break;
        case 'p':
            conf.P = atoi(optarg);
            break;
        case 'j':
            conf.J = atoi(optarg);
            break;
        case 'k':
            conf.K = atoi(optarg);
            break;
        case 'l':
            conf.L = atoi(optarg);
            break;
        case 'r':
            conf.R = atoi(optarg);
            break;
        case 'i':
            conf.I = atoi(optarg);
            break;
        case 't':
            inFile = fopen(optarg, "r");
            if (inFile == NULL) {
                fprintf(stderr, "Failed to open %s for reading\n", optarg);
                print_help_and_exit();
            }
            break;
        case 'h':
        default:
            print_help_and_exit();
            break;
        }
    }

    print_config(&conf); // Print run configuration

    setup_proc(&conf); // Setup the processor

    procsim_stats stats;
    memset(&stats, 0, sizeof(procsim_stats));

    run_proc(&stats, &conf); // Run the processor

    complete_proc(&stats); // Finalize statistics and perform cleanup

    fclose(inFile); // release file descriptor memory

    print_statistics(&stats);

    return 0;
}
