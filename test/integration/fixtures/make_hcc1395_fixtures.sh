#!/bin/sh
# Regenerate test-data/hcc1395 from the HCC1395 data on genomedata.org.
#
# Downloads ~700 MB into .cache/hcc1395 (gitignored) on first use, then runs
# make_hcc1395_fixtures.py inside the bam-readcount-test image
# (docker/Dockerfile.test) so tool versions are fixed. Expected outputs are
# regenerated separately with test/integration/update_golden.py.
set -eu

REPO=$(cd "$(dirname "$0")/../../.." && pwd)
CACHE="$REPO/.cache/hcc1395"
IMAGE=${IMAGE:-bam-readcount-test}
BASE=https://genomedata.org/hcc1395

mkdir -p "$CACHE"
cd "$CACHE"
for f in \
    results/chr6_and_chr17/align/Exome_Norm_sorted_mrkdup_bqsr.bam \
    results/chr6_and_chr17/align/Exome_Norm_sorted_mrkdup_bqsr.bai \
    references/genome/chr6_and_chr17/ref_genome.tar \
    references/exome/chr6_and_chr17/exome_regions.bed \
    results/chr6_and_chr17/germline/Exome_Norm_HC_calls.vcf
do
    [ -s "$(basename "$f")" ] || curl -sSfL -o "$(basename "$f")" "$BASE/$f"
done
if [ ! -s ref_genome.fa ]; then
    tar -xf ref_genome.tar ref_genome.fa.gz
    gunzip -c ref_genome.fa.gz > ref_genome.fa
fi

docker build -q -t "$IMAGE" -f "$REPO/docker/Dockerfile.test" "$REPO/docker" >/dev/null
docker run --rm -v "$REPO":"$REPO" -w "$REPO" "$IMAGE" sh -c "
    samtools faidx $CACHE/ref_genome.fa &&
    rm -rf test-data/hcc1395.new &&
    python3 test/integration/fixtures/make_hcc1395_fixtures.py \
        --cache $CACHE --out test-data/hcc1395.new &&
    mkdir -p test-data/hcc1395 &&
    find test-data/hcc1395 -maxdepth 1 -type f -delete &&
    mv test-data/hcc1395.new/* test-data/hcc1395/ &&
    rmdir test-data/hcc1395.new"
