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
    static IPAddress ResloveOverHttpPost(PCCHAR host, const IPAddress& DNSServerIp, PCCHAR DNSServerName, RequestType dnstype = AAAA);
    // Callback function for formatting DNS queries
    static BOOL FormatterCallback(PVOID context, CHAR ch);

public:
    // Function to resolve a hostname to an IP address (tries IPv6 first, then IPv4)
    static IPAddress Resolve(PCCHAR host);
    // Function to resolve a hostname to an IP address using DNS over TLS (defaults to IPv6/AAAA)
    static IPAddress ResolveOverTls(PCCHAR host, RequestType dnstype = AAAA);
    // Function to resolve a hostname to an IP address using DNS over HTTPS (defaults to IPv6/AAAA)
    static IPAddress ResolveOverHttp(PCCHAR host, RequestType dnstype = AAAA);
    // Cloudflare DNS over HTTPS [IP:1.1.1.1|1.0.0.1] [HOST:cloudflare-dns.com] [POST:cloudflare-dns.com/dns-query] [content-type:application/dns-message] [DNS Wireformat https://www.rfc-editor.org/rfc/rfc1035.html]
    // https://developers.cloudflare.com/1.1.1.1/encryption/dns-over-https/make-api-requests/ (defaults to IPv6/AAAA)
    static IPAddress CloudflareResolve(PCCHAR host, RequestType dnstype = AAAA);
    // Google DNS over HTTPS [IP:8.8.8.8]  [HOST:dns.google] [POST:dns.google/dns-query] [content-type:application/dns-message] [DNS Wireformat https://www.rfc-editor.org/rfc/rfc1035.html]
    // https://developers.google.com/speed/public-dns/docs/secure-transports (defaults to IPv6/AAAA)
    static IPAddress GoogleResolve(PCCHAR host, RequestType dnstype = AAAA);
};

