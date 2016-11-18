#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

char* extractMapName(char line[]);
void stripBadChars(char string[]);

enum mods {
	MOD_DM,
	MOD_CTF,
	MOD_TF
};

const char* mod_names[] = { "dm", "ctf", "tf" };

/*
Replaces the dot of the filename with NULL
and returns the new, file-extension-less string.
 */
char *stripFileExtension(char* filename) 
{
	char *retstr;
	char *lastdot;

	if (filename == NULL)
		return NULL;

	if ((retstr = malloc(strlen(filename) + 1)) == NULL)
		return NULL;

	strcpy(retstr,filename);
	lastdot = strrchr(retstr, '.');
	if (lastdot != NULL)
		*lastdot = '\0';

	return retstr;
}

char* extractMapName(char line[]) 
{
	char seps[] = "\"";
	char *token;
	char *temp;
	int fieldNo = 0;

	token = strtok(line, seps);

	while (token != NULL) {
		if (fieldNo == 2) {
			if (token != NULL) {
				return strdup(token);
			}
		}

		token = strtok(NULL, seps);
		fieldNo++;
	}	

	return 0;
}

void stripBadChars(char string[]) 
{
	int i, j;
	char temp[strlen(string) + 1];
	int lastChar;

	j = 0;
	for (i = 0; i < strlen(string); i++) {
		//printf("Reading: %c, lastchar: %c\n", string[i], lastChar);
		if (	(string[i] >= '0' && string[i] <= '9')
			||	(string[i] >= 'a' && string[i] <= 'z')
			|| 	(string[i] >= 'A' && string[i] <= 'Z')
			|| 	(string[i] == ' ') 
			|| 	(string[i] == '\\') 
			) {
				// Todo: get rid of the remaining \ ?
				if (!(lastChar == '\\' && string[i] == 'n')) {
					temp[j++] = string[i];
				}
		} else {
			
			temp[j++] = ' ';
		}

		lastChar = string[i];
	}

	// It's a string, so we null-terminate
	temp[j] = '\0';
	
	strcpy(string, temp);
}

int main(int argc, char **argv) 
{
	// File handling vars 
	DIR* FD;
	struct dirent* in_file;
	FILE *common_file;
	FILE *entry_file;
	char buffer[BUFSIZ];

	char *mapName;
	char *map;

	// DM is standard MOD if nothing else found
	int mod = MOD_DM;

	// Entity counters
	int numSSG	= 0;
	int numNG	= 0;
	int numSNG	= 0;
	int numGL	= 0;
	int numRL	= 0;
	int numLG	= 0;
	int numShellsSmall 	= 0;
	int numShellsBig 	= 0;
	int numNailsSmall	= 0;
	int numNailsBig		= 0;
	int numCellsSmall	= 0;
	int numCellsBig		= 0;
	int numRocketsSmall	= 0;
	int numRocketsBig	= 0;

	int numHealthSmall	= 0;
	int numHealthBig	= 0;
	int numMegaHealth	= 0;

	int numGA			= 0;
	int numYA			= 0;
	int numRA			= 0;

	int numQuads		= 0;
	int numRings		= 0;
	int numPents		= 0;
	int numEnviroSuits	= 0;

	int numSpawns		= 0;
	int numTeleports	= 0;
	int numSecrets 		= 0;
	int numSecretDoors	= 0;

	int mapCount 		= 0;

	const char STRING_MESSAGE[9]			= "\"message\"";

	// Entity strings to match
	const char STRING_WEAPON_SSG[33] 		= "\"classname\" \"weapon_supershotgun\"";
	const char STRING_WEAPON_NG[28] 		= "\"classname\" \"weapon_nailgun\"";
	const char STRING_WEAPON_SNG[33] 		= "\"classname\" \"weapon_supernailgun\"";
	const char STRING_WEAPON_GL[36] 		= "\"classname\" \"weapon_grenadelauncher\"";
	const char STRING_WEAPON_RL[35] 		= "\"classname\" \"weapon_rocketlauncher\"";
	const char STRING_WEAPON_LG[30] 		= "\"classname\" \"weapon_lightning\"";
	
	const char STRING_ITEM_AMMO_SHELLS[25]	= "\"classname\" \"item_shells\"";				// Spawnflags 0 = small pack, 	1 = big pack
	const char STRING_ITEM_AMMO_NAILS[25]	= "\"classname\" \"item_spikes\"";				// Spawnflags 0 = big pack, 	1 = small pack
	const char STRING_ITEM_AMMO_CELLS[24]	= "\"classname\" \"item_cells\"";				// Spawnflags 0 = small pack, 	1 = big pack
	const char STRING_ITEM_AMMO_RL[26]		= "\"classname\" \"item_rockets\"";				// Spawnflags 0 = small pack, 	1 = big pack

	const char STRING_ITEM_HEALTH[25]		= "\"classname\" \"item_health\"";				// Spawnflags 0 = +25, 			1 = +15, 		2 = +100 (megahealth)

	const char STRING_ARMOR_GA[25] 			= "\"classname\" \"item_armor1\"";
	const char STRING_ARMOR_YA[25] 			= "\"classname\" \"item_armor2\"";
	const char STRING_ARMOR_RA[27] 			= "\"classname\" \"item_armorInv\"";	

	const char STRING_POWERUP_QUAD[40]		= "\"classname\" \"item_artifact_super_damage\"";
	const char STRING_POWERUP_RING[40]		= "\"classname\" \"item_artifact_invisibility\"";
	const char STRING_POWERUP_PENT[43]		= "\"classname\" \"item_artifact_invulnerability\"";
	const char STRING_ENVIRO_SUIT[38]		= "\"classname\" \"item_artifact_envirosuit\"";

	const char STRING_DM_SPAWN[36]			= "\"classname\" \"info_player_deathmatch\"";
	const char STRING_TELEPORT[30]			= "\"classname\" \"trigger_teleport\"";
	const char STRING_SECRET[28]			= "\"classname\" \"trigger_secret\"";
	const char STRING_SECRET_DOOR[30]		= "\"classname\" \"func_door_secret\"";

	// Spawn flags - modifiers that decides size of ammo boxes etc
	const char STRING_SPAWNFLAG_1[19]		= "\"spawnflags\" \"1\"";
	const char STRING_SPAWNFLAG_2[19]		= "\"spawnflags\" \"2\"";
	
	const char STRING_MOD_CTF[29]	=	"\"classname\" \"item_flag_team1\"";
	const char STRING_MOD_TF[27]	=	"\"classname\" \"info_tfdetect\"";

	// Used to check file extensions
	char *fileExtension;
	const char dot = '.';

	common_file = fopen("map_entities.csv", "w");

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);

		return 1;
	}

	if (common_file == NULL) {
		fprintf(stderr, "Error: Failed to open common_file - %s\n", strerror(errno));

		return 1;
	}

	if (NULL == (FD = opendir (argv[1]))) {
		fprintf(stderr, "Error: Failed to open input directory (%s) - %s\n", argv[1], strerror(errno));
		fclose(common_file);

		return 1;
	}

	// Print headers in output file
	fprintf(common_file, "Map|Map description|mod|numSSG|numNG|numSNG|numGL|numRL|numLG|numShellsSmall|numShellsBig|numNailsSmall|numNailsBig|numCellsSmall|numCellsBig|numRocketsSmall|numRocketsBig|numHealthSmall|numHealthBig|numMegaHealth|numGA|numYA|numRA|numQuads|numRings|numPents|numEnviroSuits|numSpawns|numTeleports|numSecrets|numSecretDoors\n");

	while ((in_file = readdir(FD))) {
		// Skip unix folder names
		if (!strcmp(in_file->d_name, "."))
			continue;
		if (!strcmp(in_file->d_name, ".."))
			continue;

		fileExtension = strrchr(in_file->d_name, dot);
		
		// Only look for bsp files
		if (!fileExtension || strcmp(fileExtension, ".bsp"))
			continue;

		char *fullpath = malloc(strlen(argv[1]) + strlen(in_file->d_name) + 2);
		sprintf(fullpath, "%s/%s", argv[1], in_file->d_name);

		entry_file = fopen(fullpath, "rw");
		if (entry_file == NULL) {
			fprintf(stderr, "Error : Failed to open entry file (%s) - %s\n", in_file->d_name, strerror(errno));
			fclose(common_file);

			return 1;
		}

		// If we ended up here we managed to open the file
		mapCount++;
		printf("Reading: %s\n", fullpath);

		// Need to keep track of these as we can't increment
		// until we found out what spawnflag they have.
		bool curItemIsShells 	= 0;
		bool curItemIsNails		= 0;
		bool curItemIsCells		= 0;
		bool curItemIsRockets 	= 0;
		bool curItemIsHealth	= 0;
		
		int curSpawnflag = 0;

		// 
		bool foundOpenTag = 0;

		// fgets reads a line at a time and places result in buffer
		while (fgets(buffer, BUFSIZ, entry_file) != NULL) {

			// Check for map name
			if (mapName == NULL && (strncmp(buffer, STRING_MESSAGE, 9) == 0)) {
				mapName = extractMapName(buffer);

				stripBadChars(mapName);
			}

			// We found an entity opening tag
			if (strncmp(buffer, "{", 1) == 0) {
				foundOpenTag = 1;

			// We found an entity closening tag,
			// increment what we have found
			} else if ((strncmp(buffer, "}", 1) == 0) && foundOpenTag == 1) {
				
				// Shells
				if (curItemIsShells) {
					
					if (curSpawnflag == 0) {
						numShellsSmall++;
					} else {
						numShellsBig++;
					}
					curItemIsShells = 0;
				}

				// Nails
				if (curItemIsNails) {
					if (curSpawnflag == 0) {
						numNailsBig++;
					} else {
						numNailsSmall++;
					}
					curItemIsNails = 0;
				}

				// Rockets
				if (curItemIsRockets) {
					if (curSpawnflag == 0) {
						numRocketsSmall++;
					} else {
						numRocketsBig++;
					}
					curItemIsRockets = 0;
				}

				// Cells
				if (curItemIsCells) {
					if (curSpawnflag == 0) {
						numCellsSmall++;
					} else {
						numCellsBig++;
					}
					curItemIsCells = 0;
				}

				// Health (incl. mega)
				if (curItemIsHealth) {
					if (curSpawnflag == 0) {
						numHealthBig++;
					} else if (curSpawnflag == 1) {
						numHealthSmall++;
					} else {
						numMegaHealth++;
					}
					curItemIsHealth = 0;
				}

				curSpawnflag = 0;
				foundOpenTag = 0;
			} else {
				
				// Weapons, armors, powerups and spawns are straight forward, just match and increment
				if (strstr(buffer, STRING_WEAPON_SSG)) 	{ numSSG++; }
				if (strstr(buffer, STRING_WEAPON_NG)) 	{ numNG++; }
				if (strstr(buffer, STRING_WEAPON_SNG)) 	{ numSNG++; }
				if (strstr(buffer, STRING_WEAPON_GL)) 	{ numGL++; }
				if (strstr(buffer, STRING_WEAPON_RL)) 	{ numRL++; }
				if (strstr(buffer, STRING_WEAPON_LG)) 	{ numLG++; }

				if (strstr(buffer, STRING_ARMOR_GA)) 	{ numGA++; }
				if (strstr(buffer, STRING_ARMOR_YA)) 	{ numYA++; }
				if (strstr(buffer, STRING_ARMOR_RA)) 	{ numRA++; }

				if (strstr(buffer, STRING_POWERUP_QUAD)) 	{ numQuads++; }
				if (strstr(buffer, STRING_POWERUP_RING)) 	{ numRings++; }
				if (strstr(buffer, STRING_POWERUP_PENT)) 	{ numPents++; }

				if (strstr(buffer, STRING_ENVIRO_SUIT)) 	{ numEnviroSuits++; }
				
				if (strstr(buffer, STRING_DM_SPAWN)) 	{ numSpawns++; }
				if (strstr(buffer, STRING_TELEPORT)) 	{ numTeleports++; }
				if (strstr(buffer, STRING_SECRET)) 		{ numSecrets++; }
				if (strstr(buffer, STRING_SECRET_DOOR)) { numSecretDoors++; }
				
				// Some item has spawn flags that we need to read before
				// we decide the size of the ammo/health pack. So for now
				// we just remember that previous line was such a pack
				// and check if next line increases the size.
				if (strstr(buffer, STRING_ITEM_AMMO_SHELLS)) 	{ curItemIsShells = 1; }
				if (strstr(buffer, STRING_ITEM_AMMO_NAILS)) 	{ curItemIsNails = 1; }
				if (strstr(buffer, STRING_ITEM_AMMO_CELLS)) 	{ curItemIsCells = 1; }
				if (strstr(buffer, STRING_ITEM_AMMO_RL)) 		{ curItemIsRockets = 1; }
				if (strstr(buffer, STRING_ITEM_HEALTH)) 		{ curItemIsHealth = 1; }

				if (strstr(buffer, STRING_SPAWNFLAG_1)) 		{ curSpawnflag = 1; }
				if (strstr(buffer, STRING_SPAWNFLAG_2)) 		{ curSpawnflag = 2; }

				// Default mod is 0 = DM
				// If we already found the mod to be TF we leave it at that...
				if (mod != MOD_TF) {
					if (strstr(buffer, STRING_MOD_TF)) 	{ mod = MOD_TF; }
					if (strstr(buffer, STRING_MOD_CTF)) { mod = MOD_CTF; }
				}
			}
			
		}

		map = stripFileExtension(in_file->d_name);

		fprintf(common_file, "%s|%s|%s|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i|%i\n", 
			map,
			mapName,
			mod_names[mod], 
			numSSG, 
			numNG, 
			numSNG, 
			numGL, 
			numRL, 
			numLG, 
			numShellsSmall, 
			numShellsBig, 
			numNailsSmall, 
			numNailsBig, 
			numCellsSmall, 
			numCellsBig, 
			numRocketsSmall, 
			numRocketsBig,
			numHealthSmall,
			numHealthBig,
			numMegaHealth,
			numGA,
			numYA,
			numRA,
			numQuads,
			numRings,
			numPents,
			numEnviroSuits,
			numSpawns,
			numTeleports,
			numSecrets,
			numSecretDoors
			);

		free(fullpath);
		fclose(entry_file);

		// Reset counters for next map file
		free(map);
		free(mapName);
		mapName = NULL;
		mod = numSSG = numNG = numSNG = numGL = numRL = numLG = numShellsSmall = numShellsBig = numNailsSmall = numNailsBig = numCellsSmall = numCellsBig = numRocketsSmall = numRocketsBig = numHealthSmall = numHealthBig = numMegaHealth = numGA = numYA = numRA = numQuads = numRings = numPents = numEnviroSuits = numSpawns = numTeleports = numSecrets = numSecretDoors = 0;
	}

	fclose(common_file);
	printf("Finished reading %i files.\n", mapCount);
}

