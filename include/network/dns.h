#pragma once

#include "platform.h"

// https://tools.ietf.org/html/rfc1035#section-3.2.2
// https://tools.ietf.org/html/rfc3596
typedef enum
{
    // am I missing any records that might be of any practical use?
    // NSEC/NSEC3 ?
    A = 1,
    AAAA = 28,
    CNAME = 5,
    MX = 15,
    NS = 2,
    PTR = 12,
    TXT = 16
    // CERT = 37   // do we need it here? anyway, RFC to follow would be https://tools.ietf.org/html/rfc4398
} RequestType;

// Struct to represent a DNS client
class DNS
{
private:
    // Function to resolve a hostname using HTTP POST (defaults to IPv6/AAAA)
    static IPAddress ResolveOverHttp(PCCHAR host, const IPAddress& DNSServerIp, PCCHAR DNSServerName, RequestType dnstype = AAAA);

public:
    // Function to resolve a hostname to an IP address (tries IPv6 first, then IPv4)
    static IPAddress Resolve(PCCHAR host, RequestType dnstype = AAAA);
    // Cloudflare DNS over HTTPS [IP:1.1.1.1|1.0.0.1] [POST:/dns-query] [content-type:application/dns-message]
    static IPAddress CloudflareResolve(PCCHAR host, RequestType dnstype = AAAA);
    // Google DNS over HTTPS [IP:8.8.8.8|8.8.4.4] [POST:/dns-query] [content-type:application/dns-message]
    static IPAddress GoogleResolve(PCCHAR host, RequestType dnstype = AAAA);
};

