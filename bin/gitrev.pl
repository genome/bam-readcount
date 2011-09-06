#!/usr/bin/env perl

use POSIX qw/WEXITSTATUS/;

my $vtag = "v[0-9]*";

sub git {
    my $args = shift;
    my $output = `git $args 2>&1`;
    my $rc = WEXITSTATUS($?);
    die "git command '$args' returned $rc. output was '$output'" if $rc;
    chomp $output;
    return $output;
}

sub is_git_repo {
    eval { git("status"); };
    return !$@;
}

sub is_dirty {
    my $status = git("status --porcelain");
    return $status ne "";
}

sub is_tagged {
    my $status = git("describe --always --dirty --long");
    return 0 if $status =~ /-dirty/;
    if ($status =~ /^.*-([0-9]+)-g[^-]+$/) {
        return $1 == 0;
    }
    return;
}

sub tagged_version {
    my $rev;
    eval { $rev = git("describe --long --match $vtag"); };
    return unless !$@ and $rev ne "";
    return $rev;
}

sub untagged_version {
    my $rev = git("describe --always");
    return $rev;
}

sub parse_rev {
    my $rev = shift;

    $rev =~ s/^v//g;

    my $exe_suffix = "-unstable";
    my $full_version = "$rev-unstable";
    my $commit = $rev;
    my $dirty = is_dirty;

    if ($rev =~ /^([0-9]+)\.([0-9]+)\.([0-9]+)-([0-9]+)-g(.*)/) {
        $exe_suffix = "$1.$2";
        $full_version = "$1.$2.$3";
        $commit = $5;

        my $commits_past_tag = $4;
        if ($commits_past_tag > 0) {
            $exe_suffix = "$1.$2.$3-$commits_past_tag-unstable";
            $full_version = "$1.$2.$3-unstable-$commits_past_tag-$commit";
        }
    }
    if ($dirty) {
        $full_version .= "-dirty";
        $commit .= "-dirty";
    }
    return ($exe_suffix, $full_version, $commit, $dirty);
}

sub commit_hash {
    return git("rev-parse --short HEAD");
}

if (!is_git_repo) {
    print "-unstable ";
    print "unstable ";
    print "nogit\n";
    exit 0;
}

my $rev = tagged_version || untagged_version;

my ($exe_suffix, $full_version, $commit, $dirty) = parse_rev($rev);
print "$exe_suffix ";
print "$full_version ";
print "$commit\n";
