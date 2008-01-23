/* $Id: config.c,v 1.1.1.1 2006/03/17 19:55:48 zicodxx Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * contains routine(s) to read in the configuration file which contains
 * game configuration stuff like detail level, sound card, etc
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "pstypes.h"
#include "game.h"
#include "menu.h"
#include "movie.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "joy.h"
#include "songs.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "mono.h"
#include "physfsx.h"


ubyte Config_digi_volume = 8;
ubyte Config_midi_volume = 8;
ubyte Config_redbook_volume = 8;
ubyte Config_control_type = 0;
ubyte Config_channels_reversed = 0;
ubyte Config_joystick_sensitivity = 8;

static char *DigiVolumeStr = "DigiVolume";
static char *MidiVolumeStr = "MidiVolume";
static char *RedbookVolumeStr = "RedbookVolume";
static char *ReverseStereoStr = "ReverseStereo";
static char *DetailLevelStr = "DetailLevel";
static char *GammaLevelStr = "GammaLevel";
static char *LastPlayerStr = "LastPlayer";
static char *LastMissionStr = "LastMission";
static char *ResolutionXStr="ResolutionX";
static char *ResolutionYStr="ResolutionY";
static int Config_render_width=0, Config_render_height=0;

char config_last_player[CALLSIGN_LEN+1] = "";
char config_last_mission[MISSION_NAME_LEN+1] = "";

int Config_digi_type = 0;
int Config_digi_dma = 0;
int Config_midi_type = 0;

extern sbyte Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;

void set_custom_detail_vars(void);

#define CL_MC0 0xF8F
#define CL_MC1 0xF8D

int ReadConfigFile()
{
	PHYSFS_file *infile;
	char line[80], *token, *value, *ptr;
	ubyte gamma;

	strcpy( config_last_player, "" );

	Config_digi_volume = 8;
	Config_midi_volume = 8;
	Config_redbook_volume = 8;
	Config_control_type = 0;

	infile = PHYSFSX_openReadBuffered("descent.cfg");

	if (infile == NULL) {
		return 1;
	}

	while (!PHYSFS_eof(infile))
	{
		memset(line, 0, 80);
		PHYSFSX_gets(infile, line);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!value)
				value = "";
			if (!strcmp(token, DigiVolumeStr))
				Config_digi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, MidiVolumeStr))
				Config_midi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, RedbookVolumeStr))
				Config_redbook_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, ReverseStereoStr))
				Config_channels_reversed = strtol(value, NULL, 10);
			else if (!strcmp(token, GammaLevelStr)) {
				gamma = strtol(value, NULL, 10);
				gr_palette_set_gamma( gamma );
			}
			else if (!strcmp(token, DetailLevelStr)) {
				Detail_level = strtol(value, NULL, 10);
				if (Detail_level == NUM_DETAIL_LEVELS-1) {
					int count,dummy,oc,od,wd,wrd,da,sc;

					count = sscanf (value, "%d,%d,%d,%d,%d,%d,%d\n",&dummy,&oc,&od,&wd,&wrd,&da,&sc);

					if (count == 7) {
						Object_complexity = oc;
						Object_detail = od;
						Wall_detail = wd;
						Wall_render_depth = wrd;
						Debris_amount = da;
						SoundChannels = sc;
						set_custom_detail_vars();
					}
				}
			}
			else if (!strcmp(token, LastPlayerStr))	{
				char * p;
				strncpy( config_last_player, value, CALLSIGN_LEN );
				p = strchr( config_last_player, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, LastMissionStr))	{
				char * p;
				strncpy( config_last_mission, value, MISSION_NAME_LEN );
				p = strchr( config_last_mission, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, ResolutionXStr))
				Config_render_width = strtol(value, NULL, 10);
			else if (!strcmp(token, ResolutionYStr))
				Config_render_height = strtol(value, NULL, 10);
		}
	}

	PHYSFS_close(infile);

	if ( Config_digi_volume > 8 ) Config_digi_volume = 8;
	if ( Config_midi_volume > 8 ) Config_midi_volume = 8;
	if ( Config_redbook_volume > 8 ) Config_redbook_volume = 8;

	digi_set_volume( (Config_digi_volume*32768)/8, (Config_midi_volume*128)/8 );

	if (Config_render_width >= 320 && Config_render_height >= 200)
		Game_screen_mode = SM(Config_render_width,Config_render_height);

	return 0;
}

int WriteConfigFile()
{
	PHYSFS_file *infile;
	ubyte gamma = gr_palette_get_gamma();
	
	infile = PHYSFSX_openWriteBuffered("descent.cfg");

	if (infile == NULL) {
		return 1;
	}

	PHYSFSX_printf(infile,"%s=%d\n", DigiVolumeStr, Config_digi_volume);
	PHYSFSX_printf(infile,"%s=%d\n", MidiVolumeStr, Config_midi_volume);
	PHYSFSX_printf(infile,"%s=%d\n", RedbookVolumeStr, Config_redbook_volume);
	PHYSFSX_printf(infile,"%s=%d\n", ReverseStereoStr, Config_channels_reversed);
	PHYSFSX_printf(infile,"%s=%d\n", GammaLevelStr, gamma);
	if (Detail_level == NUM_DETAIL_LEVELS-1)
		PHYSFSX_printf(infile,"%s=%d,%d,%d,%d,%d,%d,%d\n", DetailLevelStr, Detail_level,
				Object_complexity,Object_detail,Wall_detail,Wall_render_depth,Debris_amount,SoundChannels);
	else
		PHYSFSX_printf(infile,"%s=%d\n", DetailLevelStr, Detail_level);
	PHYSFSX_printf(infile,"%s=%s\n", LastPlayerStr, Players[Player_num].callsign );
	PHYSFSX_printf(infile,"%s=%s\n", LastMissionStr, config_last_mission );
	PHYSFSX_printf(infile,"%s=%i\n", ResolutionXStr, SM_W(Game_screen_mode));
	PHYSFSX_printf(infile,"%s=%i\n", ResolutionYStr, SM_H(Game_screen_mode));

	PHYSFS_close(infile);

	return 0;
}
