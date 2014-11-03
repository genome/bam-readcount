#include "Options.hpp"

#include <bam.h>

#include <boost/format.hpp>

#include <ios>
#include <iomanip>
#include <sstream>

namespace po = boost::program_options;
using boost::format;

#ifndef BAM_FSUPPLEMENTAL
# define BAM_FSUPPLEMENTAL 2048
#endif

namespace {
    // From the SAM spec
    std::vector<std::pair<int, char const*>> SAM_FLAG_DESCRIPTIONS = {
          {  0x1, "template having multiple segments in sequencing"}
        , {  0x2, "each segment properly aligned according to the aligner"}
        , {  0x4, "segment unmapped"}
        , {  0x8, "next segment in the template unmapped"}
        , { 0x10, "SEQ being reverse complemented"}
        , { 0x20, "SEQ of the next segment in the template being reversed"}
        , { 0x40, "the first segment in the template"}
        , { 0x80, "the last segment in the template"}
        , {0x100, "secondary alignment"}
        , {0x200, "not passing quality controls"}
        , {0x400, "PCR or optical duplicate"}
        , {0x800, "supplementary alignment"}
        };

    std::string sam_flags_str() {
        std::stringstream ss;

        auto const& x = SAM_FLAG_DESCRIPTIONS;
        std::string divider(78, '-');

        ss << std::setw(8) << "Decimal" << " "
            << std::setw(8) << "Hex" << "  " << "Description" << "\n"
            << divider << "\n";

        for (auto i = x.begin(); i != x.end(); ++i) {
            ss << std::setw(8) << std::dec << i->first << " "
                << std::setw(8) << std::showbase << std::hex << i->first
                << "  " << i->second << "\n";
        }

        return ss.str();
    }
}


std::string Options::help_message() const {
    std::stringstream ss;
    ss << "\nUsage: " << program_name << " [OPTIONS]" << " <input-file>\n\n";
    ss << opts << "\n";
    return ss.str();
}


std::string Options::version_message() const {
    std::stringstream ss;
    ss << "\nExecutable:\t" << program_name << "\n";
    return ss.str();
}


void Options::check_help() const {
    if (var_map.count("list-sam-flags")) {
        throw CmdlineHelpException(sam_flags_str());
    }

    if (var_map.count("version")) {
        throw CmdlineHelpException(version_message());
    }

    if (var_map.count("help")) {
        throw CmdlineHelpException(help_message());
    }
}


Options::Options(int argc, char** argv)
    : program_name(argv[0])
{
    pos_opts.add("input-file", 1);
    pos_opts.add("region", -1);


    po::options_description help_opts("Help Options");
    help_opts.add_options()
        ("help,h", "this message")
        ("version,v", "display version information")
        ("list-sam-flags", "Print SAM flag reference table")
        ;

    po::options_description gen_opts("General Options");
    gen_opts.add_options()
        ("input-file,i"
            , po::value<std::string>(&input_file)->required()
            , "sorted, indexed bam file to count reads in (1st positional arg)")

        ("output-file,o"
            , po::value<std::string>(&output_file)->default_value("-")
            , "output file (- for stdout)")

        ("reference-fasta,f"
            , po::value<std::string>(&fasta_file)
            , "reference sequence in the fasta format.")

        ("site-list,l"
            , po::value<std::string>(&pos_file)
            , "file containing a list of regions to report readcounts within.")

        ("region,R"
            , po::value<std::vector<std::string>>(&regions)
            , "list of regions (chr:start-stop) to process")
        ;

    po::options_description rep_opts("Reporting Options");
    rep_opts.add_options()
        ("per-library,p"
            , po::bool_switch(&per_lib)->default_value(false)
            , "Report results by library")

        ("insertion-centric,i"
            , po::bool_switch(&insertion_centric)->default_value(false)
            , "generate indel centric readcounts. Reads containing insertions "
              "will not be included in per-base counts")

        ("print-individual-mapq,D"
            , po::value<bool>(&distribution)->default_value(false)
            , "report the mapping qualities as a comma separated list.")

        ("max-count,d"
            , po::value<int>(&max_depth)->default_value(10000000)
            , "max depth to avoid excessive memory usage.")
        ;

    po::options_description flt_opts("Filtering Options");
    flt_opts.add_options()
        ("min-mapping-quality,q"
            , po::value<int>(&min_mapq)->default_value(0)
            , "minimum allowable mapping quality.")

        ("min-base-quality,b"
            , po::value<int>(&min_baseq)->default_value(0)
            , "minimum allowable base quality.")

        ("required-flags,r"
            , po::value<int>(&required_flags)->default_value(0)
            , "SAM flags that each read must have")

        ("forbidden-flags,x"
            , po::value<int>(&forbidden_flags)->default_value(0)
            , "SAM flags that each read is forbidden to have")

        ("max-warnings,w"
            , po::value<int>(&max_warnings)->default_value(-1)
            , "maximum number of warnings of each type to emit. -1 gives an "
              "unlimited number.")
        ;

    opts.add(help_opts).add(gen_opts).add(rep_opts).add(flt_opts);

    if (argc <= 1)
        throw CmdlineHelpException(help_message());

    try {

        auto parsed_opts = po::command_line_parser(argc, argv)
                .options(opts)
                .positional(pos_opts).run();

        po::store(parsed_opts, var_map);
        po::notify(var_map);
        validate();

    } catch (std::exception const& e) {
        // program options will throw if required options are not passed
        // before we have a chance to check if the user has asked for
        // --help. If they have, let's give it to them, otherwise, rethrow.
        check_help();

        std::stringstream ss;
        ss << help_message() << "\n\nERROR: " << e.what() << "\n";
        throw CmdlineError(ss.str());
    }

    check_help();

}

void Options::validate() {
    if ((required_flags & forbidden_flags) != 0) {
        throw std::runtime_error(str(format(
            "Required flags (%1%) and forbidden flags (%2%) must be disjoint."
            ) % required_flags % forbidden_flags));
    }
}
