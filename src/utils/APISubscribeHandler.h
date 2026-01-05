/*
Author:(POC Device Magellan team)
Create Date: 28 february 2025.
Modified: 28 february 2025.
Released for private usage.
*/
#ifndef APISUBSCRIBEHANDLER_H
#define APISUBSCRIBEHANDLER_H
#include <Arduino.h>
enum ENUM_SubListName
{
    SubControlPlaintext = 0,
    SubControlJSON,
    SubServerConfigPlaintext,
    SubServerConfigJSON,
    SubServerTimePlaintext,
    SubServerTimeJSON,
    SubReportResponsePlaintext,
    SubReportResponseJSON,
    SubReportWithTimestamp,
    SubHeartbeatPlaintext,
    SubHeartbeatJSON,
    SubFirmwareInfo,
    SubFirmwareDownload
};

struct SubscribesCheckLists
{
private:
    struct SubscriptionItem
    {
        bool value;       // Boolean value
        const char *name; // Name of the subscription
    };

    static constexpr int sizeList = 13;
    SubscriptionItem subscriptions[sizeList] = {
        {false, "SubControlPlaintext"},
        {false, "SubControlJSON"},
        {false, "SubServerConfigPlaintext"},
        {false, "SubServerConfigJSON"},
        {false, "SubServerTimePlaintext"},
        {false, "SubServerTimeJSON"},
        {false, "SubReportResponsePlaintext"},
        {false, "SubReportResponseJSON"},
        {false, "SubReportWithTimestamp"},
        {false, "SubHeartbeatPlaintext"},
        {false, "SubHeartbeatJSON"},
        {false, "SubFirmwareInfo"},
        {false, "SubFirmwareDownload"}};

public:
    // Method to print subscription statuses
    void GetSubList()
    {
        Serial.println(F("=== SubscribesCheckLists ==="));
        for (int i = 0; i < sizeList; i++)
        {
            Serial.print("[");
            Serial.print(subscriptions[i].value ? "✔" : "x");
            Serial.print("] ");
            Serial.println(subscriptions[i].name);
        }
        Serial.println(F("==========================="));
    }

    // Method to update a subscription by index
    void SetSubscription(int index, bool status)
    {
        if (index >= 0 && index < sizeList)
        {
            subscriptions[index].value = status;
        }
    }

    // Method to update a subscription by name
    void SetSubscription(const char *subName, bool status)
    {
        for (int i = 0; i < sizeList; i++)
        {
            if (strcmp(subscriptions[i].name, subName) == 0)
            {
                subscriptions[i].value = status;
                return;
            }
        }
    }

    // Method to get subscription status by name
    bool GetSubscriptionStatus(const char *subName)
    {
        for (int i = 0; i < sizeList; i++)
        {
            if (strcmp(subscriptions[i].name, subName) == 0)
            {
                return subscriptions[i].value;
            }
        }
        return false; // Default to false if not found
    }
};

#endif