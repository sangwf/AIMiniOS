#include "types.h"
#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "network.h"
#include "kernel.h"
#include "http.h"
#include "rtl8139.h"
#include "arp.h"
#include "byteorder.h"
#include "tcp.h"
#include "memory.h"
#include "serial.h"
#include "pci.h"

// RTL8139 PCI device ID
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

// External function declarations
extern void print_ip(uint32_t ip);
extern bool disable_rtl_debug;
extern struct net_device net_dev;
extern void rtl8139_dump_registers(void);
extern void check_rx_buffer(void);
extern void send_icmp_echo_request(uint32_t target_ip);

// Internal variables for ping timing
uint32_t ping_counter = 0;
const uint32_t PING_INTERVAL = 10000000; // Time between pings
uint8_t num_pings_sent = 0;
const uint8_t MAX_PINGS = 3; // Send only 3 pings

// Test sending a network packet
void test_network(void) {
    terminal_writestring("Testing network send...\n");
    
    // Create a test packet (just a simple UDP packet)
    uint8_t test_data[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest MAC (broadcast)
        0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Source MAC
        0x08, 0x00,                         // IPv4 EtherType
        
        // IPv4 header
        0x45, 0x00,                         // Version, IHL, DSCP, ECN
        0x00, 0x1C,                         // Total Length (28 bytes)
        0x00, 0x00,                         // Identification
        0x00, 0x00,                         // Flags, Fragment Offset
        0x40, 0x11,                         // TTL (64), Protocol (17 = UDP)
        0x00, 0x00,                         // Header Checksum (to be filled)
        0x0A, 0x00, 0x02, 0x0F,             // Source IP (10.0.2.15)
        0xFF, 0xFF, 0xFF, 0xFF,             // Dest IP (255.255.255.255)
        
        // UDP header
        0x12, 0x34,                         // Source Port (4660)
        0x43, 0x21,                         // Dest Port (17185)
        0x00, 0x08,                         // Length (8 bytes)
        0x00, 0x00                          // Checksum (to be filled)
    };
    
    // Calculate checksums (optional in this simple test)
    
    // Send the packet
    for(int i = 0; i < sizeof(test_data); i++) {
        terminal_writehex8(test_data[i]);
        terminal_writestring(" ");
    }
    terminal_writestring("\n");
    
    if (network_send_packet(test_data, sizeof(test_data))) {
        terminal_writestring("Packet sent successfully\n");
    } else {
        terminal_writestring("Failed to send packet\n");
    }
}

// Test network basics
void test_network_basics(void) {
    // Clear screen
    terminal_clear();
    terminal_writestring("\n--- Basic Network Test ---\n\n");
    
    // 1. Check network card status
    terminal_writestring("1. Network Card Status:\n");
    terminal_writestring("   IP Address: ");
    print_ip(net_dev.ip_addr);
    terminal_writestring("\n   MAC Address: ");
    for(int i = 0; i < 6; i++) {
        terminal_writehex8(net_dev.mac_addr[i]);
        if(i < 5) terminal_writestring(":");
    }
    terminal_writestring("\n   Gateway: ");
    print_ip(net_dev.gateway);
    terminal_writestring("\n\n");
    
    // 2. Test ARP - resolve gateway MAC
    terminal_writestring("2. Test ARP - Resolving Gateway MAC Address:\n");
    terminal_writestring("   Sending ARP request...\n");
    uint8_t gateway_mac[6];
    memset(gateway_mac, 0, 6);
    
    // Enable RTL debug output for ARP resolution
    bool original_debug_setting = disable_rtl_debug;
    disable_rtl_debug = false;
    
    // Send ARP request to gateway
    send_arp_request(net_dev.gateway);
    
    // Wait a moment for ARP reply
    terminal_writestring("   Waiting for ARP reply...\n");
    for(volatile int i = 0; i < 50000000; i++) {
        // Check for responses periodically
        if (i % 10000000 == 0) {
            check_rx_buffer();
            
            // Try to resolve gateway MAC again after some time
            if (arp_resolve(net_dev.gateway, gateway_mac)) {
                terminal_writestring("   Success! Gateway MAC: ");
                for(int j = 0; j < 6; j++) {
                    terminal_writehex8(gateway_mac[j]);
                    if(j < 5) terminal_writestring(":");
                }
                terminal_writestring("\n\n");
                break;
            }
        }
    }
    
    // Restore original debug setting
    disable_rtl_debug = original_debug_setting;
    
    // Check if we got the MAC address
    if (gateway_mac[0] == 0 && gateway_mac[1] == 0 && gateway_mac[2] == 0 &&
        gateway_mac[3] == 0 && gateway_mac[4] == 0 && gateway_mac[5] == 0) {
        terminal_writestring("   Failed: Could not get gateway MAC address\n\n");
        terminal_writestring("Cannot proceed with ping test without gateway MAC address.\n");
        return;
    }
    
    // Wait a moment before sending pings
    terminal_writestring("Preparing to send ICMP echo requests to host...\n");
    for(volatile int i = 0; i < 10000000; i++);
    
    // Send ICMP echo request to host IP (gateway)
    send_icmp_echo_request(net_dev.gateway);
}

// Test sending an HTTP request
void test_http_request(void) {
    terminal_writestring("Testing HTTP Send...\n");
    
    // Send a simple HTTP request - disabled for now
    char *request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
    terminal_writestring("HTTP request prepared: ");
    terminal_writestring(request);
    terminal_writestring("\n(HTTP send feature not implemented yet)\n");
}

// Kernel main function
void kernel_main(void) {
    // Initialize terminal
    terminal_initialize();
    terminal_writestring("MiniOS Booting...\n");
    
    // Initialize GDT
    gdt_install();
    terminal_writestring("GDT initialized\n");
    
    // Initialize IDT
    idt_install();
    terminal_writestring("IDT initialized\n");
    
    // Initialize serial port
    serial_init();
    serial_write_string("Serial port initialized\r\n");
    
    // Initialize PCI and find network device
    pci_init();
    terminal_writestring("PCI initialized\n");
    
    // Find RTL8139 device
    uint16_t rtl8139_bus = 0, rtl8139_slot = 0;
    if (!pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &rtl8139_bus, &rtl8139_slot)) {
        terminal_writestring("RTL8139 not found!\n");
        return;
    }
    
    // Initialize RTL8139
    rtl8139_init(rtl8139_bus, rtl8139_slot);
    terminal_writestring("RTL8139 initialized\n");
    
    // Initialize ARP
    arp_init();
    terminal_writestring("ARP initialized\n");
    
    // Initialize network
    network_init();
    
    // Clear screen and display welcome message
    terminal_clear();
    terminal_writestring("\n\n");
    terminal_writestring("======================================\n");
    terminal_writestring("     MiniOS ICMP Client Demo     \n");
    terminal_writestring("======================================\n\n");
    
    terminal_writestring("System IP: ");
    print_ip(net_dev.ip_addr);
    terminal_writestring("\n\n");
    
    terminal_writestring("This system will send 'hello, world' PING\n");
    terminal_writestring("requests to host and display responses.\n\n");
    
    terminal_writestring("Host IP (gateway): ");
    print_ip(net_dev.gateway);
    terminal_writestring("\n\n");
    
    terminal_writestring("Preparing to send PING requests...\n");
    terminal_writestring("======================================\n");
    
    // Send serial log
    serial_write_string("\r\n=== MiniOS ICMP Client Ready ===\r\n");
    serial_write_string("System will send PING with 'hello, world' to host\r\n");
    serial_write_string("System IP: ");
    serial_print_ip(net_dev.ip_addr);
    serial_write_string("\r\n");
    serial_write_string("Target IP (gateway): ");
    serial_print_ip(net_dev.gateway);
    serial_write_string("\r\n");
    
    // Try to resolve the gateway MAC (10.0.2.2) for connectivity test
    uint8_t gateway_mac[6];
    serial_write_string("Sending ARP request to resolve gateway MAC...\r\n");
    
    // First send ARP request to gateway
    send_arp_request(net_dev.gateway);
    
    // Wait a bit for response and check RX buffer
    bool gateway_resolved = false;
    for (int i = 0; i < 20; i++) {
        // Check for received packets
        check_rx_buffer();
        
        // Check if we now have the MAC address
        if (get_mac_from_cache(net_dev.gateway, gateway_mac)) {
            serial_write_string("Gateway MAC resolved successfully: ");
            for (int j = 0; j < 6; j++) {
                serial_write_hex_byte(gateway_mac[j]);
                if (j < 5) serial_write_string(":");
            }
            serial_write_string("\r\n");
            
            terminal_writestring("Network connectivity: GOOD\n");
            terminal_writestring("Gateway (10.0.2.2) is reachable\n\n");
            gateway_resolved = true;
            break;
        }
        
        // Delay a bit
        for (volatile int j = 0; j < 100000; j++) {}
    }
    
    if (!gateway_resolved) {
        serial_write_string("Warning: Failed to resolve gateway MAC\r\n");
        terminal_writestring("Warning: Gateway (10.0.2.2) not responding to ARP\n");
        terminal_writestring("Cannot send PING without gateway MAC address\n");
    } else {
        // Send first ping request
        terminal_writestring("Sending PING request to host...\n\n");
        send_icmp_echo_request(net_dev.gateway);
    }
    
    // Force serial buffer flush
    serial_write_string("\r\n=== System ready ===\r\n");
    serial_write_string("Sending pings to gateway 10.0.2.2\r\n");
    serial_write_string("======================================\r\n\r\n");
    
    // Main loop - keep checking for network packets and periodically send pings
    uint32_t ping_timer = 0;
    while (1) {
        check_rx_buffer();
        
        // Every 20M iterations, send another ping
        ping_timer++;
        if (ping_timer >= 20000000 && gateway_resolved) {
            ping_timer = 0;
            serial_write_string("\r\nSending periodic PING to gateway...\r\n");
            terminal_writestring("\nSending new PING request to host...\n");
            send_icmp_echo_request(net_dev.gateway);
        }
    }
}
