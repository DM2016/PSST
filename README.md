# PSST
Polygenic SNP Search Tool

## Graphical Overview

![Workflow](/media/Polygenic_SNP_Search_Tool.png?raw=true "Workflow.png")

## Overview

The Polygenic SNP Search Tool is an open-source pipeline that **identifies multiple SNPs that are associated with diseases**; including SNPs that modify the *penetrance* of other SNPs. This pipeline identifies:
* Asserted pathogenic SNPs
* Genome-wide Association Studies (GWAS) identified SNPs
, crossed with database and datasets such as ClinVar, SRA, and GEO, and then constructs a report describing multiple genetic variants associated with diseases.


## Usage:

### Step 1 - Determine the top 100 phenotypes 

The script `top_100_phenotypes.sh` determines the top 100 disease phenotypes described in the latest GRCh38 release of ClinVar and outputs this into a file.

### Step 2 - Generate a summary table for the top 100 phenotypes

The script `summary_table.sh` generates a table that summarizes the number of SNPs and pathogenic variants associated with the top 100 disease phenotypes, as well as the number of SRA and GEO datasets available that are related to those phenotypes.

### Step 3 - Call polygenic variants in the SRA datasets for a particular phenotype

The script `psst.sh` determines the set of SNPs that are contained in each SRA dataset associated with the given phenotype/disease. For example, to determine the set of SNPs associated with each SRA dataset related to breast-ovarian cancer, run the command `psst.sh breast-ovarian_cancer ${PWD}`. This script will then output a TSV file describing which SNPs are associated with the breast-ovarian cancer SRA datasets. 

The `psst.sh` subpipeline is as follows:

1. Extracts flanking sequences for the phenotype associated SNP IDs and creates a FASTA file containing these flanking sequences. 

2. Uses `makeblastdb` to generate a BLAST database for the SNP flanking sequences.

3. Runs Magic-BLAST on each phenotype-associated SRA dataset and the SNP flanking sequence BLAST database.

4. From the Magic-BLAST alignments, determines which SNPs are contained in the SRA datasets using a statistical heuristic.

5. For each SRA dataset, outputs the set of IDs of the associated SNPs. 

