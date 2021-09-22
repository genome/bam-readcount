---
title: 'Bam-readcount - rapid generation of basepair-resolution sequence metrics'
tags:
  - genomics
  - cpp
  - sequencing
authors:
  - name: Ajay Khanna^[co-first author] 
    affiliation: 1
  - name: David E. Larson^[co-first author] 
    affiliation: "2,3"
  - name: Sridhar Nonavinkere Srivatsan
    affiliation: 1
  - name: Matthew Mosior
    affiliation: "1,4"
  - name: Travis E. Abbott
    affiliation: "2,5"
  - name: Susanna Kiwala
    affiliation: 2
  - name: Timothy J. Ley
    affiliation: "1,6"
  - name: Eric J. Duncavage
    affiliation: 7
  - name: Matthew J. Walter
    affiliation: "1,6"
  - name: Jason R. Walker
    affiliation: 2 
  - name: Obi L. Griffith
    affiliation: "1,2,6,8"
  - name: Malachi Griffith
    affiliation: "1,2,6,8"
  - name: Christopher A. Miller^[corresponding author]
    affiliation: "1,6"
affiliations:
  - name: Division of Oncology, Department of Internal Medicine, Washington University School of Medicine, St. Louis, MO
    index: 1
  - name: McDonnell Genome Institute, Washington University School of Medicine, St. Louis, MO
    index: 2
  - name: 'Current Affiliation: Benson Hill, Inc. St. Louis, MO'
    index: 3
  - name: 'Current Affiliation: Moffitt Cancer Center, Tampa, FL'
    index: 4
  - name: 'Current Affiliation: Google, Inc. Mountain View, CA'
    index: 5
  - name: Siteman Cancer Center, Washington University School of Medicine, St. Louis, MO
    index: 6
  - name: Department of Pathology, Washington University School of Medicine, St. Louis, MO
    index: 7
  - name: Department of Genetics, Washington University School of Medicine, St. Louis, MO
    index: 8
date: 8 August 2021
bibliography: refs/references.bib
---

# Summary

`Bam-readcount` is a utility for generating low-level information about
sequencing data at specific nucleotide positions. Originally designed to
help filter genomic mutation calls, the metrics it outputs are useful as
input for variant detection tools and for resolving ambiguity between
variant callers [@Koboldt2013-cm; @Kothen-Hill2018-is]. In addition, it
has found broad applicability in diverse fields including tumor
evolution, single-cell genomics, climate change ecology, and tracking
community spread of SARS-CoV-2 [@Paiva2020-za; @Miller2018-od;
@Muller2018-vy; @Sun2020-fm].

# Statement of need

Bam-readcount is designed to meet two related needs related to genomic
sequence analysis. The first is rapid genotyping of specific locations from
a bam file, reporting not just the dominant bases, but counts of
all bases. One context in which this is important is residual disease
monitoring, where base changes with frequency below the sensitivity of
standard genomic variant callers may still be informative. The second
is reporting 15 key metrics for each reported base, including summarized
mapping and base qualities, strandedness information, mismatch counts,
and position within the reads. This information can be useful in a large
number of contexts, with one frequent application being variant filtering,
to remove false-positive calls, either with straightforward application of
heuristic cutoffs or with semi-automated machine-learning approaches
[@Ainscough2018-yp; @Koboldt2013-us]. Another common use case is in ensemble
variant calling situations where there is disagreement about base counts or
key metrics at particular sites. Bam-readcount can be used to produce
consistent, tool-agnostic metrics that are helpful in resolving such
ambiguity [@Anzar2019-vp; @Kothen-Hill2018-is; @Kockan2017-to].

# Implementation and results

The ongoing adoption of compressed data formats has necessitated
additions to the code, and the version 1.0 release that we report on
here utilizes an updated version of HTSlib to support rapid CRAM file
access [@Bonfield2021-et]. This has also improved performance, and
`bam-readcount` can report on 100,000 randomly selected sites from a 30x
whole-genome sequencing (WGS) BAM in around 5 minutes
[@Griffith2015-gz]. Its performance scales nearly linearly with the
number of genomic sites queried and average sequencing depth (Figure 1).
Querying the same 100,000 sites from a BAM with 300x WGS takes 48
minutes, roughly 10x as long.  

![Performance of `bam-readcount` when querying randomly
selected genomic positions from BAMs (left) or corresponding CRAMs
(right) of varying sequencing depth. Colors correspond to average
sequencing depth of the downsampled BAM/CRAM file.
\label{fig:1}](figures/figure1.pdf)

Memory usage likewise is dependent on depth of sequencing, but still
requires less than 1 GB of RAM for a 300x WGS BAM. Processing small CRAM
files is somewhat slower than BAMs with comparable amounts of data, due
to the increased CPU usage for decompression, but as depth increases,
retrieval from disk becomes the bottleneck and operations on CRAMs
exceed the speed of BAM. In our testing, on a fast SSD tier of networked
disk, this transition occurs at a depth of about 180x. The problem is
also embarrassingly parallel, so assuming adequate disk I/O, a roughly
linear increase in speed can be achieved with a scatter/gather approach. 

To lower barriers to adoption, we provide docker images for
containerized workflows, and have developed a python wrapper that
annotates a VCF file with read counts produced from this tool, available
as part of the VAtools package
([http://vatools.org](http://vatools.org)). 

# Conclusions

`bam-readcount` provides fast and accurate genomic readcounts and
associated metrics, which allow it to fill a key niche in many
genomic workflows. It has been adopted as a lightweight variant caller,
finding known mutations in pre-leukemic phenotypes and used for
detecting therapy-altering mutations from cell-free DNA
[@Wyatt2016-si; @Xie2014-mm]. Viral researchers have tracked nucleotide
chnages across samples to understand diversity in Varicella Zoster
Virus Encephalitis and to perform epidemiological surveillance in
wastewater of SARS-CoV-2 [@Depledge2018-mn; @Mondal2021-gw]. Those
with RNA-sequencing data have found it useful for identifying
allele-specific expression in cancer, or for enabling copy-number
detection in single-cell RNA sequencing by retrieving allele frequencies
[@Cancer_Genome_Atlas_Research_Network2013-kd; @Muller2018-vy]. Its
feature-rich output has also enabled deep learning approaches to variant
calling and filtering [@Ainscough2018-yp; @Anzar2019-vp]. In these
roles, and other related ones, `bam-readcount` has served as key
infrastructure that supports groups of all sizes, from exploratory
analyses to core facility pipelines to large multi-institution
workflows [@Jensen2017-gp; @Griffith2015-wx; @Sandmann2018-hq]. In the
NCI’s Genomic Data Commons pipelines alone, its use in variant filtering
means that it has been run on tens of thousands of cancer genomes. 

Looking forward, we anticipate that as machine learning makes deeper
inroads into genomics, the ability to extract highly informative
features from large cohorts in a rapid manner will continue to make
`bam-readcount` useful for the next generation of genomics research.

The `bam-readcount` tool is available at
[https://github.com/genome/bam-readcount](https://github.com/genome/bam-readcount)
and is shared under a MIT license to enable broad re-use. 

# Data availability

The WGS data used for benchmarking is available through dbGaP study
[phs000159](https://www.ncbi.nlm.nih.gov/projects/gap/cgi-bin/study.cgi?study_id=phs000159),
under sample id `452198/AML31`. An archived snapshot of this 1.0 release
is available at
[https://doi.org/10.5281/zenodo.5142454](https://doi.org/10.5281/zenodo.5142454)

# Authors' contributions

Software Development: AK, DEL, SNS, MM, TEA, SK, CAM. Validation: AK,
SNS, MM, CAM. Visualization: CAM. Supervision: CAM, MG, OLG, TJL, EJD,
JRW, MJW  Writing, review, and editing:  AK, DEL, SNS, MM, TEA, SK, TJL,
EJD, MJW, JRW, OLG, MG, CAM

# Acknowledgements

This work was supported by the National Cancer Institute [R50CA211782 to
CAM, P01CA101937 to TJL, K22CA188163 to OLG, 1U01CA209936 to OLG,
U24CA237719 to OLG], the Edward P. Evans Foundation (to MJW), and the
National Human Genome Research Institute [R00 HG007940 to MG]


# References

