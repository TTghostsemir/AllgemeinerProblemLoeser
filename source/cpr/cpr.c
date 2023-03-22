// #include <utils/util.h>
// #include "tools.h"
#include <storage/sd.h>
#include "../fs/readers/folderReader.h"
#include "../fs/fstypes.h"
#include "../fs/fscopy.h"
#include <utils/sprintf.h>
#include <stdlib.h>
#include <string.h>
#include "../gfx/gfx.h"
#include "../gfx/gfxutils.h"
#include "../gfx/menu.h"
#include "../hid/hid.h"
// #include "utils.h"
#include "../utils/utils.h"
#include "../fs/fsutils.h"

#include <unistd.h>
#include <sys/types.h>
// #include <dirent.h>
// #include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void _DeleteFileSimple(char *thing){
    //char *thing = CombinePaths(path, entry.name);
    int res = f_unlink(thing);
    if (res)
        DrawError(newErrCode(res));
    free(thing);
}
void _RenameFileSimple(char *sourcePath, char *destPath){
    int res = f_rename(sourcePath, destPath);
    if (res){
        DrawError(newErrCode(res));
    }
}
ErrCode_t _FolderDelete(const char *path){
    int res = 0;
    ErrCode_t ret = newErrCode(0);
    u32 x, y;
    gfx_con_getpos(&x, &y);
    Vector_t fileVec = ReadFolder(path, &res);
    if (res){
        ret = newErrCode(res);
    }
    else {
        vecDefArray(FSEntry_t *, fs, fileVec);
        for (int i = 0; i < fileVec.count && !ret.err; i++){
            char *temp = CombinePaths(path, fs[i].name);
            if (fs[i].isDir){
                ret = _FolderDelete(temp);
            }
            else {
                res = f_unlink(temp);
                if (res){
                    ret = newErrCode(res);
                }             
            }
            free(temp);
        }
    }
    if (!ret.err){
        res = f_unlink(path);
        if (res)
            ret = newErrCode(res);
    }
    clearFileVector(&fileVec);
    return ret;
}
int _StartsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

int listdir(char *path, u32 hos_folder)
{
   	FRESULT res;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

    // Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

    dirLength = strlen(path);
	for (;;)
	{
		// Clear file or folder path.
		path[dirLength] = 0;
		// Read a directory item.
		res = f_readdir(&dir, &fno);
		// Break on error or end of dir.
		if (res != FR_OK || fno.fname[0] == 0)
			break;
		// Skip official Nintendo dir if started from root.
		if (!hos_folder && !strcmp(fno.fname, "Nintendo"))
			continue;

		// // Set new directory or file.
		memcpy(&path[dirLength], "/", 1);
		memcpy(&path[dirLength + 1], fno.fname, strlen(fno.fname) + 1);
        // gfx_printf("THING: %s\n", fno.fname);
        // gfx_printf("Path: %s\n", dir);
        // Is it a directory?
		if (fno.fattrib & AM_DIR)
		{
            if (
                strcmp(fno.fname, ".Trash") == 0 ||
                strcmp(fno.fname, ".Trashes") == 0 ||
                strcmp(fno.fname, ".DS_Store") == 0 ||
                strcmp(fno.fname, ".Spotlight-V100")  == 0 ||
                strcmp(fno.fname, ".apDisk")  == 0 ||
                strcmp(fno.fname, ".VolumeIcon.icns")  == 0 ||
                strcmp(fno.fname, ".fseventsd")  == 0 ||
                strcmp(fno.fname, ".TemporaryItems")  == 0
            ) {
                _FolderDelete(path);
            }

			// Enter the directory.
			listdir(path, 0);
			if (res != FR_OK)
				break;
		} else {
            if (
                strcmp(fno.fname, ".DS_Store") == 0 ||
                strcmp(fno.fname, ".Spotlight-V100")  == 0 ||
                strcmp(fno.fname, ".apDisk")  == 0 ||
                strcmp(fno.fname, ".VolumeIcon.icns")  == 0 ||
                strcmp(fno.fname, ".fseventsd")  == 0 ||
                strcmp(fno.fname, ".TemporaryItems")  == 0 ||
                _StartsWith(fno.fname, "._")
            ) {
                _DeleteFileSimple(path);
            }
        }
	}
    f_closedir(&dir);
    free(path);
	return res;
}

int _fix_attributes(char *path, u32 *total, u32 hos_folder, u32 check_first_run){
	FRESULT res;
	DIR dir;
	u32 dirLength = 0;
	static FILINFO fno;

	if (check_first_run)
	{
		// Read file attributes.
		res = f_stat(path, &fno);
		if (res != FR_OK)
			return res;

		// Check if archive bit is set.
		if (fno.fattrib & AM_ARC)
		{
			*(u32 *)total = *(u32 *)total + 1;
			f_chmod(path, 0, AM_ARC);
		}
	}

	// Open directory.
	res = f_opendir(&dir, path);
	if (res != FR_OK)
		return res;

	dirLength = strlen(path);
	for (;;)
	{
		// Clear file or folder path.
		path[dirLength] = 0;

		// Read a directory item.
		res = f_readdir(&dir, &fno);

		// Break on error or end of dir.
		if (res != FR_OK || fno.fname[0] == 0)
			break;

		// Skip official Nintendo dir if started from root.
		if (!hos_folder && !strcmp(fno.fname, "Nintendo"))
			continue;

		// Set new directory or file.
		memcpy(&path[dirLength], "/", 1);
		memcpy(&path[dirLength + 1], fno.fname, strlen(fno.fname) + 1);

		// Check if archive bit is set.
		if (fno.fattrib & AM_ARC)
		{
			*total = *total + 1;
			f_chmod(path, 0, AM_ARC);
		}

		// Is it a directory?
		if (fno.fattrib & AM_DIR)
		{
			// Set archive bit to NCA folders.
			if (hos_folder && !strcmp(fno.fname + strlen(fno.fname) - 4, ".nca"))
			{
				*total = *total + 1;
				f_chmod(path, AM_ARC, AM_ARC);
			}

			// Enter the directory.
			res = _fix_attributes(path, total, hos_folder, 0);
			if (res != FR_OK)
				break;
		}
	}

	f_closedir(&dir);

	return res;
}


void m_entry_fixArchiveBit(u32 type){
    gfx_clearscreen();
    gfx_printf("\n\n-- Behebe Archive Bits\n\n");

    char path[256];
	char label[16];

	u32 total = 0;
	if (sd_mount())
	{
		switch (type)
		{
		case 0:
			strcpy(path, "/");
			strcpy(label, "SD Card");
			break;
		case 1:
		default:
			strcpy(path, "/Nintendo");
			strcpy(label, "Nintendo folder");
			break;
		}

		gfx_printf("Traversing all %s files!\nDies kann einige Zeit beanspruchen...\n\n", label);
		_fix_attributes(path, &total, type, type);
		gfx_printf("%kAnzahl der Behobenden Archiv bits: %d!%k", 0xFF96FF00, total, 0xFFCCCCCC);
		
        gfx_printf("\n\n Fertig, Druecke eine Taste um Fortzufahren");
        hidWait();
	}
}


void m_entry_fixAIOUpdate(){
    gfx_clearscreen();
    gfx_printf("\n\n-- Behebe Kaputtes Switch-AiO-Updater update.\n\n");

    char *aio_fs_path = CpyStr("sd:/atmosphere/fusee-secondary.bin.aio");
    char *aio_p_path = CpyStr("sd:/sept/payload.bin.aio");
    char *aio_strt_path = CpyStr("sd:/atmosphere/stratosphere.romfs.aio");

    char *o_fs_path = CpyStr("sd:/atmosphere/fusee-secondary.bin");
    char *o_p_path = CpyStr("sd:/sept/payload.bin");
    char *o_strt_path = CpyStr("sd:/atmosphere/stratosphere.romfs");

    if (FileExists(aio_fs_path)) {
        gfx_printf("Erkannte aio aktualisierte fusee-secondary Datei -> ersetzt das Original\n\n");
        if (FileExists(o_fs_path)) {
            _DeleteFileSimple(o_fs_path);
        }
        _RenameFileSimple(aio_fs_path, o_fs_path);
    }
    free(aio_fs_path);
    free(o_fs_path);

    if (FileExists(aio_p_path)) {
        gfx_printf("Erkannte aio aktualisierte Payload-Datei -> ersetzt das Original\n\n");
        if (FileExists(o_p_path)) {
            _DeleteFileSimple(o_p_path);
        }
        _RenameFileSimple(aio_p_path, o_p_path);
    }
    free(aio_p_path);
    free(o_p_path);

    if (FileExists(aio_strt_path)) {
        gfx_printf("Erkannte aio aktualisierte Stratosphere datei -> ersetzt das Original\n\n");
        if (FileExists(o_strt_path)) {
            _DeleteFileSimple(o_strt_path);
        }
        _RenameFileSimple(aio_strt_path, o_strt_path);
    }
    free(aio_strt_path);
    free(o_strt_path);


    gfx_printf("\n\n Fertig, druecke eine Taste um Fortzufahren.");
    hidWait();
}

void m_entry_fixClingWrap(){
    gfx_clearscreen();
    gfx_printf("\n\n-- Behebe SigPatches.\n\n");
    char *bpath = CpyStr("sd:/_b0otloader");
    char *bopath = CpyStr("sd:/bootloader");
    char *kpath = CpyStr("sd:/atmosphere/_k1ps");
    char *kopath = CpyStr("sd:/atmosphere/kips");

    char *ppath = CpyStr("sd:/bootloader/_patchesCW.ini");
    char *popath = CpyStr("sd:/atmosphere/patches.ini");

    if (FileExists(bpath)) {
        if (FileExists(bopath)) {
            FolderDelete(bopath);
        }
        int res = f_rename(bpath, bopath);
        if (res){
            DrawError(newErrCode(res));
        }
        gfx_printf("-- Bootloader Behoben!\n");
    }

    if (FileExists(kpath)) {
        if (FileExists(kopath)) {
            FolderDelete(kopath);
        }
        int res = f_rename(kpath, kopath);
        if (res){
            DrawError(newErrCode(res));
        }
        gfx_printf("-- kips Behoben!\n");
    }

    if (FileExists(ppath)) {
        if (FileExists(popath)) {
            _DeleteFileSimple(popath);
        }
        _RenameFileSimple(ppath,popath);
        gfx_printf("-- patches.ini Behoben!\n");
    }

    free(bpath);
    free(bopath);
    free(kpath);
    free(kopath);
    free(ppath);
    free(popath);

    gfx_printf("\n\n Fertig, druecke eine Taste um Fortzufahren");
    hidWait();
}

void _deleteTheme(char* basePath, char* folderId){
    char *path = CombinePaths(basePath, folderId);
    if (FileExists(path)) {
        gfx_printf("-- Gefundene Themes: %s\n", path);
        FolderDelete(path);
    }
    free(path);
}

void m_entry_deleteInstalledThemes(){
    gfx_clearscreen();
    gfx_printf("\n\n-- Loesche Installierte Themes.\n\n");
    _deleteTheme("sd:/atmosphere/contents", "0100000000001000");
    _deleteTheme("sd:/atmosphere/contents", "0100000000001007");
    _deleteTheme("sd:/atmosphere/contents", "0100000000001013");

    gfx_printf("\n\n Fertig, druecke eine Taste um Fortzufahren");
    hidWait();
}

void m_entry_deleteBootFlags(){
    gfx_clearscreen();
    gfx_printf("\n\n-- Deaktieviere das Automatische Starten der sysmodule\n\n");
    char *storedPath = CpyStr("sd:/atmosphere/contents");
    int readRes = 0;
    Vector_t fileVec = ReadFolder(storedPath, &readRes);
    if (readRes){
        clearFileVector(&fileVec);
        DrawError(newErrCode(readRes));
    } else {
        vecDefArray(FSEntry_t*, fsEntries, fileVec);
        for (int i = 0; i < fileVec.count; i++){

            char *suf = "/flags/boot2.flag";
            char *flagPath = CombinePaths(storedPath, fsEntries[i].name);
            flagPath = CombinePaths(flagPath, suf);

            if (FileExists(flagPath)) {
                gfx_printf("Loesche: %s\n", flagPath);
                _DeleteFileSimple(flagPath);
            }
            free(flagPath);
        }
    }
    gfx_printf("\n\n Fertig, druecke eine Taste um Fortzufahren");
    hidWait();
}



void m_entry_fixMacSpecialFolders(){
    gfx_clearscreen();
    gfx_printf("\n\n-- Mac-Ordner reparieren (dies kann einige Zeit in Anspruch nehmen, bitte warten.)\n\n");
    listdir("/", 0);
    gfx_printf("\n\rFertig, druecke eine Taste um Fortzufahren");
    hidWait();
    
    // browse path
    // list files & folders
    // if file -> delete
    // if folder !== nintendo
    //      if folder m_entry_fixMacSpecialFolders with new path
}

void m_entry_stillNoBootInfo(){
    gfx_clearscreen();
    gfx_printf("\n\n-- Meine Switch Startet nicht mehr.\n\n");

    gfx_printf("%kHast du eine Cardrige in der Switch?\n", COLOR_WHITE);
    gfx_printf("Nimm das Spiel raus und Versuche die Switch zu Starten.\n\n");

    gfx_printf("%kHaben Sie kürzlich Atmosphere/SwitchBros aktualisiert?\n", COLOR_WHITE);
    gfx_printf("Legen Sie Ihre SD-Karte in einen Computer ein, löschen Sie 'atmosphere', 'bootloader' und 'sept', laden Sie Ihre bevorzugtes CFW Paket herunter und legen Sie die Dateien wieder auf Ihre Switch. Wir bieten euch auch gerne Hilfe auf unseren SwitchBros. Discord Server an. https://discord.gg/switchbros\n\n");

    gfx_printf("%kHaben Sie gerade eine neue SD-Karte gekauft?\n", COLOR_WHITE);
    gfx_printf("Stell sicher das es keine Fake SD karte ist. \n\n");

    gfx_printf("\n\n Fertig, druecke eine Taste um Fortzufahren");
    hidWait();
}

void m_entry_ViewCredits(){
    gfx_clearscreen();
    gfx_printf("\nCommon Problem Resolver v%d.%d.%d\nvon Team Neptune (uebersetzt von SwitchBros. und OLED Support hinzugefügt)\n\nBasierend auf TegraExplorer von SuchMemeManySkill,\nLockpick_RCM & Hekate, von shchmue & CTCaer\n\n\n", LP_VER_MJ, LP_VER_MN, LP_VER_BF);
    hidWait();
}

void m_entry_fixAll(){
    gfx_clearscreen();
    m_entry_deleteBootFlags();
    m_entry_deleteInstalledThemes();
    m_entry_fixClingWrap();
    m_entry_fixAIOUpdate();


    m_entry_stillNoBootInfo();
}
