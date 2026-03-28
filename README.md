Use this README version. It follows the style you liked and adds practical run + test snippets for normal mode, ML mode, blocking mode, and verification.

~~~markdown
# DPI Engine - Deep Packet Inspection System

This project reads packets from a PCAP file, tracks flows, classifies traffic, applies blocking rules, and writes allowed packets to a new PCAP file.

This README explains the full project in simple language and gives ready-to-run commands for normal use and testing.

---

## Table of Contents

1. What This Project Does
2. Basic Network Concepts
3. Project Structure
4. How Packet Processing Works
5. Single-Thread vs Multi-Thread Paths
6. Deep Dive by Component
7. Classification (Heuristic + ML)
8. Blocking Rules
9. Build and Run
10. Testing Scenarios
11. Rules File Format
12. Output and How to Verify It
13. Troubleshooting
14. Notes for Extension

---

## 1) What This Project Does

Input:

- A packet capture file (.pcap) with recorded network traffic.

Processing:

- Parse Ethernet/IP/TCP/UDP headers
- Track traffic by 5-tuple flow
- Extract signals like TLS SNI and HTTP Host
- Classify each flow
- Apply blocking rules (IP/app/domain/port)
- Forward allowed packets and drop blocked packets

Output:

- A filtered PCAP file
- Terminal report with packet and blocking stats

High-level flow:

~~~text
input.pcap -> DPI Engine -> output.pcap
                    |
                    +-> classify + apply rules + stats
~~~

---

## 2) Basic Network Concepts

### 5-Tuple

A flow is identified by:

- source IP
- destination IP
- source port
- destination port
- protocol (TCP/UDP)

Packets with the same 5-tuple belong to the same connection flow.

### Why SNI matters

In HTTPS, most payload is encrypted, but early TLS handshake data can expose the target domain in SNI (Server Name Indication).  
This engine uses SNI for domain/app identification.

---

## 3) Project Structure

~~~text
Packet_analyzer/
├── include/
│   ├── connection_tracker.h
│   ├── dpi_engine.h
│   ├── fast_path.h
│   ├── load_balancer.h
│   ├── ml_traffic_classifier.h
│   ├── packet_parser.h
│   ├── pcap_reader.h
│   ├── platform.h
│   ├── rule_manager.h
│   ├── sni_extractor.h
│   ├── thread_safe_queue.h
│   └── types.h
├── src/
│   ├── connection_tracker.cpp
│   ├── dpi_engine.cpp
│   ├── dpi_mt.cpp
│   ├── fast_path.cpp
│   ├── load_balancer.cpp
│   ├── main.cpp
│   ├── main_dpi.cpp
│   ├── main_simple.cpp
│   ├── main_working.cpp
│   ├── ml_traffic_classifier.cpp
│   ├── packet_parser.cpp
│   ├── pcap_reader.cpp
│   ├── rule_manager.cpp
│   ├── sni_extractor.cpp
│   └── types.cpp
├── models/
│   └── traffic_dt_model.txt
├── blocking_rules_demo.txt
├── generate_test_pcap.py
├── train_ml_classifier.py
├── run_dpi.ps1
├── CMakeLists.txt
└── README.md
~~~

Main entry for full engine:

- src/main_dpi.cpp

---

## 4) How Packet Processing Works

### Step 1: Read packets

pcap_reader opens input PCAP and reads packets one by one.

### Step 2: Parse headers

packet_parser extracts:

- IP addresses
- ports
- protocol
- payload offsets

### Step 3: Build flow key

Engine builds 5-tuple and looks up tracked connection state.

### Step 4: Inspect payload

fast_path + sni_extractor inspect:

- TLS SNI
- HTTP Host
- basic protocol hints

### Step 5: Classify flow

Flow gets an AppType label (heuristic and optional ML).

### Step 6: Apply rules

rule_manager checks rules in this order:

1. source IP
2. destination port
3. app label
4. domain

If any match, packet is dropped.

### Step 7: Write output

Allowed packets are written to output PCAP.

### Step 8: Report

Engine prints stats and app distribution.

---

## 5) Single-Thread vs Multi-Thread Paths

You will see multiple main files in src from earlier versions.

Current full path is main_dpi.cpp with multi-thread architecture.

Pipeline:

~~~text
Reader -> Load Balancers -> Fast Paths -> Output writer
~~~

This keeps same-flow packets on the same processing path, which is required for correct flow state and consistent blocking.

---

## 6) Deep Dive by Component

### pcap_reader

- Reads PCAP global header and packet records
- Validates basic format

### packet_parser

- Parses Ethernet/IPv4/TCP/UDP headers
- Computes payload boundaries safely

### connection_tracker

- Tracks active connections
- Updates packet/byte counters and timestamps

### sni_extractor

- Extracts TLS SNI from Client Hello
- Extracts HTTP Host when available

### rule_manager

- Stores blocked IPs/apps/domains/ports
- Loads and saves rule files
- Central shouldBlock decision

### fast_path

- Per-thread packet processing
- Classification + rule check
- Drop or forward action

### load_balancer

- Hash-based routing from input to worker queues

### dpi_engine

- Orchestrates initialization, threads, shutdown
- Loads rules and model
- Prints final summary

### ml_traffic_classifier

- Loads text decision-tree model
- Builds features
- Predicts AppType label (label-only, no confidence score)

### types

- Core structures: FiveTuple, PacketJob, Connection, AppType
- String mapping helpers

---

## 7) Classification (Heuristic + ML)

Classification is per connection flow.

Labels include:

- Generic: Unknown, HTTP, HTTPS, DNS, TLS, QUIC
- Service/app labels: Google, YouTube, Facebook, Instagram, Twitter/X, Netflix, Amazon, Microsoft, Apple, WhatsApp, Telegram, TikTok, Spotify, Zoom, Discord, GitHub, Cloudflare

Default model path:

- models/traffic_dt_model.txt

---

## 8) Blocking Rules

Supported rule types:

- IP block
- App block
- Domain block (exact or wildcard)
- Port block

Example wildcard domain:

- *.youtube.com

---

## 9) Build and Run

## Option A: CMake (recommended)

From Packet_analyzer folder:

~~~bash
cmake -S . -B build
cmake --build build --config Release
~~~

Typical binaries:

- Linux/macOS: build/dpi_engine
- Windows: build/dpi_engine.exe

## Option B: manual g++ build

~~~bash
g++ -std=c++17 -pthread -O2 -I include -o dpi_engine \
    src/main_dpi.cpp \
    src/dpi_engine.cpp \
    src/load_balancer.cpp \
    src/fast_path.cpp \
    src/connection_tracker.cpp \
    src/rule_manager.cpp \
    src/ml_traffic_classifier.cpp \
    src/pcap_reader.cpp \
    src/packet_parser.cpp \
    src/sni_extractor.cpp \
    src/types.cpp
~~~

---

## 10) Testing Scenarios

This section gives direct test commands for all common conditions.

### A) Normal run (no blocking)

~~~bash
./build/dpi_engine test_dpi.pcap output_normal.pcap 200
~~~

Expected:

- Dropped/Blocked close to 0 (unless default rules loaded elsewhere)

### B) Run with ML model

~~~bash
./build/dpi_engine test_dpi.pcap output_ml.pcap 200 --ml-model models/traffic_dt_model.txt
~~~

Expected:

- Classification report appears at end

### C) Run with demo blocking rules file

~~~bash
./build/dpi_engine test_dpi.pcap output_blocked.pcap 200 --ml-model models/traffic_dt_model.txt --rules blocking_rules_demo.txt
~~~

Expected:

- Rules loaded lines
- BLOCKED packet lines
- Dropped/Blocked greater than 0

### D) Run with inline blocking flags

~~~bash
./build/dpi_engine test_dpi.pcap output_inline_block.pcap 200 \
  --block-app DNS \
  --block-domain *.google.com \
  --block-ip 192.168.1.50
~~~

### E) Run with custom thread config

~~~bash
./build/dpi_engine test_dpi.pcap output_threads.pcap 200 --lbs 4 --fps 2
~~~

### F) Windows run from Packet_analyzer (recommended script)

~~~powershell
powershell -ExecutionPolicy Bypass -File .\run_dpi.ps1 `
  -InputPcap test_dpi.pcap `
  -OutputPcap out.pcap `
  -MaxPackets 200 `
  -RulesFile blocking_rules_demo.txt
~~~

### G) Windows run from parent DPI folder

~~~powershell
powershell -ExecutionPolicy Bypass -File .\run_dpi.ps1 `
  -InputPcap test_dpi.pcap `
  -OutputPcap out.pcap `
  -MaxPackets 200 `
  -RulesFile blocking_rules_demo.txt
~~~

### H) Quick compare baseline vs blocking

~~~powershell
# baseline
powershell -ExecutionPolicy Bypass -File .\run_dpi.ps1 -InputPcap test_dpi.pcap -OutputPcap out_base.pcap -MaxPackets 200

# with rules
powershell -ExecutionPolicy Bypass -File .\run_dpi.ps1 -InputPcap test_dpi.pcap -OutputPcap out_blocked.pcap -MaxPackets 200 -RulesFile blocking_rules_demo.txt
~~~

Then compare final stats:

- Forwarded
- Dropped/Blocked
- Blocked Apps/Domains/Ports counts

---

## 11) Rules File Format

Use INI-style sections exactly as below:

~~~ini
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
~~~

Demo file:

- blocking_rules_demo.txt

---

## 12) Output and How to Verify It

At runtime, check for:

- startup banner
- model loaded line
- rules loaded line
- processing summary
- classification report

Blocking is active when all 3 are true:

1. Rule loading shown:
   - RuleManager loaded message
2. Runtime block lines shown:
   - BLOCKED packet messages
3. Final summary shows:
   - Dropped/Blocked greater than 0
   - blocked rule counters non-zero as expected

File verification examples:

### PowerShell

~~~powershell
Get-Item .\out.pcap | Select Name,Length,LastWriteTime
~~~

### Bash

~~~bash
ls -lh out.pcap
~~~

---

## 13) Troubleshooting

### Output not readable in Windows terminal

Use run_dpi.ps1. It sets UTF-8 console mode.

### Rules not taking effect

Check:

- RulesFile path is correct
- section names are exact
- terminal says rules loaded
- your rules actually match traffic in test file

### Model not loaded

Check:

- model path is correct
- file exists at models/traffic_dt_model.txt
- terminal prints ML model loaded line

### Build succeeds but run fails

Check:

- correct working directory
- input PCAP exists
- output path is writable

---

## 14) Notes for Extension

Possible next improvements:

- add more domain-to-app mappings
- improve ML features and retrain model
- add IPv6 parsing
- add unit tests for parser/rules/classifier
- add live dashboard for stats

---

## Quick Start

~~~powershell
# from Packet_analyzer
cmake -S . -B build
cmake --build build --config Release

powershell -ExecutionPolicy Bypass -File .\run_dpi.ps1 `
  -InputPcap test_dpi.pcap `
  -OutputPcap out.pcap `
  -MaxPackets 200 `
  -RulesFile blocking_rules_demo.txt


If you want, I can also give a second one-page README version for recruiters while keeping this as the full technical README.If you want, I can also give a second one-page README version for recruiters while keeping this as the full technical README.
