#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <brrtools/brrtil.h>

#define _init_fcc(_a_, _b_, _c_, _d_) {.bytes={(_a_), (_b_), (_c_), (_d_)}}
#define _fcc_lit(_l_) _init_fcc((_l_)[0], (_l_)[1], (_l_)[2], (_l_)[3])
#define MAKE_FCC(_l_) _fcc_lit(#_l_)
#define GET_FCC_BYTES(_f_) (_f_).bytes._0, (_f_).bytes._1, (_f_).bytes._2, (_f_).bytes._3

const fourccT goggfcc = MAKE_FCC(OggS);
const fourccT gwemfcc = MAKE_FCC(RIFF);
const fourccT gbnkfcc = MAKE_FCC(BKHD);

void BRRCALL
input_delete(inputT *const input)
{
	if (input) {
		brrstg_delete(&input->input);
		input->options = (input_optionsT){0};
	}
}
void BRRCALL
input_print(brrlog_priorityT priority, int newline, const inputT *const input, brrsz max_input_length)
{
	gbrrlog_level_last = gbrrlog_level_debug;
	gbrrlog_format_last = gbrrlog_format_normal;
	BRRLOG_LASTNP(" ");
	if (input->options.type == INPUT_TYPE_OGG)      { BRRLOG_FORENP(brrlog_color_blue, "OGG"); }
	else if (input->options.type == INPUT_TYPE_WEM) { BRRLOG_FORENP(brrlog_color_green, "WEM"); }
	else if (input->options.type == INPUT_TYPE_WSP) { BRRLOG_FORENP(brrlog_color_yellow, "WSP"); }
	else if (input->options.type == INPUT_TYPE_BNK) { BRRLOG_FORENP(brrlog_color_red, "BNK"); }
	else                                            { BRRLOG_STYLENP(brrlog_color_magenta, -1, brrlog_style_bold, "AUT"); }

	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " log ");
	if (input->options.log_enabled) { BRRLOG_STYLENP(brrlog_color_green, -1, brrlog_style_bold, "ENB"); }
	else { BRRLOG_FORENP(brrlog_color_red, "DSB"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_color_enabled) { BRRLOG_STYLENP(brrlog_color_blue, -1, brrlog_style_bold, "STY"); }
	else { BRRLOG_FORENP(brrlog_color_red, "SMP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.log_debug) { BRRLOG_FORENP(brrlog_color_cyan, "DBG"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "NRM"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	BRRLOG_FORENP(brrlog_color_normal + 1 + input->options.log_priority,
	    "%s", brrlog_priority_dbgstr(input->options.log_priority));
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " operation ");
	if (input->options.dry_run) { BRRLOG_FORENP(brrlog_color_yellow, "DRY"); }
	else { BRRLOG_FORENP(brrlog_color_blue, "WET"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " w2o ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(brrlog_color_magenta, "AUT"); }
	else { BRRLOG_FORENP(brrlog_color_red, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(brrlog_color_cyan, "INP"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " rvb ");
	if (input->options.auto_ogg) { BRRLOG_FORENP(brrlog_color_magenta, "AUT"); }
	else { BRRLOG_FORENP(brrlog_color_red, "MAN"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " ");
	if (input->options.inplace_ogg) { BRRLOG_FORENP(brrlog_color_cyan, "INP"); }
	else { BRRLOG_FORENP(brrlog_color_yellow, "SEP"); }
	BRRLOG_FORENP(gbrrlog_format_normal.foreground, " bank recurse ");
	if (input->options.bank_recurse) { BRRLOG_FORENP(brrlog_color_green, "FUL"); }
	else { BRRLOG_FORENP(brrlog_color_red, "NON"); }
	if (newline)
		BRRLOG_LASTP("");
}

inputT *BRRCALL
find_argument(const char *const arg, const inputT *const inputs, brrsz input_count)
{
	if (!inputs || !input_count || !arg) {
		return NULL;
	} else {
		for (brrsz i = 0; i < input_count; ++i) {
			if (0 == strcmp(inputs[i].input.opaque, arg))
				return (inputT *)&inputs[i];
		}
		return NULL;
	}
}
int BRRCALL
parse_argument(void (*const print_help)(void),
    const char *const arg, int *const reset,
    input_optionsT *const options, inputT *const inputs, brrsz input_count,
    const input_optionsT *const default_options)
{
	inputT *temp = NULL;
	if (-1 != brrstg_cstr_compare(arg, 0, "-h", "-help", "--help", "-v", "-version", "--version", NULL)) {
		print_help();
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-a", "-auto", "-detect", NULL)) {
		options->type = INPUT_TYPE_UNK; return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-o", "-ogg", NULL)) {
		options->type = INPUT_TYPE_OGG; return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-w", "-wem", "-weem", NULL)) {
		options->type = INPUT_TYPE_WEM; return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-W", "-wsp", "-wisp", NULL)) {
		options->type = INPUT_TYPE_WSP; return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-b", "-bnk", "-bank", NULL)) {
		options->type = INPUT_TYPE_BNK; return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-R", "-recurse-bank", NULL)) {
		BRRTIL_TOGGLE(options->bank_recurse); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-O", "-weem2ogg", NULL)) {
		BRRTIL_TOGGLE(options->auto_ogg); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-oi", "-ogg-inplace", NULL)) {
		BRRTIL_TOGGLE(options->inplace_ogg); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-r", "-revorb", NULL)) {
		BRRTIL_TOGGLE(options->auto_revorb); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-ri", "-rvb-inplace", NULL)) {
		BRRTIL_TOGGLE(options->inplace_revorb); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-Q", "-qq", "-too-quiet", NULL)) {
		BRRTIL_TOGGLE(options->log_enabled); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-c", "-color", NULL)) {
		BRRTIL_TOGGLE(options->log_color_enabled); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-d", "-debug", NULL)) {
		BRRTIL_TOGGLE(options->log_debug);
		if (options->log_debug)
			options->log_enabled = 1;
		return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-q", "-quiet", NULL)) {
		if (options->log_priority > 0)
			options->log_priority--;
		if (options->log_priority == 0 && !options->log_debug)
			options->log_enabled = 0;
		return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "+q", "+quiet", NULL)) {
		if (options->log_priority < brrlog_priority_count - 1)
			options->log_priority++;
		options->log_enabled = 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-n", "-dry", "-dryrun", NULL)) {
		BRRTIL_TOGGLE(options->dry_run); return 1;
	} else if (-1 != brrstg_cstr_compare(arg, 1, "-reset", NULL)) {
		BRRTIL_TOGGLE(*reset); return 1;
	} else if ((temp = find_argument(arg, inputs, input_count))) {
		temp->options = *options; *options = *default_options; return 1;
	}
	return 0;
}

int BRRCALL
determine_type(inputT *const input, const brrpath_infoT *const input_info)
{
	FILE *input_fp = NULL;
	fourccT input_fcc = {0};
	int err = 0;
	if (!(input_fp = fopen((char *)input->input.opaque, "rb"))) {
		return TYPE_ERR_INPUT;
	}
	if (fread(&input_fcc.integer, 1, 4, input_fp) != 4) {
		err = TYPE_ERR_READ;
	}
	fclose(input_fp);
	if (!err) {
		BRRLOG_DEBUGNP(" FCC %08X = %02X %02X %02X %02X", input_fcc, GET_FCC_BYTES(input_fcc));
	}
	if (input_fcc.integer == goggfcc.integer) {
		input->options.type = INPUT_TYPE_OGG;
	} else if (input_fcc.integer == gwemfcc.integer) {
		if (!brrstg_cstr_compare(brrstg_raw(&input_info->extension), 0, "wem", NULL)) {
			input->options.type = INPUT_TYPE_WEM;
		} else if (!brrstg_cstr_compare(brrstg_raw(&input_info->extension), 0, "wsp", NULL)) {
			input->options.type = INPUT_TYPE_WSP;
		} else {
			return TYPE_ERR_TYPE;
		}
	} else if (input_fcc.integer == gbnkfcc.integer) {
		input->options.type = INPUT_TYPE_BNK;
	} else {
		err = TYPE_ERR_TYPE;
	}
	return err;
}
