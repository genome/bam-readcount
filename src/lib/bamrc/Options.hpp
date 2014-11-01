#pragma once

#include <boost/program_options.hpp>

#include <stdexcept>
#include <string>
#include <vector>

class CmdlineHelpException : public std::runtime_error {
public:
    CmdlineHelpException(std::string const& msg)
        : std::runtime_error(msg)
    {
    }
};

class CmdlineError : public std::runtime_error {
public:
    CmdlineError(std::string const& msg)
        : std::runtime_error(msg)
    {
    }
};


struct Options {
    int min_mapq;
    int min_baseq;
    int max_depth;
    int max_warnings;
    int required_flags;
    int forbidden_flags;
    bool distribution;  // display all mapping qualities?
    bool per_lib;
    bool insertion_centric;

    std::string input_file;
    std::string fasta_file;
    std::string pos_file;
    std::string output_file;
    std::vector<std::string> regions;

    Options(int argc, char** argv);
    void validate();

private:
    std::string help_message() const;
    std::string version_message() const;
    void check_help() const;

private:
    std::string program_name;
    boost::program_options::options_description opts;
    boost::program_options::positional_options_description pos_opts;
    boost::program_options::variables_map var_map;
};
