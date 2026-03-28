

---

# DPI Engine - Deep Packet Inspection System

This project reads network packets from a PCAP file, groups them into flows, classifies the traffic, applies blocking rules, and writes allowed packets to a new PCAP file.

---

## Overview

**Input**

* A `.pcap` file with recorded network traffic

**Processing**

* Read packets
* Extract basic details (IP, port, protocol)
* Group packets into flows
* Identify domain (SNI / HTTP Host)
* Classify traffic
* Apply blocking rules

**Output**

* Filtered `.pcap` file
* Summary of processed and blocked traffic

---

## Basic Idea

### Flow (5-tuple)

A flow is a group of packets with:

* source IP
* destination IP
* source port
* destination port
* protocol

All packets with the same values belong to one connection.

### SNI

Even when traffic is encrypted (HTTPS), the domain name is often visible in the TLS handshake.
This project uses it to identify websites.

---

## Architecture

```
Input PCAP
   ↓
Packet Reader
   ↓
Load Balancer
   ↓
Worker Threads (Fast Path)
   ↓
Classification + Rules
   ↓
Output PCAP
```

* Multiple threads process packets in parallel
* Same flow always goes to the same thread

---

## Main Components

* **pcap_reader** → reads packets from file
* **packet_parser** → extracts IP, ports, protocol
* **connection_tracker** → keeps track of flows
* **sni_extractor** → gets domain names
* **rule_manager** → checks blocking rules
* **fast_path** → processes packets in threads
* **load_balancer** → distributes packets
* **dpi_engine** → controls everything
* **ml_traffic_classifier** → optional ML-based classification

---

## Classification

Traffic is labeled per flow.

Examples:

* HTTP, HTTPS, DNS
* Google, YouTube, Netflix, WhatsApp, etc.

ML model (optional):

```
models/traffic_dt_model.txt
```

---

## Blocking Rules

You can block traffic based on:

* IP
* Domain (supports wildcard like `*.youtube.com`)
* App
* Port

---

## Build and Run

### Using CMake (recommended)

```bash
cmake -S . -B build
cmake --build build --config Release
```

Run:

```bash
./build/dpi_engine input.pcap output.pcap 200
```

---

### Manual build

```bash
g++ -std=c++17 -pthread -O2 -I include -o dpi_engine \
src/main_dpi.cpp src/dpi_engine.cpp src/load_balancer.cpp \
src/fast_path.cpp src/connection_tracker.cpp src/rule_manager.cpp \
src/ml_traffic_classifier.cpp src/pcap_reader.cpp \
src/packet_parser.cpp src/sni_extractor.cpp src/types.cpp
```

---

## Running Examples

### Normal run

```bash
./build/dpi_engine test_dpi.pcap output.pcap 200
```

### With ML model

```bash
./build/dpi_engine test_dpi.pcap output.pcap 200 \
--ml-model models/traffic_dt_model.txt
```

### With rules file

```bash
./build/dpi_engine test_dpi.pcap output.pcap 200 \
--rules blocking_rules_demo.txt
```

### Inline blocking

```bash
./build/dpi_engine test_dpi.pcap output.pcap 200 \
--block-app DNS \
--block-domain *.google.com \
--block-ip 192.168.1.50
```

---

## Rules File Format

```
[BLOCKED_IPS]
192.168.1.50

[BLOCKED_APPS]
DNS
YouTube

[BLOCKED_DOMAINS]
*.youtube.com
*.google.com

[BLOCKED_PORTS]
443
```

---

## Output

The program shows:

* total packets processed
* packets forwarded
* packets blocked

You can also check the output file:

```bash
ls -lh out.pcap
```

---

## Common Issues

**Rules not working**

* check file path
* check section names
* make sure rules match traffic

**Model not loading**

* check file path
* check console output

**Program not running**

* check input file
* check current folder

---

## Future Improvements

* IPv6 support
* better ML model
* real-time stats
* more protocol support

---

## Quick Start

```bash
cmake -S . -B build
cmake --build build --config Release

./build/dpi_engine test_dpi.pcap out.pcap 200 \
--rules blocking_rules_demo.txt
```

---




