#include "err.h"
#include "gfx/gfx.h"
#include "hid/hid.h"
#include "gfx/gfxutils.h"

const char *TEErrors[] = {
    "I/O ERROR",
    "NO DISK",
    "NOT READY",
    "NO FILE",
    "NO PATH",
    "PATH INVALID",
    "ACCESS DENIED",
    "ACCESS DENIED",
    "INVALID PTR",
    "PROTECTED",
    "INVALID DRIVE",
    "NO MEM",
    "NO FAT",
    "MKFS ABORT",
    [TE_ERR_UNIMPLEMENTED - 1] = "Nicht Implementiert",
    [TE_EXCEPTION_RESET - 1] = "E Zuruecksetzen",
    [TE_EXCEPTION_UNDEFINED - 1] = "E Unbestimmt",
    [TE_EXCEPTION_PREF_ABORT - 1] = "E Pref Ueber",
    [TE_EXCEPTION_DATA_ABORT - 1] = "E Daten ueber",
    [TE_ERR_SAME_LOC - 1] = "Selber Kopier Ort",
    [TE_ERR_KEYDUMP_FAIL - 1] = "Keysicherung fehlgeschlagen",
    [TE_ERR_PARTITION_NOT_FOUND - 1] = "Partition nicht gefunden",
    [TE_ERR_PATH_IN_PATH - 1] = "Ordner kann nicht in sich selbst verschoben/kopiert werden",
    [TE_ERR_EMMC_READ_FAIL - 1] = "eMMC/emuMMC fehler beim Lesen",
    [TE_ERR_EMMC_WRITE_FAIL - 1] = "eMMC/emuMMC fehler beim Schreiben",
    [TE_ERR_NO_SD - 1] = "Keine SD gefunden",
    [TE_ERR_FILE_TOO_BIG_FOR_DEST - 1] = "Datei ist zu gross fuer das ziel",
};

const char *GetErrStr(u32 err){
    --err; // obv error codes cannot be 0
    if (err >= 0 && err < ARRAY_SIZE(TEErrors))
        return TEErrors[err];

    return "(Unknown)";
}

#define lx 256
#define ly 240
#define lenx 768
#define leny 240

void DrawError(ErrCode_t err){
    if (err.err == 0)
        return;
        
    SETCOLOR(COLOR_ORANGE, COLOR_DARKGREY);
    gfx_box(lx, ly, lx + lenx, ly + leny, COLOR_ORANGE);
    gfx_boxGrey(lx + 16, ly + 16, lx + lenx - 16, ly + leny - 16, 0x33);
    gfx_con_setpos(lx + ((lenx - 17 * 16) / 2), ly + 32);
    gfx_printf("Ein Fehler ist aufgetaucht!\n\n%bErr : %d\nLine: %d\nFile: %s\nDesc: %s%b", lx + 48, err.err, err.loc, err.file, GetErrStr(err.err), 0);
    gfx_con_setpos(lx + ((lenx - 19 * 16) / 2), ly + leny - 48);
    gfx_printf("Druecke A um Fortzufahren");
    
    hidWaitMask((JoyA | JoyB));
}