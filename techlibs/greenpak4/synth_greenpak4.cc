/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/register.h"
#include "kernel/celltypes.h"
#include "kernel/rtlil.h"
#include "kernel/log.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

bool check_label(bool &active, std::string run_from, std::string run_to, std::string label)
{
	if (label == run_from)
		active = true;
	if (label == run_to)
		active = false;
	return active;
}

struct SynthGreenPAK4Pass : public Pass {
	SynthGreenPAK4Pass() : Pass("synth_greenpak4", "synthesis for GreenPAK4 FPGAs") { }
	virtual void help()
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    synth_greenpak4 [options]\n");
		log("\n");
		log("This command runs synthesis for GreenPAK4 FPGAs. This work is experimental.\n");
		log("\n");
		log("    -top <module>\n");
		log("        use the specified module as top module (default='top')\n");
		log("\n");
		log("    -part <part>\n");
		log("        synthesize for the specified part. Valid values are SLG46140V,\n");
		log("        SLG46620V, and SLG46621V (default).\n");
		log("\n");
		log("    -json <file>\n");
		log("        write the design to the specified JSON file. writing of an output file\n");
		log("        is omitted if this parameter is not specified.\n");
		log("\n");
		log("    -run <from_label>:<to_label>\n");
		log("        only run the commands between the labels (see below). an empty\n");
		log("        from label is synonymous to 'begin', and empty to label is\n");
		log("        synonymous to the end of the command list.\n");
		log("\n");
		log("    -noflatten\n");
		log("        do not flatten design before synthesis\n");
		log("\n");
		log("    -retime\n");
		log("        run 'abc' with -dff option\n");
		log("\n");
		log("\n");
		log("The following commands are executed by this synthesis command:\n");
		log("\n");
		log("    begin:\n");
		log("        read_verilog -lib +/greenpak4/cells_sim.v\n");
		log("        hierarchy -check -top <top>\n");
		log("\n");
		log("    flatten:         (unless -noflatten)\n");
		log("        proc\n");
		log("        flatten\n");
		log("        tribuf -logic\n");
		log("\n");
		log("    coarse:\n");
		log("        synth -run coarse\n");
		log("\n");
		log("    fine:\n");
		log("        greenpak4_counters\n");
		log("        clean\n");
		log("        opt -fast -mux_undef -undriven -fine\n");
		log("        memory_map\n");
		log("        opt -undriven -fine\n");
		log("        techmap\n");
		log("        dfflibmap -prepare -liberty +/greenpak4/gp_dff.lib\n");
		log("        opt -fast\n");
		log("        abc -dff     (only if -retime)\n");
		log("\n");
		log("    map_luts:\n");
		log("        nlutmap -luts 0,6,8,2        (for -part SLG46140V)\n");
		log("        nlutmap -luts 0,8,16,2       (for -part SLG46620V)\n");
		log("        nlutmap -luts 0,8,16,2       (for -part SLG46621V)\n");
		log("        clean\n");
		log("\n");
		log("    map_cells:\n");
		log("        dfflibmap -liberty +/greenpak4/gp_dff.lib\n");
		log("        techmap -map +/greenpak4/cells_map.v\n");
		log("        dffinit -ff GP_DFF Q INIT\n");
		log("        dffinit -ff GP_DFFR Q INIT\n");
		log("        dffinit -ff GP_DFFS Q INIT\n");
		log("        dffinit -ff GP_DFFSR Q INIT\n");
		log("        clean\n");
		log("\n");
		log("    check:\n");
		log("        hierarchy -check\n");
		log("        stat\n");
		log("        check -noinit\n");
		log("\n");
		log("    json:\n");
		log("        splitnets                    (temporary workaround for gp4par parser limitation)\n");
		log("        write_json <file-name>\n");
		log("\n");
	}
	virtual void execute(std::vector<std::string> args, RTLIL::Design *design)
	{
		std::string top_opt = "-auto-top";
		std::string part = "SLG46621V";
		std::string run_from, run_to;
		std::string json_file;
		bool flatten = true;
		bool retime = false;

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			if (args[argidx] == "-top" && argidx+1 < args.size()) {
				top_opt = "-top " + args[++argidx];
				continue;
			}
			if (args[argidx] == "-json" && argidx+1 < args.size()) {
				json_file = args[++argidx];
				continue;
			}
			if (args[argidx] == "-part" && argidx+1 < args.size()) {
				part = args[++argidx];
				continue;
			}
			if (args[argidx] == "-run" && argidx+1 < args.size()) {
				size_t pos = args[argidx+1].find(':');
				if (pos == std::string::npos)
					break;
				run_from = args[++argidx].substr(0, pos);
				run_to = args[argidx].substr(pos+1);
				continue;
			}
			if (args[argidx] == "-noflatten") {
				flatten = false;
				continue;
			}
			if (args[argidx] == "-retime") {
				retime = true;
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		if (!design->full_selection())
			log_cmd_error("This comannd only operates on fully selected designs!\n");

		if (part != "SLG46140V" && part != "SLG46620V" && part != "SLG46621V")
			log_cmd_error("Invalid part name: '%s'\n", part.c_str());

		bool active = run_from.empty();

		log_header("Executing SYNTH_GREENPAK4 pass.\n");
		log_push();

		if (check_label(active, run_from, run_to, "begin"))
		{
			Pass::call(design, "read_verilog -lib +/greenpak4/cells_sim.v");
			Pass::call(design, stringf("hierarchy -check %s", top_opt.c_str()));
		}

		if (flatten && check_label(active, run_from, run_to, "flatten"))
		{
			Pass::call(design, "proc");
			Pass::call(design, "flatten");
			Pass::call(design, "tribuf -logic");
		}

		if (check_label(active, run_from, run_to, "coarse"))
		{
			Pass::call(design, "synth -run coarse");
		}

		if (check_label(active, run_from, run_to, "fine"))
		{
			Pass::call(design, "greenpak4_counters");
			Pass::call(design, "clean");
			Pass::call(design, "opt -fast -mux_undef -undriven -fine");
			Pass::call(design, "memory_map");
			Pass::call(design, "opt -undriven -fine");
			Pass::call(design, "techmap");
			Pass::call(design, "dfflibmap -prepare -liberty +/greenpak4/gp_dff.lib");
			Pass::call(design, "opt -fast");
			if (retime)
				Pass::call(design, "abc -dff");
		}

		if (check_label(active, run_from, run_to, "map_luts"))
		{
			if (part == "SLG46140V") Pass::call(design, "nlutmap -luts 0,6,8,2");
			if (part == "SLG46620V") Pass::call(design, "nlutmap -luts 2,8,16,2");
			if (part == "SLG46621V") Pass::call(design, "nlutmap -luts 2,8,16,2");
			Pass::call(design, "clean");
		}

		if (check_label(active, run_from, run_to, "map_cells"))
		{
			Pass::call(design, "dfflibmap -liberty +/greenpak4/gp_dff.lib");
			Pass::call(design, "techmap -map +/greenpak4/cells_map.v");
			Pass::call(design, "dffinit -ff GP_DFF Q INIT");
			Pass::call(design, "dffinit -ff GP_DFFR Q INIT");
			Pass::call(design, "dffinit -ff GP_DFFS Q INIT");
			Pass::call(design, "dffinit -ff GP_DFFSR Q INIT");
			Pass::call(design, "clean");
		}

		if (check_label(active, run_from, run_to, "check"))
		{
			Pass::call(design, "hierarchy -check");
			Pass::call(design, "stat");
			Pass::call(design, "check -noinit");
		}

		if (check_label(active, run_from, run_to, "json"))
		{
			Pass::call(design, "splitnets");
			if (!json_file.empty())
				Pass::call(design, stringf("write_json %s", json_file.c_str()));
		}

		log_pop();
	}
} SynthGreenPAK4Pass;

PRIVATE_NAMESPACE_END
