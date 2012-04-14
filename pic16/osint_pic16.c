#include <stdio.h>
#include <stdlib.h>
#include "db_vm.h"

#include "XGS_PIC_SYSTEM_V010.h"
#include "XGS_PIC_UART_DRV_V010.h"

#ifdef USE_TV

#include "XGS_PIC_GFX_DRV_V010.h"
#include "XGS_PIC_NTSC_TILE2_V010.h"
#include "XGS_PIC_C64_FONT_8x8_V010.h"
#include "XGS_PIC_KEYBOARD_DRV_V010.h"
#include <ctype.h>

DATA_SPACE unsigned char g_TileMap[SCREEN_TILE_MAP_WIDTH*SCREEN_TILE_MAP_HEIGHT];
DATA_SPACE unsigned char g_Palettes[MAX_PALETTE_SIZE];
DATA_SPACE Sprite g_Sprites[MAX_SPRITES];

#endif

void VM_sysinit(int argc, char *argv[])
{
    int i;

    SYS_ConfigureClock(FCY_NTSC);

    UART_Init(57600);
    DELAY_MS(500);

#ifdef USE_TV
    // Init the graphics system
    GFX_InitTile(g_TileFontBitmap, g_TileMap, g_Palettes, g_Sprites);

    // Keyboard init
    KEYBRD_Init();

    // Loop through and give everything their default color
    for(i = 0; i < MAX_PALETTE_SIZE; ++i)
        g_Palettes[i] = i;

    // Set palette 0 to blue for text background, white for fon
    g_Palettes[0] = NTSC_BLUE;
    g_Palettes[1] = NTSC_WHITE;

    // Initalize the Text Mode graphics functions
    GFX_TMap_Size(SCREEN_TILE_MAP_WIDTH, SCREEN_TILE_MAP_HEIGHT);

    // Clear out the text screen with spaces
    GFX_TMap_CLS(' ');
    GFX_TMap_Print_String("XGS PIC 16BIT\nKEYBOARD DEMO V1.0\n\n");
    GFX_TMap_Print_String("TYPE ON THE KEYBOARD\n");
    GFX_TMap_Print_String("(F5 TO CLEAR SCREEN)\n\n");
#endif

#if 0
    SPI_Init();
    Keyboard_Load();
    NTSC_ClearScreen();
    NTSC_Color(0);
    VGA_ClearScreen();
    VGA_Color(0);
	LCD_Init();
	StartTimer();
	LCD_CursorOn(ReadTimer(), 5);
#endif
}

void VM_sysexit(void)
{
    for (;;)
        ;
}

#define BS  0xc8

void VM_getline(char *buf, int size)
{
    int max = size - 2;
    int i = 0;
    int ch;

    for (;;) {
        switch (ch = VM_getchar()) {
        case '\r':
            VM_putchar('\n');
            buf[i++] = '\n';
            buf[i] = '\0';
            return;
        case BS:
            if (i > 0) {
                VM_putchar('\010');
                VM_putchar(' ');
                VM_putchar('\010');
                --i;
            }
            break;
        default:
            if (i < max) {
                buf[i++] = ch;
                VM_putchar(ch);
            }
            break;
        }
    }
}

void VM_vprintf(const char *fmt, va_list ap)
{
    char buf[80], *p = buf;
    vsnprintf(buf, sizeof(buf), fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
}

int VM_getchar(void)
{
    int ch;
    while ((ch = UART_getchar()) < 0)
        ;
    return ch;
}

void VM_putchar(int ch)
{
    UART_putchar(ch);
}

void VM_flush(void)
{
}

#ifdef LOAD_SAVE

static int mounted = VMFALSE;

static int TryMountSD(void)
{
    if (!mounted) {
        if (FSInit())
            mounted = VMTRUE;
        else
            VM_printf("SD card initialization failed\n");
    }
    return mounted;
}

int VM_opendir(const char *path, VMDIR *dir)
{
    if (!mounted && !TryMountSD())
        return -1;
    /* BUG: need to do something with 'path' */
    if (FindFirst("*.*", ATTR_MASK, &dir->rec) != 0)
        return -1;
    dir->first = VMTRUE;
    return 0;
}

int VM_readdir(VMDIR *dir, VMDIRENT *entry)
{
    if (dir->first)
        dir->first = VMFALSE;
    else if (FindNext(&dir->rec) != 0)
        return -1;
    strcpy(entry->name, dir->rec.filename);
    return 0;
}

void VM_closedir(VMDIR *dir)
{
}

VMFILE *VM_fopen(const char *name, const char *mode)
{
    if (!mounted && !TryMountSD())
        return NULL;
    return (VMFILE *)FSfopen(name, mode);
}

int VM_fclose(VMFILE *fp)
{
    return FSfclose((FSFILE *)fp);
}

static int VM_getc(VMFILE *fp)
{
    char ch;
    return FSfread(&ch, 1, 1, (FSFILE *)fp) == 1 ? ch : -1;
}

char *VM_fgets(char *buf, int size, VMFILE *fp)
{
    char *p = buf;
    int ch;
    while (--size >= 1) {
        if ((ch = VM_getc(fp)) < 0)
            break;
        else if ((*p++ = ch) == '\n')
            break;
    }
    *p = '\0';
    return ch < 0 ? NULL : buf;
}

int VM_fputs(const char *buf, VMFILE *fp)
{
    return FSfwrite(buf, strlen(buf), 1, (FSFILE *)fp) == 1 ? 0 : -1;
}

#endif

int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && (tolower(*s1) == tolower(*s2))) {
        ++s1;
        ++s2;
    }
    return tolower((unsigned char) *s1) - tolower((unsigned char) *s2);
}
