#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <ViGEm/Client.h>

#include <mutex>
#include <iostream>

static std::mutex mutex;

_Function_class_(EVT_VIGEM_X360_NOTIFICATION)
VOID CALLBACK notification(
    PVIGEM_CLIENT /*Client*/,
    PVIGEM_TARGET /*Target*/,
    UCHAR LargeMotor,
    UCHAR SmallMotor,
    UCHAR LedNumber,
    LPVOID /*UserData*/
)
{
    static int count = 1;

    auto lock = std::unique_lock<std::mutex>(mutex);
    {
        std::cout.width(3);
        std::cout << count++ << " ";
        std::cout.width(3);
        std::cout << (int)LargeMotor << " ";
        std::cout.width(3);
        std::cout << (int)SmallMotor << " ";
        std::cout.width(3);
        std::cout << (int)LedNumber << std::endl;
    }
}

uint32_t mode  = 0;
uint32_t count = 0;

PVIGEM_CLIENT  client  = nullptr;
PVIGEM_TARGET* targets = nullptr;

BOOL WINAPI signal_handler(_In_ DWORD sign)
{
    if (sign != CTRL_C_EVENT) {
        return false;
    }

    if (targets) {
        for (size_t i = 0; i < count; ++i) {

            if (mode == 1) {
                vigem_target_x360_unregister_notification(targets[i]);
            }

            vigem_target_remove(client, targets[i]);
            vigem_target_free(targets[i]);

            targets[i] = nullptr;
        }

        free(targets);

        targets = nullptr;
    }

    if (client) {
        vigem_free(client);

        client = nullptr;
    }

    return true;
}

int main()
{
    VIGEM_ERROR result;

    (void)printf ("Please select mode (1. XBOX 360; 2. DS4 ): ");
    (void)scanf_s("%u", &mode);

    (void)printf ("Please input count (count <= 32): ");
    (void)scanf_s("%u", &count);

    do {
        if (mode != 1 && mode != 2) {
            result = VIGEM_ERROR_INVALID_PARAMETER;
            break;
        }

        if (count > 32) {
            result = VIGEM_ERROR_INVALID_PARAMETER;
            break;
        }

        if (!SetConsoleCtrlHandler(signal_handler, true)) {
            result = (VIGEM_ERROR)GetLastError();
            break;
        }

        client = vigem_alloc();
        result = vigem_connect(client);
        if (!VIGEM_SUCCESS(result)) {
            break;
        }

        targets = (PVIGEM_TARGET*)malloc(sizeof(PVIGEM_TARGET) * count);
        if (targets == nullptr) {
            result = (VIGEM_ERROR)ERROR_OUTOFMEMORY;
            break;
        }

        for (size_t i = 0; i < count; i++) {
            if (mode == 1) {
                targets[i] = vigem_target_x360_alloc();
            }
            else {
                targets[i] = vigem_target_ds4_alloc();
            }

            result = vigem_target_add(client, targets[i]);
            if (!VIGEM_SUCCESS(result)) {
                break;
            }
        }

        if (!VIGEM_SUCCESS(result)) {
            break;
        }

        (void)printf("press any key to test send (escape end)... \n");
        (void)getchar();

        if (mode == 1) {
            XUSB_REPORT report;
            XUSB_REPORT_INIT(&report);

            while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) && targets) {
                result = vigem_target_x360_update(client, targets[0], report);
                report.bLeftTrigger++;
                Sleep(100);
            }
        }
        else {
            DS4_REPORT report;
            DS4_REPORT_INIT(&report);

            while (!(GetAsyncKeyState(VK_ESCAPE) & 0x8000) && targets) {
                result = vigem_target_ds4_update(client, targets[0], report);
                report.bThumbLX++;
                report.wButtons |= DS4_BUTTON_CIRCLE;
                Sleep(100);
            }
        }

        (void)printf("press any key to exit... \n");
        (void)getchar();

    } while (false);

    GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
    return result;
}
