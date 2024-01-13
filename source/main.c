#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>

static Handle c_amHandle;
static int c_amRefCount;

Result c_amInit(void)
{
    Result ret;

    if (AtomicPostIncrement(&c_amRefCount)) return 0;

    ret = srvGetServiceHandle(&c_amHandle, "am:net");
    if (R_FAILED(ret)) AtomicDecrement(&c_amRefCount);

    return ret;
}

void c_amExit(void)
{
    if (AtomicDecrement(&c_amRefCount)) return;
    svcCloseHandle(c_amHandle);
}

#define AM_NET_DEVICECERT_SIZE 0x180
Result c_AM_GetDeviceCert(u8 deviceCert[AM_NET_DEVICECERT_SIZE], u32* additionalErrorCode)
{
    Result ret = 0;
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x818,1,2); // 0x08180042
    cmdbuf[1] = AM_NET_DEVICECERT_SIZE;
    cmdbuf[2] = IPC_Desc_Buffer(AM_NET_DEVICECERT_SIZE, IPC_BUFFER_W);
    cmdbuf[3] = (u32)deviceCert;

    if(R_FAILED(ret = svcSendSyncRequest(c_amHandle))) return ret;
    *additionalErrorCode = cmdbuf[2];
    
    return (Result)cmdbuf[1];
}

int main(int argc, char* argv[])
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    
    u32 additionalErrorCode;
    u8 ctcert[AM_NET_DEVICECERT_SIZE];

    printf("amInit...");
    Result res = c_amInit();
    printf(" 0x%08lX\n", res);
    
    if (R_SUCCEEDED(res)) {
        printf("amGetDeviceCert...");
        res = c_AM_GetDeviceCert(ctcert, &additionalErrorCode);
        printf("0x%08lX, 0x%08lX\n", res, additionalErrorCode);
        c_amExit();
    }

    if (R_SUCCEEDED(res)) {
        FILE *f = fopen("/CTCert.bin", "w");
        fwrite(ctcert, 1, sizeof(ctcert), f);
        fclose(f);
        
        printf("\n\nCTCert.bin dumped to the SD card.\nNOTE: Do not share this file with anyone!\n\nPress START to exit.");
    } else {
        printf("\n\nFailed to dump CTCert.bin\n\nPress START to exit.");
    }

    while (aptMainLoop())
    {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;
    }

    gfxExit();
    return 0;
}
