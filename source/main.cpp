#include <cstring>
#include <orbis/libkernel.h>

int main()
{
    OrbisNotificationRequest notification{};
    notification.type = OrbisNotificationRequestType::NotificationRequest;
    notification.unk3 = 0;
    notification.useIconImageUri = 1;
    notification.targetId = -1;

    std::strncpy(notification.iconUri,
                 "cxml://psnotification/tex_icon_system",
                 sizeof(notification.iconUri) - 1);
    std::strncpy(notification.message,
                 "BLACKOUT TEST - EBOOT OK",
                 sizeof(notification.message) - 1);

    sceKernelSendNotificationRequest(0, &notification, sizeof(notification), 0);

    // Keep the title running briefly so the notification can be observed.
    sceKernelUsleep(10000000);
    return 0;
}
