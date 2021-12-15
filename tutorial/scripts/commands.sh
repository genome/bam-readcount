#!/bin/bash

dockercmd () {
  docker run --rm -w $(pwd) -v $(pwd):$(pwd) "$@"
}
dockercmd curlimages/curl -o NC_045512.2.fa "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi?db=nuccore&id=NC_045512.2&rettype=fasta"
dockercmd ncbi/sra-tools fastq-dump --split-files --origfmt SRR17054502
dockercmd seqfu/alpine-bwa bwa index NC_045512.2.fa
dockercmd seqfu/alpine-bwa bwa mem NC_045512.2.fa SRR17054502_1.fastq SRR17054502_2.fastq > CERI-KRISP-K032274.sam
dockercmd seqfu/alpine-samtools-1.10 samtools view -b CERI-KRISP-K032274.sam > CERI-KRISP-K032274.bam
dockercmd seqfu/alpine-samtools-1.10 samtools sort CERI-KRISP-K032274.bam > CERI-KRISP-K032274.sorted.bam
dockercmd seqfu/alpine-samtools-1.10 samtools index CERI-KRISP-K032274.sorted.bam
dockercmd mgibio/bam-readcount -w1 -f NC_045512.2.fa CERI-KRISP-K032274.sorted.bam NC_045512.2:21563-25384 > CERI-KRISP-K032274.brc.tsv
dockercmd curlimages/curl -o parse_brc.py "https://raw.githubusercontent.com/genome/bam-readcount/master/tutorial/scripts/parse_brc.py"
dockercmd python:3.8.2-alpine python parse_brc.py CERI-KRISP-K032274.brc.tsv | wc -l
dockercmd python:3.8.2-alpine python parse_brc.py --min-cov 10 --min-vaf 0.2 CERI-KRISP-K032274.brc.tsv | wc -l
dockercmd python:3.8.2-alpine python parse_brc.py --min-cov 10 --min-vaf 0.2 CERI-KRISP-K032274.brc.tsv
dockercmd python:3.8.2-alpine python parse_brc.py --min-cov 10 --min-vaf 0.2 CERI-KRISP-K032274.brc.tsv > CERI-KRISP-K032274.brc.filtered_table.tsv 
paste <(cut -f1 CERI-KRISP-K032274.brc.filtered_table.tsv) <(cut -f2 CERI-KRISP-K032274.brc.filtered_table.tsv) <(cut -f2 CERI-KRISP-K032274.brc.filtered_table.tsv) | grep -v chrom > CERI-KRISP-K032274.variant_position_list.tsv
dockercmd mgibio/bam-readcount -l CERI-KRISP-K032274.variant_position_list.tsv -w1 -f NC_045512.2.fa CERI-KRISP-K032274.sorted.bam NC_045512.2:21563-25384 > CERI-KRISP-K032274.brc.variant_positions.tsv
dockercmd python:3.8.2-alpine python parse_brc.py CERI-KRISP-K032274.brc.tsv > CERI-KRISP-K032274.brc.parsed.tsv 
