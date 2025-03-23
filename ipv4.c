// 将IP地址字符串转换为32位整数
uint32_t ip_str_to_int(const char *ip_str) {
    uint32_t a, b, c, d;
    sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a) | (b << 8) | (c << 16) | (d << 24);
} 