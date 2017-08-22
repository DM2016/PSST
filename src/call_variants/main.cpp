#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <cstdlib> // For std::exit
#include <map>
// For multithreading
#include <future>
#include <thread>
// Custom libraries
#include "structs.hpp"
#include "data_manipulation.hpp"
#include "queries_with_ref_bases.hpp"

std::vector<BlastOutput> get_mbo_paths(std::string mbo_list_path)
/* Retrieves the paths to the MBO alignment files */
{
    // Open the file
    std::ifstream mbo_list_file (mbo_list_path,std::ios::in);
    if ( not mbo_list_file.is_open() ) {
        std::cerr << "Error: unable to read SRA list file.\n";
        std::exit(1);
    }

    std::string line;
    std::vector<BlastOutput> mbo_paths;

    // Each line of the file corresponds to an line number
    while ( std::getline(mbo_list_file,line) ) {
        // Remove new lines from the line
        line.erase( std::remove(line.begin(), line.end(), '\n'), line.end() );
        // Split into tokens
        std::vector<std::string> tokens = split(line); 
        // The first token is the accession, the second is the MBO path
        BlastOutput mbo_path;
        mbo_path.accession = tokens.at(0);
        mbo_path.path = tokens.at(1); 
        // Add it to the list
        mbo_paths.push_back(mbo_path); 
    }

    mbo_list_file.close();
    return mbo_paths
}

std::map<std::string,VarBoundary> get_var_boundary_map(std::string var_boundary_path)
/* Retrieve the variant flanking sequence information from a file */
{
    // Open the file
    std::ifstream var_boundary_file (var_boundary_path,std::ios::in);
    if ( not var_boundary_file.is_open() )
    {
        std::cerr << "Error: unable to read variant information file.\n";
        std::exit(1);
    }

    std::string line;
    std::map<std::string, VarBoundary> var_boundary_map;

    // Each line corresponds to the information for a single variant
    while ( std::getline(var_boundary_file,line) ) {
        // Remove new lines from the line
        line.erase( std::remove(line.begin(), line.end(), '\n'), line.end() );
        // Split into tokens
        std::vector<std::string> tokens = split(line);
        // Put the tokens into the variant info object
        accession = tokens.at(0);
        VarBoundary var_boundary;
        // Convert the strings into integers
        var_boundary.start = std::stoi( tokens.at(1) );
        var_boundary.stop = std::stoi( tokens.at(2) ); 
        var_boundary_map[accession] = var_boundary;
    } 

    var_inf_file.close();
    return var_boundary_map;
}

SraAlignments get_sra_alignments(BlastOutput blast_output)
/* Retrieves the Magic-BLAST alignments for a particular SRA dataset */
{
    accession = blast_output.accession
    path = blast_output.path;

    std::ifstream mbo_file (path,std::ios::in);
    if ( not mbo_file.is_open() ) {
        std::cerr << "Error: unable to read " << path << std::endl;
        std::exit(1);
    }

    std::string line;
    std::vector<Alignment> alignments;
    Alignment alignment;
    
    while ( std::getline(mbo_file,line) ) {
        // Remove new lines from the line
        line.erase( std::remove(line.begin(), line.end(), '\n'), line.end() );
        // Split into tokens like awk
        std::vector<std::string> tokens = split(line);
        std::string variant_acc = tokens.at(1);
        int ref_start = std::stoi( tokens.at(8) );
        int ref_stop = std::stoi( tokens.at(9) );
        if (ref_start > ref_stop) {
            int temp = ref_start;
            ref_start = ref_stop;
            ref_stop = temp;
        }
        std::string btop = tokens.at(16);
        alignment.variant_acc = variant_acc;
        alignment.ref_start = ref_start;
        alignment.ref_stop = ref_stop;
        alignment.btop = btop;
        alignments.push_back(alignment); 
    } 
    SraAlignments sra_alignments;
    sra_alignments.accession = accession;
    sra_alignments.alignments = alignments;
    return sra_alignments;
} 

std::vector<CalledVariants> call_variants_in_datasets(const std::vector<BlastOutput> &blast_outputs,
                                                   const std::map<std::string,VarBoundary> &var_boundary_map)
/* Determines which variants exist in multiple SRA datasets */
{
    std::vector<CalledVariants> called_variants_list;
    // Call variants in each of the SRA datasets
    for (int index = 0; index < blast_outputs.size(); index++) {
        BlastOutput blast_output = blast_outputs.at(index);
        SraAlignments sra_alignments = get_sra_alignments(blast_output);
        VariantFrequenciesMap var_freq_map;
        // Analyze the alignments of the SRA dataset onto the variant flanking sequences
        for (int alignments_index = 0; alignments_index < sra_alignments.size(); alignments_index++) {
            Alignment alignment = sra_alignments.get(alignments_index);
            std::string variant_acc = alignment.variant_acc;
            VarBoundary var_boundary = var_boundary_map[var_acc];
            // Make sure the alignment spans the variant first
            if ( alignment_spans_variant(alignment,var_boundary) ) {
                // Make sure the variant exists in var_freq_map
                if ( var_freq_map.variant_frequency_map.find(variant_acc) 
                     == var_freq_map.variant_frequency_map.end() ) {
                    var_freq_map.variant_accessions.push_back(variant_acc);
                    var_freq_map.variant_frequency_map[variant_acc].matches = 0;
                    var_freq_map.variant_frequency_map[variant_acc].mismatches = 0;
                }
                //Determine whether there is a match between the read and the variant flanking sequence at the variant
                matched = query_contains_ref_bases(alignment,var_boundary);
                // Update the matches and mismatches counts
                if (matched) {var_freq_map.variant_frequency_map[variant_acc].matches++;}
                else {var_freq_map.variant_frequency_map[variant_acc].mismatches++;}
            }
        }
        // From all the variant match and mismatch counts, determine which variants exist in the SRA dataset
        CalledVariants called_variants = call_variants(var_freq_map);
        called_variants.accession = accession;
        called_variants_list.push_back(sra_variants);
    }
    return called_variants_list;
}

void write_tsv(const std::vector< std::vector<CalledVariants> > &called_variants_partitions, std::string output_path)
/* Create a TSV file describing which variants exist in each SRA dataset */
{
    std::ofstream tsv_file (output_path, std::ios::out | std::ios::trunc);
    if (tsv_file.is_open()) {
        tsv_file << "SRA Dataset \t Heterozygous \t Homozygous\n";
        for (int i = 0; i < called_variants_partitions.size(); i++) {
            std::vector<CalledVariants> called_variants = called_variants_partitions.at(i);
            for (int u = 0; u < called_variants.size(); u++) {
                CalledVariants sra_variants = called_variants.at(u);
                std::string accession = sra_variants.accession;
                std::vector<std::string> heterozygous = sra_variants.heterozygous;
                std::vector<std::string> homozygous = sra_variants.homozygous;
                tsv_file << accession << " \t ";
                for (int het_index = 0; het_index < heterozygous.size(); het_index++) {
                    tsv_file << heterozygous.at(het_index) << " ";
                }
                tsv_file << " \t ";
                for (int hom_index = 0; hom_index < homozygous.size(); hom_index++) {
                    tsv_file << homozygous.at(hom_index) << " ";
                } 
                tsv_file << "\n";
            }
        }
        tsv_file.close();
    } else {
        std::cerr << "Error: unable to create TSV file.\n";
    }
} 

int main(int argc, char *argv[])
{
    int opt;

    std::string mbo_list_path = "";
    std::string var_boundary_path = "";
    int64_t threads = 1;
    std::string output_path = "";

    while ( (opt = getopt(argc, argv, "hm:v:p:o:")) != 1 ) {
        switch (opt) {
            case 'h':
                display_help();
                display_usage();
                return 0;
            case 'm':
                mbo_list_path = optarg;
                break;
            case 'v':
                var_boundary_path = optarg;     
                break;
            case 'p':
                threads = std::stoi(optarg);
                break;
            case 'o':
                output_path = optarg;
                break;
            default:
                std::cerr << "Error: unrecognized option.\n";
                display_usage();
                return 1;
        }
    }

    bool opts_incomplete = false;

    if (mbo_list_path == "") {
        std::cerr << "Error: please provide the path to the file containing the paths of the MBO files.\n";
        opts_incomplete = true;
    }
    if (var_boundary_path == "") {
        std::cerr << "Error: please provide the path to the variant flanking sequence information file.\n";
        opts_incomplete = true;
    }
    if (output_path == "") {
        std::cerr << "Error: please specify an output path.\n";
        opts_incomplete = true;
    }
    if (opts_incomplete) {
        display_usage();
        return 1;
    }

    std::vector<BlastOutput> mbo_paths_list = get_mbo_paths(mbo_list_path);    
    std::map<std::string,VarBoundary> var_boundary_map = get_var_boundary_map(var_boundary_path);
    std::vector< std::vector<BlastOutput> > mbo_paths_partitions = partition_mbo_paths(mbo_paths_list,threads);
    // Process each partition separately in its own thread
    std::vector< std::future<std::vector<BlastOutput>> > partition_threads;
    // Only create thread - 1 new threads; work will be done in the main thread, too
    for (int i = 1; i < mbo_paths_partitions.size(); i++) {
        partition_threads.push_back( std::async(std::launch::async, call_variants_in_datasets,
                                                mbo_paths_partitions.at(i), var_boundary_map) ); 
    }
    // Container for called variants
    std::vector< std::vector<CalledVariants> > called_variants_partitions;
    // Perform work in this thread, too
    called_variants_partitions.push_back( call_variants_in_datasets(mbo_paths_partitions.at(0),var_boundary_map) );
    // Retrieve output from all the threads
    for (int i = 0; i < partition_threads.size(); i++) {
        called_variants_partitions.push_back( partition_threads.at(i).get() ); 
    }
    // Write the TSV file
    write_tsv(called_variants_partitions,output_path);
}