#include <ntddk.h>
#include <wdm.h>
#include "src\libwsk.h"


EXTERN_C_START
DRIVER_INITIALIZE   DriverEntry;
DRIVER_UNLOAD       DriverUnload;
EXTERN_C_END

#define MAX_ADDRESS_STRING_LENGTH   64u

NTSTATUS DriverEntry(_In_ DRIVER_OBJECT* DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS     Status    = STATUS_SUCCESS;
    PADDRINFOEXW DNSResult = nullptr;

    do 
    {
        DriverObject->DriverUnload = DriverUnload;

        WSKDATA WSKData{};
        Status = WSKStartup(MAKE_WSK_VERSION(1, 0), &WSKData);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        ADDRINFOEXW Hints{};
        Hints.ai_family = AF_UNSPEC;

        Status = WSKGetAddrInfo(L"httpbin.org", L"https", NS_DNS, nullptr, &Hints, &DNSResult,
            WSK_INFINITE_WAIT, nullptr);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        while (DNSResult)
        {
            WCHAR  AddressString[MAX_ADDRESS_STRING_LENGTH]{};
            UINT32 AddressStringLength = MAX_ADDRESS_STRING_LENGTH;

            SOCKADDR_INET Address;
            Address.si_family = static_cast<ADDRESS_FAMILY>(DNSResult->ai_family);

            if (Address.si_family == AF_INET && DNSResult->ai_addrlen >= sizeof Address.Ipv4)
            {
                Address.Ipv4 = *reinterpret_cast<sockaddr_in*>(DNSResult->ai_addr);
            }
            if (Address.si_family == AF_INET6 && DNSResult->ai_addrlen >= sizeof Address.Ipv6)
            {
                Address.Ipv6 = *reinterpret_cast<sockaddr_in6*>(DNSResult->ai_addr);
            }

            WSKAddressToString(
                &Address,
                sizeof Address,
                AddressString,
                &AddressStringLength);

            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                "IP Address: %ls\n", AddressString);

            DNSResult = DNSResult->ai_next;
        }

    } while (false);

    return Status;
}

VOID DriverUnload(_In_ DRIVER_OBJECT* DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    WSKCleanup();
}
