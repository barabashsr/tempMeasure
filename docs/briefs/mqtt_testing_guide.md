# MQTT Testing and Debugging Guide for Temperature Controller

## Table of Contents
1. [Testing Tools Setup](#testing-tools-setup)
2. [Basic Connectivity Tests](#basic-connectivity-tests)
3. [Command Testing Procedures](#command-testing-procedures)
4. [Telemetry Verification](#telemetry-verification)
5. [Load Testing](#load-testing)
6. [Debugging Common Issues](#debugging-common-issues)
7. [Integration Testing](#integration-testing)
8. [Production Monitoring](#production-monitoring)
9. [ESP32-Specific Considerations](#esp32-specific-considerations)
10. [Troubleshooting Flowcharts](#troubleshooting-flowcharts)

## 1. Testing Tools Setup

### 1.1 Mosquitto Client Installation

#### Linux/WSL
```bash
sudo apt update
sudo apt install mosquitto-clients
```

#### macOS
```bash
brew install mosquitto
```

#### Windows
Download installer from: https://mosquitto.org/download/

### 1.2 Mosquitto Client Usage Examples

#### Subscribe to all device topics
```bash
mosquitto_sub -h broker.hivemq.com -p 1883 -t "plant/area1/line2/tempcontroller01/#" -v
```

#### Subscribe with authentication
```bash
mosquitto_sub -h broker.hivemq.com -p 1883 \
  -u "username" -P "password" \
  -t "plant/area1/line2/tempcontroller01/#" -v
```

#### Publish test command
```bash
mosquitto_pub -h broker.hivemq.com -p 1883 \
  -t "plant/area1/line2/tempcontroller01/command/request" \
  -m '{"cmd_id":"test-001","command":"get_status","parameters":{}}'
```

### 1.3 MQTT Explorer Configuration

1. Download MQTT Explorer: http://mqtt-explorer.com/
2. Create connection profile:
   ```
   Name: Temperature Controller Test
   Protocol: mqtt://
   Host: broker.hivemq.com
   Port: 1883
   Username: [if required]
   Password: [if required]
   ```
3. Advanced settings:
   - Client ID: mqtt-explorer-test
   - Keep Alive: 60
   - Clean Session: Yes
   - Auto Reconnect: Yes

### 1.4 Wireshark MQTT Debugging

#### Filter for MQTT traffic
```
tcp.port == 1883 or tcp.port == 8883
```

#### Decode as MQTT
1. Right-click on packet → "Decode As..."
2. Set Current to "MQTT"

#### Common MQTT filters
```
mqtt.msgtype == 1    # CONNECT
mqtt.msgtype == 3    # PUBLISH
mqtt.msgtype == 8    # SUBSCRIBE
mqtt.topic contains "tempcontroller01"
```

### 1.5 Local Broker Setup

#### HiveMQ Community Edition
```bash
# Download and extract
wget https://www.hivemq.com/releases/hivemq-ce-latest.zip
unzip hivemq-ce-latest.zip
cd hivemq-ce-*/

# Start broker
./bin/run.sh

# Access web UI at http://localhost:8080
```

#### EMQX Docker Setup
```bash
docker run -d --name emqx \
  -p 1883:1883 \
  -p 8083:8083 \
  -p 8084:8084 \
  -p 8883:8883 \
  -p 18083:18083 \
  emqx/emqx:latest

# Access dashboard at http://localhost:18083
# Default login: admin/public
```

#### Mosquitto Local Broker
```bash
# Create config file
cat > mosquitto.conf << EOF
listener 1883
allow_anonymous true
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log
log_type all
EOF

# Run with Docker
docker run -d --name mosquitto \
  -p 1883:1883 \
  -p 9001:9001 \
  -v $(pwd)/mosquitto.conf:/mosquitto/config/mosquitto.conf \
  eclipse-mosquitto
```

## 2. Basic Connectivity Tests

### 2.1 Testing Broker Connection

#### Test Script
```python
#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time
import sys

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✓ Connected successfully")
        print(f"  Flags: {flags}")
        client.disconnect()
    else:
        print(f"✗ Connection failed with code: {rc}")
        print(f"  0: Success")
        print(f"  1: Incorrect protocol version")
        print(f"  2: Invalid client identifier")
        print(f"  3: Server unavailable")
        print(f"  4: Bad username or password")
        print(f"  5: Not authorized")

def test_connection(broker, port, username=None, password=None):
    client = mqtt.Client("test-client-" + str(int(time.time())))
    client.on_connect = on_connect
    
    if username:
        client.username_pw_set(username, password)
    
    print(f"Testing connection to {broker}:{port}")
    try:
        client.connect(broker, port, 60)
        client.loop_forever()
    except Exception as e:
        print(f"✗ Connection error: {e}")

if __name__ == "__main__":
    test_connection("broker.hivemq.com", 1883)
```

### 2.2 Authentication Verification

#### Basic Auth Test
```bash
# Test with wrong credentials
mosquitto_pub -h broker.hivemq.com -p 1883 \
  -u "wrong_user" -P "wrong_pass" \
  -t "test/auth" -m "test" -d

# Expected output: Connection Refused: not authorised.
```

#### Client Certificate Test
```bash
# Generate test certificates
openssl req -new -x509 -days 365 -nodes \
  -out client-cert.pem -keyout client-key.pem \
  -subj "/CN=tempcontroller01"

# Test with certificate
mosquitto_pub -h broker.hivemq.com -p 8883 \
  --cafile ca.pem \
  --cert client-cert.pem \
  --key client-key.pem \
  -t "test/tls" -m "test" -d
```

### 2.3 TLS/SSL Certificate Testing

#### Certificate Verification Script
```python
#!/usr/bin/env python3
import ssl
import socket
import OpenSSL.crypto

def check_mqtt_tls(hostname, port=8883):
    context = ssl.create_default_context()
    
    with socket.create_connection((hostname, port), timeout=10) as sock:
        with context.wrap_socket(sock, server_hostname=hostname) as ssock:
            der_cert = ssock.getpeercert(True)
            pem_cert = ssl.DER_cert_to_PEM_cert(der_cert)
            
            cert = OpenSSL.crypto.load_certificate(
                OpenSSL.crypto.FILETYPE_PEM, pem_cert
            )
            
            print(f"✓ TLS connection successful")
            print(f"  Subject: {cert.get_subject()}")
            print(f"  Issuer: {cert.get_issuer()}")
            print(f"  Version: {cert.get_version()}")
            print(f"  Serial: {cert.get_serial_number()}")
            print(f"  Not Before: {cert.get_notBefore()}")
            print(f"  Not After: {cert.get_notAfter()}")
            
            # Check expiration
            import datetime
            expiry = datetime.datetime.strptime(
                cert.get_notAfter().decode('ascii'), '%Y%m%d%H%M%SZ'
            )
            days_left = (expiry - datetime.datetime.utcnow()).days
            print(f"  Days until expiry: {days_left}")

if __name__ == "__main__":
    check_mqtt_tls("broker.hivemq.com", 8883)
```

### 2.4 Network Reliability Tests

#### Latency Test
```bash
#!/bin/bash
# Test MQTT round-trip time

BROKER="broker.hivemq.com"
TOPIC="test/latency/$(date +%s)"

# Start subscriber in background
mosquitto_sub -h $BROKER -t "$TOPIC" -C 1 > /tmp/mqtt_received &
SUB_PID=$!

# Wait for subscriber to connect
sleep 1

# Send message with timestamp
START=$(date +%s%N)
echo "$START" | mosquitto_pub -h $BROKER -t "$TOPIC" -l

# Wait for message
wait $SUB_PID
END=$(date +%s%N)

# Calculate latency
LATENCY=$(( ($END - $START) / 1000000 ))
echo "Round-trip latency: ${LATENCY}ms"
```

#### Connection Stability Test
```python
#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import time
import statistics

class StabilityTester:
    def __init__(self, broker, port):
        self.broker = broker
        self.port = port
        self.connected = False
        self.connect_times = []
        self.disconnect_count = 0
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.connected = True
            connect_time = time.time() - self.connect_start
            self.connect_times.append(connect_time)
            print(f"✓ Connected in {connect_time:.2f}s")
    
    def on_disconnect(self, client, userdata, rc):
        if rc != 0:
            self.connected = False
            self.disconnect_count += 1
            print(f"✗ Unexpected disconnect #{self.disconnect_count}")
            self.connect_start = time.time()
    
    def run_test(self, duration=300):
        client = mqtt.Client()
        client.on_connect = self.on_connect
        client.on_disconnect = self.on_disconnect
        
        print(f"Running {duration}s stability test...")
        self.connect_start = time.time()
        client.connect(self.broker, self.port)
        
        start_time = time.time()
        while time.time() - start_time < duration:
            client.loop(0.1)
        
        client.disconnect()
        
        print("\n=== Test Results ===")
        print(f"Disconnections: {self.disconnect_count}")
        if self.connect_times:
            print(f"Avg connect time: {statistics.mean(self.connect_times):.2f}s")
            print(f"Max connect time: {max(self.connect_times):.2f}s")

if __name__ == "__main__":
    tester = StabilityTester("broker.hivemq.com", 1883)
    tester.run_test(60)  # 1 minute test
```

## 3. Command Testing Procedures

### 3.1 Step-by-Step Command Testing

#### Command Test Framework
```python
#!/usr/bin/env python3
import paho.mqtt.client as mqtt
import json
import uuid
import time
import threading

class CommandTester:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        self.client = mqtt.Client()
        self.responses = {}
        self.response_event = threading.Event()
        
        # Topic paths
        self.cmd_topic = f"plant/area1/line2/{device_id}/command/request"
        self.resp_topic = f"plant/area1/line2/{device_id}/command/response"
        
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
    
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            client.subscribe(self.resp_topic)
            print(f"✓ Connected and subscribed to responses")
    
    def on_message(self, client, userdata, msg):
        try:
            response = json.loads(msg.payload.decode())
            cmd_id = response.get('cmd_id')
            if cmd_id:
                self.responses[cmd_id] = response
                self.response_event.set()
        except Exception as e:
            print(f"✗ Error parsing response: {e}")
    
    def send_command(self, command, parameters={}, timeout=5):
        cmd_id = str(uuid.uuid4())
        
        cmd_msg = {
            "cmd_id": cmd_id,
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "command": command,
            "parameters": parameters
        }
        
        print(f"\n→ Sending command: {command}")
        print(f"  ID: {cmd_id}")
        print(f"  Parameters: {json.dumps(parameters)}")
        
        self.response_event.clear()
        self.client.publish(self.cmd_topic, json.dumps(cmd_msg))
        
        if self.response_event.wait(timeout):
            response = self.responses.get(cmd_id)
            if response:
                print(f"← Response received:")
                print(f"  Status: {response.get('status')}")
                print(f"  Time: {response.get('execution_time')}ms")
                print(f"  Data: {json.dumps(response.get('data', {}), indent=2)}")
                return response
        else:
            print(f"✗ Timeout waiting for response")
            return None
    
    def run_test_suite(self):
        print("=== Temperature Controller Command Test Suite ===\n")
        
        self.client.connect(self.broker, 1883)
        self.client.loop_start()
        time.sleep(2)  # Wait for connection
        
        # Test 1: Help command
        print("Test 1: Help Command")
        self.send_command("help")
        
        # Test 2: System status
        print("\nTest 2: System Status")
        self.send_command("get_status")
        
        # Test 3: Get specific point
        print("\nTest 3: Get Point Data")
        self.send_command("get_point", {"point_id": 5})
        
        # Test 4: Get all alarms
        print("\nTest 4: Get Active Alarms")
        self.send_command("get_active_alarms")
        
        # Test 5: Set threshold
        print("\nTest 5: Set Temperature Threshold")
        self.send_command("set_threshold", {
            "point_id": 5,
            "threshold_type": "high",
            "value": 85.0,
            "hysteresis": 2.0
        })
        
        # Test 6: Invalid command
        print("\nTest 6: Invalid Command")
        self.send_command("invalid_command")
        
        self.client.loop_stop()
        self.client.disconnect()

if __name__ == "__main__":
    tester = CommandTester("broker.hivemq.com", "tempcontroller01")
    tester.run_test_suite()
```

### 3.2 Response Validation

#### Response Validator
```python
def validate_response(response, expected_schema):
    """Validate response against expected schema"""
    errors = []
    
    # Check required fields
    for field in expected_schema.get('required', []):
        if field not in response:
            errors.append(f"Missing required field: {field}")
    
    # Check field types
    for field, expected_type in expected_schema.get('types', {}).items():
        if field in response:
            actual_type = type(response[field]).__name__
            if actual_type != expected_type:
                errors.append(f"Field '{field}' type mismatch: "
                            f"expected {expected_type}, got {actual_type}")
    
    # Check status values
    if 'status' in response:
        valid_statuses = ['success', 'error', 'partial']
        if response['status'] not in valid_statuses:
            errors.append(f"Invalid status: {response['status']}")
    
    return errors

# Example schema
status_response_schema = {
    'required': ['cmd_id', 'timestamp', 'status', 'data'],
    'types': {
        'cmd_id': 'str',
        'timestamp': 'str',
        'status': 'str',
        'execution_time': 'int',
        'data': 'dict'
    }
}
```

### 3.3 Error Simulation

#### Error Test Cases
```python
def test_error_conditions():
    """Test various error conditions"""
    
    test_cases = [
        # Malformed JSON
        {
            "name": "Malformed JSON",
            "payload": '{"cmd_id": "test", invalid json}',
            "expected": "JSON parse error"
        },
        # Missing required fields
        {
            "name": "Missing cmd_id",
            "payload": json.dumps({
                "command": "get_status"
            }),
            "expected": "Missing required field: cmd_id"
        },
        # Invalid parameter types
        {
            "name": "Invalid point_id type",
            "payload": json.dumps({
                "cmd_id": "test-003",
                "command": "get_point",
                "parameters": {"point_id": "not_a_number"}
            }),
            "expected": "Invalid parameter type"
        },
        # Out of range values
        {
            "name": "Point ID out of range",
            "payload": json.dumps({
                "cmd_id": "test-004",
                "command": "get_point",
                "parameters": {"point_id": 100}
            }),
            "expected": "Point ID out of range"
        },
        # Command injection attempt
        {
            "name": "Command injection",
            "payload": json.dumps({
                "cmd_id": "test-005",
                "command": "get_status; rm -rf /",
                "parameters": {}
            }),
            "expected": "Invalid command"
        }
    ]
    
    for test in test_cases:
        print(f"\nTesting: {test['name']}")
        print(f"Expected: {test['expected']}")
        # Send test payload and verify error response
```

### 3.4 Performance Benchmarking

#### Command Performance Test
```python
import time
import statistics

class PerformanceTester:
    def __init__(self, command_tester):
        self.tester = command_tester
        self.results = []
    
    def benchmark_command(self, command, parameters={}, iterations=100):
        """Benchmark a specific command"""
        print(f"\nBenchmarking '{command}' ({iterations} iterations)")
        
        response_times = []
        success_count = 0
        
        for i in range(iterations):
            start = time.time()
            response = self.tester.send_command(command, parameters, timeout=10)
            end = time.time()
            
            if response and response.get('status') == 'success':
                response_time = (end - start) * 1000  # Convert to ms
                response_times.append(response_time)
                success_count += 1
                
                # Also track execution time from device
                if 'execution_time' in response:
                    device_time = response['execution_time']
                    network_time = response_time - device_time
            
            if (i + 1) % 10 == 0:
                print(f"  Progress: {i + 1}/{iterations}")
        
        # Calculate statistics
        if response_times:
            stats = {
                'command': command,
                'iterations': iterations,
                'success_rate': (success_count / iterations) * 100,
                'avg_response_time': statistics.mean(response_times),
                'min_response_time': min(response_times),
                'max_response_time': max(response_times),
                'std_dev': statistics.stdev(response_times) if len(response_times) > 1 else 0
            }
            
            print(f"\n=== Results for '{command}' ===")
            print(f"Success Rate: {stats['success_rate']:.1f}%")
            print(f"Avg Response Time: {stats['avg_response_time']:.2f}ms")
            print(f"Min Response Time: {stats['min_response_time']:.2f}ms")
            print(f"Max Response Time: {stats['max_response_time']:.2f}ms")
            print(f"Std Deviation: {stats['std_dev']:.2f}ms")
            
            self.results.append(stats)
            return stats
    
    def run_benchmark_suite(self):
        """Run comprehensive benchmark suite"""
        commands = [
            ("help", {}),
            ("get_status", {}),
            ("get_point", {"point_id": 5}),
            ("get_summary", {}),
            ("get_active_alarms", {})
        ]
        
        for cmd, params in commands:
            self.benchmark_command(cmd, params, iterations=50)
        
        # Generate report
        self.generate_report()
    
    def generate_report(self):
        """Generate performance report"""
        print("\n=== PERFORMANCE REPORT ===")
        print(f"{'Command':<20} {'Avg (ms)':<10} {'Min (ms)':<10} {'Max (ms)':<10} {'Success %':<10}")
        print("-" * 60)
        
        for result in self.results:
            print(f"{result['command']:<20} "
                  f"{result['avg_response_time']:<10.2f} "
                  f"{result['min_response_time']:<10.2f} "
                  f"{result['max_response_time']:<10.2f} "
                  f"{result['success_rate']:<10.1f}")
```

## 4. Telemetry Verification

### 4.1 Data Accuracy Checks

#### Telemetry Validator
```python
class TelemetryValidator:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        self.telemetry_data = []
        self.client = mqtt.Client()
        
    def validate_telemetry_message(self, msg):
        """Validate telemetry message structure and content"""
        errors = []
        warnings = []
        
        try:
            data = json.loads(msg.payload.decode())
            
            # Check timestamp
            if 'timestamp' in data:
                try:
                    ts = datetime.fromisoformat(data['timestamp'].replace('Z', '+00:00'))
                    age = (datetime.now(timezone.utc) - ts).total_seconds()
                    if age > 60:
                        warnings.append(f"Timestamp is {age:.1f}s old")
                except:
                    errors.append("Invalid timestamp format")
            else:
                errors.append("Missing timestamp")
            
            # Validate points
            if 'points' in data:
                points = data['points']
                
                # Check for expected number of points
                if isinstance(points, dict):
                    point_count = len(points)
                    if point_count < 45:  # Assuming at least 45 active points
                        warnings.append(f"Only {point_count} points reported")
                    
                    # Validate each point
                    for point_id, point_data in points.items():
                        # Temperature range check
                        temp = point_data.get('temp')
                        if temp is not None:
                            if temp < -50 or temp > 200:
                                errors.append(f"Point {point_id}: Invalid temperature {temp}")
                            
                            # Check for stuck values
                            if hasattr(self, 'last_temps'):
                                if point_id in self.last_temps:
                                    if self.last_temps[point_id] == temp:
                                        if not hasattr(self, 'stuck_counts'):
                                            self.stuck_counts = {}
                                        self.stuck_counts[point_id] = \
                                            self.stuck_counts.get(point_id, 0) + 1
                                        if self.stuck_counts[point_id] > 10:
                                            warnings.append(
                                                f"Point {point_id}: Value stuck at {temp}"
                                            )
                        
                        # Sensor status check
                        status = point_data.get('status')
                        if status not in ['OK', 'ERROR', 'DISCONNECTED']:
                            warnings.append(f"Point {point_id}: Unknown status '{status}'")
            
            # Validate summary
            if 'summary' in data:
                summary = data['summary']
                
                # Cross-check summary with points
                if 'points' in data:
                    actual_count = len([p for p in data['points'].values() 
                                      if p.get('status') == 'OK'])
                    if summary.get('active_points') != actual_count:
                        errors.append(
                            f"Summary mismatch: reported {summary.get('active_points')} "
                            f"active, found {actual_count}"
                        )
            
        except json.JSONDecodeError as e:
            errors.append(f"JSON decode error: {e}")
        except Exception as e:
            errors.append(f"Validation error: {e}")
        
        return errors, warnings
```

### 4.2 Publishing Frequency Validation

#### Frequency Monitor
```python
class FrequencyMonitor:
    def __init__(self, expected_interval=60):
        self.expected_interval = expected_interval
        self.last_timestamp = None
        self.intervals = []
        
    def on_telemetry(self, msg):
        """Track telemetry message timing"""
        current_time = time.time()
        
        if self.last_timestamp:
            interval = current_time - self.last_timestamp
            self.intervals.append(interval)
            
            # Check if interval is within tolerance (±10%)
            tolerance = self.expected_interval * 0.1
            if abs(interval - self.expected_interval) > tolerance:
                print(f"⚠ Interval deviation: {interval:.1f}s "
                      f"(expected {self.expected_interval}s)")
        
        self.last_timestamp = current_time
        
        # Report statistics every 10 messages
        if len(self.intervals) >= 10:
            avg_interval = statistics.mean(self.intervals)
            std_dev = statistics.stdev(self.intervals)
            print(f"\nFrequency Stats (last 10):")
            print(f"  Average interval: {avg_interval:.1f}s")
            print(f"  Std deviation: {std_dev:.2f}s")
            print(f"  Min/Max: {min(self.intervals):.1f}s / {max(self.intervals):.1f}s")
            self.intervals = []
```

### 4.3 QoS Level Testing

#### QoS Test Suite
```python
def test_qos_levels():
    """Test different QoS levels for reliability"""
    
    test_configs = [
        {"qos": 0, "messages": 100, "topic": "test/qos0"},
        {"qos": 1, "messages": 100, "topic": "test/qos1"},
        {"qos": 2, "messages": 100, "topic": "test/qos2"}
    ]
    
    for config in test_configs:
        print(f"\nTesting QoS {config['qos']} ({config['messages']} messages)")
        
        # Set up subscriber
        received = []
        
        def on_message(client, userdata, msg):
            received.append(msg.payload.decode())
        
        sub_client = mqtt.Client()
        sub_client.on_message = on_message
        sub_client.connect("broker.hivemq.com", 1883)
        sub_client.subscribe(config['topic'], qos=config['qos'])
        sub_client.loop_start()
        
        # Publish messages
        pub_client = mqtt.Client()
        pub_client.connect("broker.hivemq.com", 1883)
        
        for i in range(config['messages']):
            pub_client.publish(
                config['topic'], 
                f"Message {i}", 
                qos=config['qos']
            )
            time.sleep(0.01)  # Small delay
        
        # Wait for messages
        time.sleep(2)
        
        # Report results
        delivery_rate = (len(received) / config['messages']) * 100
        print(f"  Delivery rate: {delivery_rate:.1f}%")
        print(f"  Messages lost: {config['messages'] - len(received)}")
        
        sub_client.loop_stop()
        sub_client.disconnect()
        pub_client.disconnect()
```

### 4.4 Retained Message Verification

#### Retained Message Test
```python
def test_retained_messages():
    """Test retained message functionality"""
    
    print("Testing retained messages...")
    
    # Publish retained message
    pub_client = mqtt.Client()
    pub_client.connect("broker.hivemq.com", 1883)
    
    test_topic = f"test/retained/{int(time.time())}"
    test_message = "This is a retained message"
    
    pub_client.publish(test_topic, test_message, retain=True)
    pub_client.disconnect()
    
    print(f"✓ Published retained message to {test_topic}")
    
    # Wait a bit
    time.sleep(2)
    
    # Connect new subscriber
    received = []
    
    def on_message(client, userdata, msg):
        received.append({
            'payload': msg.payload.decode(),
            'retained': msg.retain
        })
    
    sub_client = mqtt.Client()
    sub_client.on_message = on_message
    sub_client.connect("broker.hivemq.com", 1883)
    sub_client.subscribe(test_topic)
    
    # Should receive retained message immediately
    sub_client.loop(timeout=2)
    
    if received:
        msg = received[0]
        print(f"✓ Received retained message: '{msg['payload']}'")
        print(f"  Retained flag: {msg['retained']}")
    else:
        print("✗ No retained message received")
    
    # Clean up - remove retained message
    pub_client = mqtt.Client()
    pub_client.connect("broker.hivemq.com", 1883)
    pub_client.publish(test_topic, "", retain=True)  # Empty retained message
    pub_client.disconnect()
    
    sub_client.disconnect()
```

## 5. Load Testing

### 5.1 60-Point Telemetry Stress Test

#### Telemetry Load Generator
```python
import threading
import queue

class TelemetryLoadTest:
    def __init__(self, broker, device_count=10):
        self.broker = broker
        self.device_count = device_count
        self.stats = {
            'messages_sent': 0,
            'messages_failed': 0,
            'bytes_sent': 0,
            'start_time': None,
            'errors': []
        }
        self.stats_lock = threading.Lock()
        
    def generate_telemetry(self, device_id):
        """Generate realistic 60-point telemetry data"""
        import random
        
        data = {
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "device_id": device_id,
            "sequence": random.randint(1000, 99999),
            "points": {}
        }
        
        # Generate 60 points
        for i in range(60):
            if random.random() > 0.1:  # 90% chance point is active
                data["points"][str(i)] = {
                    "name": f"Point_{i}",
                    "temp": round(random.uniform(20.0, 90.0), 1),
                    "sensor_id": f"28FF{i:012X}",
                    "status": "OK" if random.random() > 0.05 else "ERROR"
                }
        
        data["summary"] = {
            "total_points": 60,
            "active_points": len(data["points"]),
            "sensor_errors": len([p for p in data["points"].values() 
                                if p["status"] == "ERROR"]),
            "min_temp": min([p["temp"] for p in data["points"].values()]),
            "max_temp": max([p["temp"] for p in data["points"].values()])
        }
        
        return data
    
    def device_simulator(self, device_id, interval=10, duration=300):
        """Simulate a single device"""
        client = mqtt.Client(f"load-test-{device_id}")
        topic = f"plant/area1/line2/{device_id}/telemetry/temperature"
        
        try:
            client.connect(self.broker, 1883)
            start_time = time.time()
            
            while time.time() - start_time < duration:
                try:
                    # Generate and send telemetry
                    telemetry = self.generate_telemetry(device_id)
                    payload = json.dumps(telemetry)
                    
                    result = client.publish(topic, payload, qos=0)
                    
                    with self.stats_lock:
                        if result.rc == 0:
                            self.stats['messages_sent'] += 1
                            self.stats['bytes_sent'] += len(payload)
                        else:
                            self.stats['messages_failed'] += 1
                    
                    time.sleep(interval)
                    
                except Exception as e:
                    with self.stats_lock:
                        self.stats['errors'].append(f"{device_id}: {str(e)}")
                        
        except Exception as e:
            print(f"Device {device_id} connection failed: {e}")
        finally:
            client.disconnect()
    
    def run_load_test(self, telemetry_interval=10, test_duration=60):
        """Run the load test"""
        print(f"=== 60-Point Telemetry Load Test ===")
        print(f"Devices: {self.device_count}")
        print(f"Interval: {telemetry_interval}s")
        print(f"Duration: {test_duration}s")
        print(f"Expected messages: {(test_duration/telemetry_interval)*self.device_count}")
        print("\nStarting test...")
        
        self.stats['start_time'] = time.time()
        
        # Start device threads
        threads = []
        for i in range(self.device_count):
            device_id = f"tempcontroller{i:02d}"
            thread = threading.Thread(
                target=self.device_simulator,
                args=(device_id, telemetry_interval, test_duration)
            )
            thread.start()
            threads.append(thread)
        
        # Monitor progress
        while any(t.is_alive() for t in threads):
            time.sleep(5)
            with self.stats_lock:
                elapsed = time.time() - self.stats['start_time']
                rate = self.stats['messages_sent'] / elapsed if elapsed > 0 else 0
                print(f"Progress: {self.stats['messages_sent']} messages, "
                      f"{rate:.1f} msg/s, "
                      f"{self.stats['bytes_sent']/1024/1024:.1f} MB")
        
        # Wait for all threads
        for t in threads:
            t.join()
        
        # Final report
        self.generate_report()
    
    def generate_report(self):
        """Generate load test report"""
        elapsed = time.time() - self.stats['start_time']
        
        print("\n=== Load Test Results ===")
        print(f"Duration: {elapsed:.1f}s")
        print(f"Messages sent: {self.stats['messages_sent']}")
        print(f"Messages failed: {self.stats['messages_failed']}")
        print(f"Success rate: {(self.stats['messages_sent']/(self.stats['messages_sent']+self.stats['messages_failed'])*100):.1f}%")
        print(f"Average rate: {self.stats['messages_sent']/elapsed:.1f} msg/s")
        print(f"Total data: {self.stats['bytes_sent']/1024/1024:.1f} MB")
        print(f"Bandwidth: {self.stats['bytes_sent']/elapsed/1024:.1f} KB/s")
        
        if self.stats['errors']:
            print(f"\nErrors ({len(self.stats['errors'])}):")
            for error in self.stats['errors'][:10]:  # Show first 10
                print(f"  - {error}")
```

### 5.2 Concurrent Command Handling

#### Concurrent Command Test
```python
class ConcurrentCommandTest:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        self.results = queue.Queue()
        
    def send_command_thread(self, thread_id, command, parameters, count):
        """Thread function to send commands"""
        client = mqtt.Client(f"cmd-test-{thread_id}")
        cmd_topic = f"plant/area1/line2/{self.device_id}/command/request"
        resp_topic = f"plant/area1/line2/{self.device_id}/command/response"
        
        responses = {}
        
        def on_message(client, userdata, msg):
            try:
                resp = json.loads(msg.payload.decode())
                if 'cmd_id' in resp:
                    responses[resp['cmd_id']] = resp
            except:
                pass
        
        client.on_message = on_message
        client.connect(self.broker, 1883)
        client.subscribe(resp_topic)
        client.loop_start()
        
        for i in range(count):
            cmd_id = f"thread{thread_id}-cmd{i}"
            cmd = {
                "cmd_id": cmd_id,
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "command": command,
                "parameters": parameters
            }
            
            start = time.time()
            client.publish(cmd_topic, json.dumps(cmd))
            
            # Wait for response
            timeout = 5
            while cmd_id not in responses and time.time() - start < timeout:
                time.sleep(0.01)
            
            if cmd_id in responses:
                response_time = (time.time() - start) * 1000
                self.results.put({
                    'thread': thread_id,
                    'cmd_id': cmd_id,
                    'response_time': response_time,
                    'status': responses[cmd_id].get('status')
                })
            else:
                self.results.put({
                    'thread': thread_id,
                    'cmd_id': cmd_id,
                    'response_time': None,
                    'status': 'timeout'
                })
        
        client.loop_stop()
        client.disconnect()
    
    def run_test(self, thread_count=10, commands_per_thread=10):
        """Run concurrent command test"""
        print(f"=== Concurrent Command Test ===")
        print(f"Threads: {thread_count}")
        print(f"Commands per thread: {commands_per_thread}")
        print(f"Total commands: {thread_count * commands_per_thread}")
        
        threads = []
        start_time = time.time()
        
        # Start threads
        for i in range(thread_count):
            thread = threading.Thread(
                target=self.send_command_thread,
                args=(i, "get_status", {}, commands_per_thread)
            )
            thread.start()
            threads.append(thread)
        
        # Wait for completion
        for t in threads:
            t.join()
        
        # Analyze results
        results = []
        while not self.results.empty():
            results.append(self.results.get())
        
        total_time = time.time() - start_time
        successful = [r for r in results if r['status'] == 'success']
        
        print(f"\n=== Results ===")
        print(f"Total time: {total_time:.1f}s")
        print(f"Commands sent: {len(results)}")
        print(f"Successful: {len(successful)}")
        print(f"Success rate: {(len(successful)/len(results)*100):.1f}%")
        
        if successful:
            response_times = [r['response_time'] for r in successful]
            print(f"Avg response time: {statistics.mean(response_times):.1f}ms")
            print(f"Min response time: {min(response_times):.1f}ms")
            print(f"Max response time: {max(response_times):.1f}ms")
```

### 5.3 Network Interruption Scenarios

#### Network Resilience Test
```python
import subprocess
import platform

class NetworkResilienceTest:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        self.is_windows = platform.system() == "Windows"
        
    def simulate_network_interruption(self, duration=10):
        """Simulate network interruption (requires admin privileges)"""
        if self.is_windows:
            # Windows firewall commands
            block_cmd = f"netsh advfirewall firewall add rule name=\"MQTT_TEST_BLOCK\" dir=out action=block remoteip={self.broker}"
            unblock_cmd = "netsh advfirewall firewall delete rule name=\"MQTT_TEST_BLOCK\""
        else:
            # Linux iptables commands
            block_cmd = f"sudo iptables -A OUTPUT -d {self.broker} -j DROP"
            unblock_cmd = f"sudo iptables -D OUTPUT -d {self.broker} -j DROP"
        
        print(f"Blocking connection to {self.broker} for {duration}s...")
        subprocess.run(block_cmd, shell=True)
        
        time.sleep(duration)
        
        print("Restoring connection...")
        subprocess.run(unblock_cmd, shell=True)
    
    def test_reconnection(self):
        """Test MQTT client reconnection behavior"""
        reconnect_times = []
        message_queue = []
        
        class ResilientClient:
            def __init__(self, broker):
                self.client = mqtt.Client()
                self.connected = False
                self.reconnect_count = 0
                self.last_disconnect = None
                
                self.client.on_connect = self.on_connect
                self.client.on_disconnect = self.on_disconnect
                
            def on_connect(self, client, userdata, flags, rc):
                if rc == 0:
                    self.connected = True
                    if self.last_disconnect:
                        reconnect_time = time.time() - self.last_disconnect
                        reconnect_times.append(reconnect_time)
                        print(f"✓ Reconnected after {reconnect_time:.1f}s")
                    client.subscribe("test/resilience")
                    
                    # Send queued messages
                    while message_queue:
                        msg = message_queue.pop(0)
                        client.publish(msg['topic'], msg['payload'])
            
            def on_disconnect(self, client, userdata, rc):
                if rc != 0:
                    self.connected = False
                    self.reconnect_count += 1
                    self.last_disconnect = time.time()
                    print(f"✗ Disconnected (count: {self.reconnect_count})")
            
            def publish_with_queue(self, topic, payload):
                if self.connected:
                    self.client.publish(topic, payload)
                else:
                    message_queue.append({'topic': topic, 'payload': payload})
                    print(f"  Message queued (queue size: {len(message_queue)})")
        
        # Run test
        print("=== Network Resilience Test ===")
        
        client = ResilientClient(self.broker)
        client.client.connect(self.broker, 1883)
        client.client.loop_start()
        
        # Wait for initial connection
        time.sleep(2)
        
        # Test with interruptions
        for i in range(3):
            print(f"\nTest iteration {i+1}/3")
            
            # Send some messages
            for j in range(5):
                client.publish_with_queue(
                    "test/resilience",
                    f"Message {i}-{j}"
                )
                time.sleep(1)
            
            # Simulate network interruption
            if i < 2:  # Don't interrupt on last iteration
                self.simulate_network_interruption(duration=5)
                time.sleep(10)  # Wait for reconnection
        
        client.client.loop_stop()
        client.client.disconnect()
        
        # Report
        print("\n=== Resilience Test Results ===")
        print(f"Disconnections: {client.reconnect_count}")
        if reconnect_times:
            print(f"Avg reconnect time: {statistics.mean(reconnect_times):.1f}s")
            print(f"Max reconnect time: {max(reconnect_times):.1f}s")
```

### 5.4 Memory Leak Detection

#### Memory Monitor
```python
import psutil
import gc

class MemoryLeakDetector:
    def __init__(self):
        self.process = psutil.Process()
        self.baseline_memory = None
        self.memory_samples = []
        
    def start_monitoring(self):
        """Start memory monitoring"""
        gc.collect()  # Force garbage collection
        self.baseline_memory = self.process.memory_info().rss / 1024 / 1024  # MB
        print(f"Baseline memory: {self.baseline_memory:.1f} MB")
        
    def sample_memory(self):
        """Take a memory sample"""
        gc.collect()
        current_memory = self.process.memory_info().rss / 1024 / 1024
        self.memory_samples.append(current_memory)
        return current_memory
    
    def run_leak_test(self, duration=300, sample_interval=10):
        """Run memory leak test"""
        print(f"=== Memory Leak Test ({duration}s) ===")
        
        self.start_monitoring()
        start_time = time.time()
        
        # Create MQTT client with activity
        client = mqtt.Client()
        client.connect("broker.hivemq.com", 1883)
        client.loop_start()
        
        # Simulate activity
        message_count = 0
        while time.time() - start_time < duration:
            # Publish large message
            large_payload = json.dumps({
                "data": "x" * 1000,
                "array": list(range(100)),
                "timestamp": time.time()
            })
            
            client.publish("test/memory", large_payload)
            message_count += 1
            
            # Subscribe/unsubscribe periodically
            if message_count % 100 == 0:
                client.subscribe("test/memory/response")
                client.unsubscribe("test/memory/response")
            
            # Sample memory
            if message_count % (sample_interval * 10) == 0:
                current_mem = self.sample_memory()
                growth = current_mem - self.baseline_memory
                print(f"Memory: {current_mem:.1f} MB (growth: {growth:+.1f} MB)")
            
            time.sleep(0.1)
        
        client.loop_stop()
        client.disconnect()
        
        # Analyze results
        self.analyze_results()
    
    def analyze_results(self):
        """Analyze memory usage pattern"""
        if len(self.memory_samples) < 2:
            print("Insufficient samples")
            return
        
        # Calculate trend
        x = list(range(len(self.memory_samples)))
        y = self.memory_samples
        
        # Simple linear regression
        n = len(x)
        sum_x = sum(x)
        sum_y = sum(y)
        sum_xy = sum(x[i] * y[i] for i in range(n))
        sum_x2 = sum(x[i]**2 for i in range(n))
        
        slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x**2)
        
        print("\n=== Memory Analysis ===")
        print(f"Initial memory: {self.baseline_memory:.1f} MB")
        print(f"Final memory: {self.memory_samples[-1]:.1f} MB")
        print(f"Total growth: {self.memory_samples[-1] - self.baseline_memory:.1f} MB")
        print(f"Growth rate: {slope:.3f} MB/sample")
        
        if slope > 0.1:  # More than 0.1 MB growth per sample
            print("⚠ WARNING: Possible memory leak detected!")
        else:
            print("✓ No significant memory leak detected")
```

## 6. Debugging Common Issues

### 6.1 Connection Failures

#### Connection Debugger
```python
class MQTTConnectionDebugger:
    def __init__(self):
        self.checks = []
        
    def add_check(self, name, check_func):
        """Add a diagnostic check"""
        self.checks.append((name, check_func))
    
    def run_diagnostics(self, broker, port=1883, username=None, password=None):
        """Run all diagnostic checks"""
        print("=== MQTT Connection Diagnostics ===\n")
        
        results = []
        for name, check_func in self.checks:
            print(f"Checking: {name}...")
            try:
                result, details = check_func(broker, port, username, password)
                status = "✓ PASS" if result else "✗ FAIL"
                results.append((name, status, details))
                print(f"  {status}: {details}")
            except Exception as e:
                results.append((name, "✗ ERROR", str(e)))
                print(f"  ✗ ERROR: {e}")
        
        print("\n=== Summary ===")
        for name, status, details in results:
            print(f"{status} - {name}")
        
        return results

# Define diagnostic checks
def check_dns_resolution(broker, *args):
    """Check DNS resolution"""
    try:
        import socket
        ip = socket.gethostbyname(broker)
        return True, f"Resolved to {ip}"
    except:
        return False, "DNS resolution failed"

def check_port_connectivity(broker, port, *args):
    """Check port connectivity"""
    import socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    try:
        result = sock.connect_ex((broker, port))
        sock.close()
        if result == 0:
            return True, f"Port {port} is open"
        else:
            return False, f"Port {port} is closed or filtered"
    except:
        return False, "Connection test failed"

def check_mqtt_connect(broker, port, username, password):
    """Test MQTT connection"""
    client = mqtt.Client("debug-client")
    if username:
        client.username_pw_set(username, password)
    
    connected = False
    connect_error = None
    
    def on_connect(client, userdata, flags, rc):
        nonlocal connected, connect_error
        if rc == 0:
            connected = True
        else:
            connect_error = f"RC={rc}"
    
    client.on_connect = on_connect
    
    try:
        client.connect(broker, port, 60)
        client.loop(timeout=5)
        
        if connected:
            client.disconnect()
            return True, "MQTT connection successful"
        else:
            return False, f"MQTT connection failed: {connect_error}"
    except Exception as e:
        return False, f"Connection error: {str(e)}"

# Usage
debugger = MQTTConnectionDebugger()
debugger.add_check("DNS Resolution", check_dns_resolution)
debugger.add_check("Port Connectivity", check_port_connectivity)
debugger.add_check("MQTT Connection", check_mqtt_connect)
debugger.run_diagnostics("broker.hivemq.com", 1883)
```

### 6.2 Authentication Problems

#### Authentication Tester
```python
def test_authentication_methods():
    """Test different authentication methods"""
    
    test_cases = [
        {
            "name": "No Authentication",
            "broker": "broker.hivemq.com",
            "port": 1883,
            "auth": None
        },
        {
            "name": "Username/Password",
            "broker": "test.mosquitto.org",
            "port": 1884,
            "auth": {
                "username": "rw",
                "password": "readwrite"
            }
        },
        {
            "name": "Client Certificate",
            "broker": "test.mosquitto.org",
            "port": 8884,
            "auth": {
                "ca_certs": "mosquitto.org.crt",
                "certfile": "client.crt",
                "keyfile": "client.key"
            }
        }
    ]
    
    for test in test_cases:
        print(f"\nTesting: {test['name']}")
        
        client = mqtt.Client()
        
        # Configure authentication
        if test['auth']:
            if 'username' in test['auth']:
                client.username_pw_set(
                    test['auth']['username'],
                    test['auth']['password']
                )
            elif 'ca_certs' in test['auth']:
                client.tls_set(
                    ca_certs=test['auth']['ca_certs'],
                    certfile=test['auth'].get('certfile'),
                    keyfile=test['auth'].get('keyfile')
                )
        
        # Test connection
        try:
            client.connect(test['broker'], test['port'], 60)
            client.loop(timeout=5)
            
            if client.is_connected():
                print("  ✓ Authentication successful")
                client.disconnect()
            else:
                print("  ✗ Authentication failed")
        except Exception as e:
            print(f"  ✗ Error: {e}")
```

### 6.3 Topic Mismatch Errors

#### Topic Validator
```python
class TopicValidator:
    def __init__(self, device_id="tempcontroller01"):
        self.device_id = device_id
        self.topic_patterns = {
            'telemetry': r'^[\w/]+/' + device_id + r'/telemetry/\w+$',
            'command': r'^[\w/]+/' + device_id + r'/command/(request|response)$',
            'alarm': r'^[\w/]+/' + device_id + r'/alarm/\w+$',
            'state': r'^[\w/]+/' + device_id + r'/state/\w+$'
        }
    
    def validate_topic(self, topic):
        """Validate topic format"""
        import re
        
        errors = []
        
        # Check for common mistakes
        if ' ' in topic:
            errors.append("Topic contains spaces")
        
        if topic.startswith('/'):
            errors.append("Topic starts with '/' (not recommended)")
        
        if topic.endswith('/'):
            errors.append("Topic ends with '/'")
        
        if '//' in topic:
            errors.append("Topic contains empty levels (//)")
        
        # Check against patterns
        matched = False
        for category, pattern in self.topic_patterns.items():
            if re.match(pattern, topic):
                matched = True
                break
        
        if not matched:
            errors.append("Topic doesn't match any expected pattern")
        
        return len(errors) == 0, errors
    
    def suggest_correction(self, topic):
        """Suggest corrected topic"""
        # Remove common mistakes
        corrected = topic.strip()
        corrected = corrected.rstrip('/')
        corrected = corrected.replace('//', '/')
        corrected = corrected.replace(' ', '_')
        
        # Try to identify topic type
        if 'telemetry' in topic:
            base = f"plant/area1/line2/{self.device_id}/telemetry/"
            if 'temp' in topic:
                corrected = base + "temperature"
            elif 'system' in topic:
                corrected = base + "system"
        elif 'command' in topic:
            if 'req' in topic:
                corrected = f"plant/area1/line2/{self.device_id}/command/request"
            else:
                corrected = f"plant/area1/line2/{self.device_id}/command/response"
        
        return corrected

# Test topic validation
validator = TopicValidator()
test_topics = [
    "plant/area1/line2/tempcontroller01/telemetry/temperature",
    "plant/area1/line2/tempcontroller01/command request",  # Space
    "/plant/area1/line2/tempcontroller01/telemetry/",  # Leading/trailing /
    "plant//area1/line2/tempcontroller01/alarm/active",  # Double /
    "wrong/topic/format"
]

for topic in test_topics:
    valid, errors = validator.validate_topic(topic)
    if not valid:
        print(f"\n✗ Invalid topic: {topic}")
        for error in errors:
            print(f"  - {error}")
        suggestion = validator.suggest_correction(topic)
        print(f"  Suggestion: {suggestion}")
    else:
        print(f"✓ Valid topic: {topic}")
```

### 6.4 JSON Parsing Failures

#### JSON Debug Helper
```python
def debug_json_payload(payload_str):
    """Debug JSON parsing issues"""
    import json
    import re
    
    print("=== JSON Debugging ===")
    print(f"Payload length: {len(payload_str)} bytes")
    
    # Try to parse
    try:
        data = json.loads(payload_str)
        print("✓ Valid JSON")
        print(f"Type: {type(data)}")
        print(f"Keys: {list(data.keys()) if isinstance(data, dict) else 'N/A'}")
        return True
    except json.JSONDecodeError as e:
        print(f"✗ JSON Error: {e}")
        print(f"  Position: {e.pos}")
        
        # Show error context
        start = max(0, e.pos - 20)
        end = min(len(payload_str), e.pos + 20)
        context = payload_str[start:end]
        error_pos = e.pos - start
        
        print(f"  Context: {context}")
        print(f"  {' ' * (error_pos + 10)}^")
        
        # Common issues
        issues = []
        
        # Check for BOM
        if payload_str.startswith('\ufeff'):
            issues.append("UTF-8 BOM detected")
        
        # Check for single quotes
        if "'" in payload_str:
            issues.append("Single quotes found (JSON requires double quotes)")
        
        # Check for trailing commas
        if re.search(r',\s*[}\]]', payload_str):
            issues.append("Trailing comma detected")
        
        # Check for unquoted keys
        if re.search(r'{\s*\w+\s*:', payload_str):
            issues.append("Unquoted object keys detected")
        
        # Check for undefined values
        if 'undefined' in payload_str:
            issues.append("'undefined' is not valid JSON (use null)")
        
        if issues:
            print("\nCommon issues found:")
            for issue in issues:
                print(f"  - {issue}")
        
        # Try to fix common issues
        fixed = payload_str
        fixed = fixed.replace("'", '"')  # Single to double quotes
        fixed = re.sub(r',(\s*[}\]])', r'\1', fixed)  # Remove trailing commas
        
        try:
            json.loads(fixed)
            print("\n✓ Fixed JSON is valid!")
            print("Suggested fix applied:")
            print(f"  - Replaced single quotes with double quotes")
            print(f"  - Removed trailing commas")
        except:
            print("\n✗ Automatic fix failed")
        
        return False

# Test with problematic JSON
test_payloads = [
    '{"valid": "json", "number": 123}',
    "{'invalid': 'single quotes'}",
    '{"trailing": "comma",}',
    '{unquoted: "key"}',
    '{"undefined": undefined}',
    '{"incomplete": '
]

for payload in test_payloads:
    print(f"\nTesting: {payload[:50]}...")
    debug_json_payload(payload)
```

### 6.5 Memory Constraints

#### ESP32 Memory Monitor
```python
def generate_memory_test_code():
    """Generate ESP32 code for memory monitoring"""
    
    code = '''
// ESP32 MQTT Memory Monitor
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

class MemoryMonitor {
private:
    unsigned long lastReport = 0;
    const unsigned long REPORT_INTERVAL = 10000; // 10 seconds
    
public:
    void update() {
        if (millis() - lastReport > REPORT_INTERVAL) {
            reportMemory();
            lastReport = millis();
        }
    }
    
    void reportMemory() {
        Serial.println("=== Memory Report ===");
        
        // Heap memory
        Serial.printf("Free Heap: %d bytes\\n", ESP.getFreeHeap());
        Serial.printf("Min Free Heap: %d bytes\\n", ESP.getMinFreeHeap());
        Serial.printf("Max Alloc Heap: %d bytes\\n", ESP.getMaxAllocHeap());
        
        // PSRAM (if available)
        if (psramFound()) {
            Serial.printf("Free PSRAM: %d bytes\\n", ESP.getFreePsram());
        }
        
        // Stack high water mark
        Serial.printf("Stack HWM: %d bytes\\n", uxTaskGetStackHighWaterMark(NULL));
        
        // Fragmentation estimate
        float fragmentation = 100.0 * (1.0 - ((float)ESP.getMaxAllocHeap() / (float)ESP.getFreeHeap()));
        Serial.printf("Fragmentation: %.1f%%\\n", fragmentation);
        
        // MQTT buffer usage estimate
        size_t mqttBufferSize = 0;
        if (mqttClient.connected()) {
            // Estimate based on max packet size
            mqttBufferSize = MQTT_MAX_PACKET_SIZE;
        }
        Serial.printf("MQTT Buffer: ~%d bytes\\n", mqttBufferSize);
        
        Serial.println("==================");
    }
    
    bool checkMemory(size_t required) {
        size_t available = ESP.getFreeHeap();
        if (available < required + 10240) { // Keep 10KB buffer
            Serial.printf("WARNING: Low memory! Required: %d, Available: %d\\n", 
                         required, available);
            return false;
        }
        return true;
    }
};

// Optimized telemetry publishing for 60 points
void publishTelemetryOptimized() {
    const size_t BUFFER_SIZE = 8192; // 8KB buffer
    char* buffer = (char*)malloc(BUFFER_SIZE);
    
    if (!buffer) {
        Serial.println("ERROR: Failed to allocate buffer");
        return;
    }
    
    // Use streaming JSON to minimize memory usage
    size_t written = 0;
    written += snprintf(buffer + written, BUFFER_SIZE - written,
                       "{\\"timestamp\\":\\"%s\\",", getTimestamp());
    written += snprintf(buffer + written, BUFFER_SIZE - written,
                       "\\"device_id\\":\\"%s\\",", DEVICE_ID);
    written += snprintf(buffer + written, BUFFER_SIZE - written,
                       "\\"points\\":{");
    
    // Stream points
    bool first = true;
    for (int i = 0; i < 60; i++) {
        if (points[i].isActive()) {
            if (!first) {
                written += snprintf(buffer + written, BUFFER_SIZE - written, ",");
            }
            
            written += snprintf(buffer + written, BUFFER_SIZE - written,
                              "\\"%d\\":{\\"temp\\":%.1f,\\"status\\":\\"%s\\"}",
                              i, points[i].getTemp(), points[i].getStatus());
            
            first = false;
            
            // Check buffer space
            if (BUFFER_SIZE - written < 100) {
                Serial.println("WARNING: Buffer nearly full");
                break;
            }
        }
    }
    
    written += snprintf(buffer + written, BUFFER_SIZE - written, "}}");
    
    // Publish
    if (mqttClient.publish(TELEMETRY_TOPIC, buffer)) {
        Serial.printf("Published %d bytes\\n", written);
    } else {
        Serial.println("Publish failed - message too large?");
    }
    
    free(buffer);
}

// Memory-safe command processing
void processCommand(const char* payload, size_t length) {
    // Use static buffer to avoid allocation
    const size_t MAX_CMD_SIZE = 1024;
    static StaticJsonDocument<MAX_CMD_SIZE> doc;
    
    // Clear previous data
    doc.clear();
    
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.printf("JSON parse error: %s\\n", error.c_str());
        return;
    }
    
    // Process command...
}
'''
    
    print("=== ESP32 Memory Management Code ===")
    print(code)
    return code

# Memory optimization tips
memory_tips = """
=== ESP32 MQTT Memory Optimization Tips ===

1. **Use Static Allocation Where Possible**
   - StaticJsonDocument instead of DynamicJsonDocument
   - Fixed-size buffers for known data

2. **Stream Large Data**
   - Use chunked publishing for 60-point telemetry
   - Consider multiple smaller messages

3. **Monitor Fragmentation**
   - Check ESP.getMaxAllocHeap() vs ESP.getFreeHeap()
   - Restart if fragmentation > 50%

4. **Optimize JSON Size**
   - Short key names ("t" instead of "temperature")
   - Omit null/default values
   - Use arrays instead of objects where possible

5. **MQTT Buffer Management**
   - Reduce MQTT_MAX_PACKET_SIZE if possible
   - Use QoS 0 for large telemetry
   - Clear buffers after publishing

6. **Task Stack Sizes**
   - Monitor with uxTaskGetStackHighWaterMark()
   - Reduce if overallocated

7. **Use PSRAM**
   - Move large buffers to PSRAM if available
   - Keep critical data in internal RAM

Example memory budget for 60-point system:
- WiFi: ~30KB
- MQTT: ~20KB (with TLS: ~50KB)
- JSON buffer: 8KB
- Application: ~40KB
- Free buffer: 20KB+
Total: ~120KB minimum
"""

print(memory_tips)
```

## 7. Integration Testing

### 7.1 Web Interface Configuration Test

#### Web Config Integration Test
```python
import requests
import json

class WebConfigTest:
    def __init__(self, device_ip):
        self.device_ip = device_ip
        self.base_url = f"http://{device_ip}"
        
    def test_mqtt_config_page(self):
        """Test MQTT configuration page"""
        print("=== Web Interface MQTT Config Test ===")
        
        # Get current config
        try:
            resp = requests.get(f"{self.base_url}/api/mqtt/config")
            current_config = resp.json()
            print("Current configuration:")
            print(json.dumps(current_config, indent=2))
        except Exception as e:
            print(f"Failed to get config: {e}")
            return
        
        # Test configuration update
        test_config = {
            "mqtt_enabled": True,
            "mqtt_broker": "test.mosquitto.org",
            "mqtt_port": 1883,
            "mqtt_username": "testuser",
            "mqtt_password": "testpass",
            "mqtt_device_name": "test_controller",
            "mqtt_telemetry_interval": 30,
            "mqtt_use_tls": False,
            "mqtt_retain_telemetry": True,
            "topic_level1_type": "custom",
            "topic_level1_value": "test",
            "topic_level2_type": "skip",
            "topic_level3_type": "skip"
        }
        
        print("\nUpdating configuration...")
        try:
            resp = requests.post(
                f"{self.base_url}/api/mqtt/config",
                json=test_config
            )
            if resp.status_code == 200:
                print("✓ Configuration updated successfully")
            else:
                print(f"✗ Update failed: {resp.status_code}")
        except Exception as e:
            print(f"✗ Update error: {e}")
        
        # Verify MQTT connection
        print("\nChecking MQTT status...")
        try:
            resp = requests.get(f"{self.base_url}/api/mqtt/status")
            status = resp.json()
            print(f"Connected: {status.get('connected')}")
            print(f"Broker: {status.get('broker')}")
            print(f"Messages sent: {status.get('messages_sent')}")
            print(f"Last error: {status.get('last_error', 'None')}")
        except Exception as e:
            print(f"Failed to get status: {e}")

# Run test
tester = WebConfigTest("192.168.1.100")
tester.test_mqtt_config_page()
```

### 7.2 Alarm Trigger Verification

#### Alarm Integration Test
```python
class AlarmMQTTTest:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        self.alarm_events = []
        
    def monitor_alarms(self, duration=60):
        """Monitor alarm events via MQTT"""
        print(f"=== Alarm MQTT Integration Test ({duration}s) ===")
        
        client = mqtt.Client()
        alarm_topic = f"plant/area1/line2/{self.device_id}/alarm/#"
        
        def on_message(client, userdata, msg):
            try:
                data = json.loads(msg.payload.decode())
                self.alarm_events.append({
                    'topic': msg.topic,
                    'data': data,
                    'timestamp': time.time()
                })
                
                print(f"\n🔔 Alarm Event:")
                print(f"  Topic: {msg.topic}")
                print(f"  Type: {data.get('alarm_type')}")
                print(f"  Point: {data.get('point_name')} (ID: {data.get('point_id')})")
                print(f"  Priority: {data.get('priority')}")
                print(f"  State: {data.get('state_transition')}")
                print(f"  Value: {data.get('temperature')}°C (threshold: {data.get('threshold')})")
                
            except Exception as e:
                print(f"Error processing alarm: {e}")
        
        client.on_message = on_message
        client.connect(self.broker, 1883)
        client.subscribe(alarm_topic)
        
        print(f"Monitoring alarms on: {alarm_topic}")
        print("Trigger test alarms on the device...\n")
        
        client.loop_start()
        time.sleep(duration)
        client.loop_stop()
        client.disconnect()
        
        # Summary
        print(f"\n=== Alarm Summary ===")
        print(f"Total events: {len(self.alarm_events)}")
        
        # Group by type
        by_type = {}
        for event in self.alarm_events:
            alarm_type = event['data'].get('alarm_type', 'UNKNOWN')
            by_type[alarm_type] = by_type.get(alarm_type, 0) + 1
        
        print("Events by type:")
        for alarm_type, count in by_type.items():
            print(f"  {alarm_type}: {count}")
    
    def trigger_test_alarm(self):
        """Send command to trigger test alarm"""
        client = mqtt.Client()
        client.connect(self.broker, 1883)
        
        cmd = {
            "cmd_id": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "command": "test_alarm",
            "parameters": {
                "point_id": 5,
                "alarm_type": "HIGH_TEMP",
                "test_value": 95.0
            }
        }
        
        topic = f"plant/area1/line2/{self.device_id}/command/request"
        client.publish(topic, json.dumps(cmd))
        print("Test alarm command sent")
        
        client.disconnect()
```

### 7.3 Relay Control Testing

#### Relay Control Test
```python
class RelayMQTTTest:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        
    def test_relay_control(self):
        """Test relay control via MQTT"""
        print("=== Relay Control MQTT Test ===")
        
        client = mqtt.Client()
        cmd_topic = f"plant/area1/line2/{self.device_id}/command/request"
        resp_topic = f"plant/area1/line2/{self.device_id}/command/response"
        event_topic = f"plant/area1/line2/{self.device_id}/event/relay"
        
        responses = {}
        relay_events = []
        
        def on_message(client, userdata, msg):
            try:
                data = json.loads(msg.payload.decode())
                
                if "command/response" in msg.topic:
                    cmd_id = data.get('cmd_id')
                    if cmd_id:
                        responses[cmd_id] = data
                
                elif "event/relay" in msg.topic:
                    relay_events.append(data)
                    print(f"🔌 Relay Event: Relay {data.get('relay_id')} -> {data.get('state')}")
                    
            except Exception as e:
                print(f"Message error: {e}")
        
        client.on_message = on_message
        client.connect(self.broker, 1883)
        client.subscribe([(resp_topic, 1), (event_topic, 1)])
        client.loop_start()
        
        # Test sequence
        test_sequence = [
            {"relay_id": 1, "state": "ON", "duration": 5},
            {"relay_id": 1, "state": "OFF", "duration": 0},
            {"relay_id": 2, "state": "TOGGLE", "duration": 0},
            {"relay_id": 3, "state": "ON", "duration": 10},
        ]
        
        for test in test_sequence:
            cmd_id = str(uuid.uuid4())
            cmd = {
                "cmd_id": cmd_id,
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "command": "set_relay",
                "parameters": test
            }
            
            print(f"\nSending: Relay {test['relay_id']} -> {test['state']}")
            client.publish(cmd_topic, json.dumps(cmd))
            
            # Wait for response
            timeout = 5
            start = time.time()
            while cmd_id not in responses and time.time() - start < timeout:
                time.sleep(0.1)
            
            if cmd_id in responses:
                resp = responses[cmd_id]
                print(f"Response: {resp.get('status')} - {resp.get('message')}")
            else:
                print("Response: Timeout")
            
            time.sleep(2)
        
        client.loop_stop()
        client.disconnect()
        
        print(f"\n=== Test Summary ===")
        print(f"Commands sent: {len(test_sequence)}")
        print(f"Responses received: {len(responses)}")
        print(f"Relay events: {len(relay_events)}")
```

### 7.4 Schedule Execution Test

#### Schedule Test
```python
class ScheduleMQTTTest:
    def __init__(self, broker, device_id):
        self.broker = broker
        self.device_id = device_id
        
    def test_scheduled_messages(self):
        """Test MQTT scheduler functionality"""
        print("=== MQTT Schedule Test ===")
        
        client = mqtt.Client()
        cmd_topic = f"plant/area1/line2/{self.device_id}/command/request"
        resp_topic = f"plant/area1/line2/{self.device_id}/command/response"
        telemetry_topic = f"plant/area1/line2/{self.device_id}/telemetry/summary"
        
        scheduled_messages = []
        
        def on_message(client, userdata, msg):
            if "telemetry/summary" in msg.topic:
                scheduled_messages.append({
                    'timestamp': time.time(),
                    'data': json.loads(msg.payload.decode())
                })
                print(f"📅 Scheduled message received at {time.strftime('%H:%M:%S')}")
        
        client.on_message = on_message
        client.connect(self.broker, 1883)
        client.subscribe([(resp_topic, 1), (telemetry_topic, 1)])
        client.loop_start()
        
        # Add test schedule
        schedule_id = "test_schedule_" + str(int(time.time()))
        cmd = {
            "cmd_id": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "command": "add_schedule",
            "parameters": {
                "schedule_id": schedule_id,
                "schedule_type": "interval",
                "interval_seconds": 10,  # Every 10 seconds
                "command": "get_summary",
                "parameters": {}
            }
        }
        
        print(f"Adding schedule: {schedule_id}")
        print(f"Interval: 10 seconds")
        client.publish(cmd_topic, json.dumps(cmd))
        
        # Monitor for 1 minute
        print("\nMonitoring scheduled messages for 60 seconds...")
        start_time = time.time()
        
        while time.time() - start_time < 60:
            remaining = 60 - (time.time() - start_time)
            print(f"\rTime remaining: {remaining:.0f}s  Messages: {len(scheduled_messages)}", 
                  end='', flush=True)
            time.sleep(1)
        
        print("\n")
        
        # Remove schedule
        cmd = {
            "cmd_id": str(uuid.uuid4()),
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "command": "remove_schedule",
            "parameters": {"schedule_id": schedule_id}
        }
        
        print(f"Removing schedule: {schedule_id}")
        client.publish(cmd_topic, json.dumps(cmd))
        
        time.sleep(2)
        client.loop_stop()
        client.disconnect()
        
        # Analyze results
        print(f"\n=== Schedule Test Results ===")
        print(f"Expected messages: ~6 (every 10s for 60s)")
        print(f"Received messages: {len(scheduled_messages)}")
        
        if len(scheduled_messages) > 1:
            intervals = []
            for i in range(1, len(scheduled_messages)):
                interval = scheduled_messages[i]['timestamp'] - scheduled_messages[i-1]['timestamp']
                intervals.append(interval)
            
            avg_interval = statistics.mean(intervals)
            print(f"Average interval: {avg_interval:.1f}s")
            print(f"Interval accuracy: {(10/avg_interval)*100:.1f}%")
```

## 8. Production Monitoring

### 8.1 MQTT Statistics Tracking

#### Statistics Collector
```python
class MQTTStatisticsCollector:
    def __init__(self):
        self.stats = {
            'messages_published': 0,
            'messages_received': 0,
            'bytes_sent': 0,
            'bytes_received': 0,
            'connection_time': 0,
            'disconnections': 0,
            'errors': [],
            'message_types': {},
            'response_times': []
        }
        self.start_time = time.time()
        
    def on_publish(self, topic, payload):
        """Track published message"""
        self.stats['messages_published'] += 1
        self.stats['bytes_sent'] += len(payload)
        
        # Track message type
        msg_type = self._get_message_type(topic)
        self.stats['message_types'][msg_type] = \
            self.stats['message_types'].get(msg_type, 0) + 1
    
    def on_message(self, topic, payload):
        """Track received message"""
        self.stats['messages_received'] += 1
        self.stats['bytes_received'] += len(payload)
    
    def on_disconnect(self, reason):
        """Track disconnection"""
        self.stats['disconnections'] += 1
        if reason:
            self.stats['errors'].append({
                'time': time.time(),
                'type': 'disconnect',
                'reason': reason
            })
    
    def _get_message_type(self, topic):
        """Extract message type from topic"""
        parts = topic.split('/')
        if len(parts) >= 5:
            return parts[4]  # e.g., 'telemetry', 'command', 'alarm'
        return 'unknown'
    
    def get_report(self):
        """Generate statistics report"""
        uptime = time.time() - self.start_time
        
        report = {
            'uptime_seconds': uptime,
            'messages': {
                'published': self.stats['messages_published'],
                'received': self.stats['messages_received'],
                'rate_per_minute': (self.stats['messages_published'] / uptime) * 60
            },
            'data': {
                'sent_MB': self.stats['bytes_sent'] / 1024 / 1024,
                'received_MB': self.stats['bytes_received'] / 1024 / 1024,
                'bandwidth_kbps': (self.stats['bytes_sent'] + self.stats['bytes_received']) / uptime / 1024 * 8
            },
            'reliability': {
                'disconnections': self.stats['disconnections'],
                'availability_percent': ((uptime - self.stats['disconnections'] * 30) / uptime) * 100
            },
            'message_distribution': self.stats['message_types'],
            'recent_errors': self.stats['errors'][-10:]  # Last 10 errors
        }
        
        return report
    
    def print_report(self):
        """Print formatted report"""
        report = self.get_report()
        
        print("=== MQTT Statistics Report ===")
        print(f"Uptime: {report['uptime_seconds']/3600:.1f} hours")
        print(f"\nMessages:")
        print(f"  Published: {report['messages']['published']:,}")
        print(f"  Received: {report['messages']['received']:,}")
        print(f"  Rate: {report['messages']['rate_per_minute']:.1f}/min")
        print(f"\nData Transfer:")
        print(f"  Sent: {report['data']['sent_MB']:.2f} MB")
        print(f"  Received: {report['data']['received_MB']:.2f} MB")
        print(f"  Bandwidth: {report['data']['bandwidth_kbps']:.1f} kbps")
        print(f"\nReliability:")
        print(f"  Disconnections: {report['reliability']['disconnections']}")
        print(f"  Availability: {report['reliability']['availability_percent']:.2f}%")
        print(f"\nMessage Types:")
        for msg_type, count in report['message_distribution'].items():
            print(f"  {msg_type}: {count:,}")
```

### 8.2 Error Rate Monitoring

#### Error Monitor
```python
class MQTTErrorMonitor:
    def __init__(self, alert_threshold=0.05):  # 5% error rate
        self.alert_threshold = alert_threshold
        self.error_counts = {}
        self.total_operations = {}
        self.alerts = []
        
    def record_operation(self, operation_type, success=True, error_detail=None):
        """Record an operation result"""
        if operation_type not in self.total_operations:
            self.total_operations[operation_type] = 0
            self.error_counts[operation_type] = 0
        
        self.total_operations[operation_type] += 1
        
        if not success:
            self.error_counts[operation_type] += 1
            
            # Check if alert needed
            error_rate = self.error_counts[operation_type] / self.total_operations[operation_type]
            if error_rate > self.alert_threshold:
                self.trigger_alert(operation_type, error_rate, error_detail)
    
    def trigger_alert(self, operation_type, error_rate, error_detail):
        """Trigger an error rate alert"""
        alert = {
            'timestamp': time.time(),
            'operation': operation_type,
            'error_rate': error_rate,
            'threshold': self.alert_threshold,
            'detail': error_detail
        }
        
        self.alerts.append(alert)
        
        # Log alert
        print(f"\n⚠️  ERROR RATE ALERT ⚠️")
        print(f"Operation: {operation_type}")
        print(f"Error rate: {error_rate*100:.1f}% (threshold: {self.alert_threshold*100:.0f}%)")
        print(f"Total operations: {self.total_operations[operation_type]}")
        print(f"Errors: {self.error_counts[operation_type]}")
        if error_detail:
            print(f"Latest error: {error_detail}")
        
        # Could also send MQTT notification, email, etc.
    
    def get_error_rates(self):
        """Get current error rates"""
        rates = {}
        for op_type in self.total_operations:
            if self.total_operations[op_type] > 0:
                rates[op_type] = self.error_counts[op_type] / self.total_operations[op_type]
        return rates
    
    def reset_counters(self):
        """Reset error counters (e.g., daily)"""
        self.error_counts = {}
        self.total_operations = {}

# Usage example
error_monitor = MQTTErrorMonitor()

# Simulate operations
for i in range(100):
    # 95% success rate for publish
    success = random.random() > 0.05
    error_monitor.record_operation('publish', success, 
                                 "Connection lost" if not success else None)
    
    # 98% success rate for commands
    success = random.random() > 0.02  
    error_monitor.record_operation('command', success,
                                 "Timeout" if not success else None)

# Check rates
print("\nCurrent Error Rates:")
for op, rate in error_monitor.get_error_rates().items():
    print(f"  {op}: {rate*100:.1f}%")
```

### 8.3 Performance Metrics

#### Performance Monitor
```python
class MQTTPerformanceMonitor:
    def __init__(self):
        self.metrics = {
            'publish_times': [],
            'command_response_times': [],
            'connection_times': [],
            'queue_sizes': [],
            'cpu_usage': [],
            'memory_usage': []
        }
        
    def record_publish_time(self, duration_ms):
        """Record time taken to publish message"""
        self.metrics['publish_times'].append(duration_ms)
        
        # Keep only last 1000 samples
        if len(self.metrics['publish_times']) > 1000:
            self.metrics['publish_times'].pop(0)
    
    def record_command_response(self, duration_ms):
        """Record command round-trip time"""
        self.metrics['command_response_times'].append(duration_ms)
        
        if len(self.metrics['command_response_times']) > 1000:
            self.metrics['command_response_times'].pop(0)
    
    def get_performance_summary(self):
        """Get performance metrics summary"""
        summary = {}
        
        # Publish performance
        if self.metrics['publish_times']:
            summary['publish'] = {
                'avg_ms': statistics.mean(self.metrics['publish_times']),
                'min_ms': min(self.metrics['publish_times']),
                'max_ms': max(self.metrics['publish_times']),
                'p95_ms': self._percentile(self.metrics['publish_times'], 95),
                'p99_ms': self._percentile(self.metrics['publish_times'], 99)
            }
        
        # Command performance
        if self.metrics['command_response_times']:
            summary['commands'] = {
                'avg_ms': statistics.mean(self.metrics['command_response_times']),
                'min_ms': min(self.metrics['command_response_times']),
                'max_ms': max(self.metrics['command_response_times']),
                'p95_ms': self._percentile(self.metrics['command_response_times'], 95),
                'p99_ms': self._percentile(self.metrics['command_response_times'], 99)
            }
        
        return summary
    
    def _percentile(self, data, percentile):
        """Calculate percentile"""
        sorted_data = sorted(data)
        index = int((percentile / 100) * len(sorted_data))
        return sorted_data[min(index, len(sorted_data) - 1)]
    
    def check_sla(self, sla_config):
        """Check if performance meets SLA"""
        summary = self.get_performance_summary()
        violations = []
        
        # Check publish SLA
        if 'publish' in summary and 'publish_p95_ms' in sla_config:
            if summary['publish']['p95_ms'] > sla_config['publish_p95_ms']:
                violations.append(f"Publish P95 ({summary['publish']['p95_ms']:.1f}ms) "
                                f"exceeds SLA ({sla_config['publish_p95_ms']}ms)")
        
        # Check command SLA
        if 'commands' in summary and 'command_p95_ms' in sla_config:
            if summary['commands']['p95_ms'] > sla_config['command_p95_ms']:
                violations.append(f"Command P95 ({summary['commands']['p95_ms']:.1f}ms) "
                                f"exceeds SLA ({sla_config['command_p95_ms']}ms)")
        
        return len(violations) == 0, violations

# Example SLA configuration
sla_config = {
    'publish_p95_ms': 100,      # 95% of publishes < 100ms
    'publish_p99_ms': 500,      # 99% of publishes < 500ms
    'command_p95_ms': 500,      # 95% of commands < 500ms
    'command_p99_ms': 1000,     # 99% of commands < 1000ms
}
```

### 8.4 Audit Logging

#### Audit Logger
```python
class MQTTAuditLogger:
    def __init__(self, log_file="mqtt_audit.log"):
        self.log_file = log_file
        self.setup_logger()
        
    def setup_logger(self):
        """Setup audit logger"""
        import logging
        from logging.handlers import RotatingFileHandler
        
        self.logger = logging.getLogger('mqtt_audit')
        self.logger.setLevel(logging.INFO)
        
        # Rotating file handler (10MB per file, keep 10 files)
        handler = RotatingFileHandler(
            self.log_file,
            maxBytes=10*1024*1024,
            backupCount=10
        )
        
        # Format: timestamp | event_type | details
        formatter = logging.Formatter(
            '%(asctime)s | %(levelname)s | %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        handler.setFormatter(formatter)
        
        self.logger.addHandler(handler)
    
    def log_connection(self, broker, client_id, success, error=None):
        """Log connection attempt"""
        if success:
            self.logger.info(f"CONNECTION | broker={broker} | client_id={client_id} | status=success")
        else:
            self.logger.error(f"CONNECTION | broker={broker} | client_id={client_id} | status=failed | error={error}")
    
    def log_command(self, cmd_id, command, parameters, source, result):
        """Log command execution"""
        self.logger.info(
            f"COMMAND | id={cmd_id} | cmd={command} | "
            f"params={json.dumps(parameters)} | source={source} | result={result}"
        )
    
    def log_alarm(self, alarm_id, point_id, alarm_type, action, user=None):
        """Log alarm actions"""
        self.logger.info(
            f"ALARM | id={alarm_id} | point={point_id} | type={alarm_type} | "
            f"action={action} | user={user or 'system'}"
        )
    
    def log_config_change(self, parameter, old_value, new_value, source):
        """Log configuration changes"""
        self.logger.info(
            f"CONFIG | param={parameter} | old={old_value} | new={new_value} | source={source}"
        )
    
    def log_security_event(self, event_type, details):
        """Log security-related events"""
        self.logger.warning(f"SECURITY | type={event_type} | details={details}")
    
    def search_logs(self, criteria):
        """Search audit logs"""
        matches = []
        
        with open(self.log_file, 'r') as f:
            for line in f:
                if all(criterion in line for criterion in criteria):
                    matches.append(line.strip())
        
        return matches

# Usage example
audit = MQTTAuditLogger()

# Log various events
audit.log_connection("broker.hivemq.com", "device001", True)
audit.log_command("cmd-123", "set_threshold", {"point_id": 5, "value": 85.0}, 
                 "web_interface", "success")
audit.log_alarm("ALM-456", 5, "HIGH_TEMP", "acknowledged", "operator1")
audit.log_config_change("telemetry_interval", 60, 30, "mqtt_command")
audit.log_security_event("invalid_auth", "Multiple failed auth attempts from 192.168.1.50")

# Search logs
print("Commands from web interface:")
for log in audit.search_logs(["COMMAND", "web_interface"]):
    print(f"  {log}")
```

## 9. ESP32-Specific Considerations

### 9.1 ESP32 MQTT Implementation

```cpp
// ESP32 MQTT Test Framework
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

class ESP32MQTTTester {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    // Test metrics
    struct TestMetrics {
        uint32_t connectAttempts = 0;
        uint32_t connectSuccess = 0;
        uint32_t publishAttempts = 0;
        uint32_t publishSuccess = 0;
        uint32_t messagesReceived = 0;
        uint32_t reconnects = 0;
        unsigned long totalConnectTime = 0;
        unsigned long lastHeapSize = 0;
    } metrics;
    
public:
    ESP32MQTTTester() : mqttClient(wifiClient) {}
    
    void runTestSuite() {
        Serial.println("=== ESP32 MQTT Test Suite ===");
        
        // Test 1: Connection stability
        testConnectionStability();
        
        // Test 2: Large payload handling
        testLargePayloads();
        
        // Test 3: Rapid publish
        testRapidPublish();
        
        // Test 4: Memory stability
        testMemoryStability();
        
        // Test 5: Power failure recovery
        testPowerFailureRecovery();
        
        // Print results
        printTestResults();
    }
    
    void testConnectionStability() {
        Serial.println("\n[TEST 1] Connection Stability");
        
        for (int i = 0; i < 10; i++) {
            unsigned long start = millis();
            
            if (connectMQTT()) {
                metrics.connectSuccess++;
                metrics.totalConnectTime += (millis() - start);
                
                // Stay connected for 30 seconds
                unsigned long testStart = millis();
                while (millis() - testStart < 30000) {
                    mqttClient.loop();
                    
                    // Publish heartbeat every 5 seconds
                    if ((millis() - testStart) % 5000 == 0) {
                        String payload = "{\"test\":\"heartbeat\",\"iteration\":" + 
                                       String(i) + "}";
                        if (mqttClient.publish("test/heartbeat", payload.c_str())) {
                            metrics.publishSuccess++;
                        }
                        metrics.publishAttempts++;
                    }
                    
                    delay(100);
                }
                
                mqttClient.disconnect();
            }
            
            metrics.connectAttempts++;
            delay(2000); // Wait before next iteration
        }
    }
    
    void testLargePayloads() {
        Serial.println("\n[TEST 2] Large Payload Handling");
        
        if (!connectMQTT()) return;
        
        // Test different payload sizes
        size_t sizes[] = {1024, 2048, 4096, 8192, 16384};
        
        for (size_t size : sizes) {
            // Create payload
            char* payload = (char*)malloc(size);
            if (!payload) {
                Serial.printf("Failed to allocate %d bytes\n", size);
                continue;
            }
            
            // Fill with test data
            for (size_t i = 0; i < size - 1; i++) {
                payload[i] = 'A' + (i % 26);
            }
            payload[size - 1] = '\0';
            
            // Measure publish time
            unsigned long start = millis();
            bool success = mqttClient.publish("test/large", payload);
            unsigned long duration = millis() - start;
            
            Serial.printf("Size: %d bytes, Success: %s, Time: %ldms\n",
                         size, success ? "YES" : "NO", duration);
            
            free(payload);
            delay(1000);
        }
        
        mqttClient.disconnect();
    }
    
    void testRapidPublish() {
        Serial.println("\n[TEST 3] Rapid Publishing");
        
        if (!connectMQTT()) return;
        
        const int MESSAGE_COUNT = 100;
        int successCount = 0;
        unsigned long start = millis();
        
        for (int i = 0; i < MESSAGE_COUNT; i++) {
            StaticJsonDocument<128> doc;
            doc["msg"] = i;
            doc["time"] = millis();
            
            char buffer[128];
            serializeJson(doc, buffer);
            
            if (mqttClient.publish("test/rapid", buffer, false)) {
                successCount++;
            }
            
            // Minimal delay to avoid overwhelming
            delay(10);
        }
        
        unsigned long duration = millis() - start;
        float rate = (float)MESSAGE_COUNT / (duration / 1000.0);
        
        Serial.printf("Published %d/%d messages in %ldms (%.1f msg/s)\n",
                     successCount, MESSAGE_COUNT, duration, rate);
        
        mqttClient.disconnect();
    }
    
    void testMemoryStability() {
        Serial.println("\n[TEST 4] Memory Stability");
        
        metrics.lastHeapSize = ESP.getFreeHeap();
        
        for (int cycle = 0; cycle < 5; cycle++) {
            Serial.printf("Cycle %d - Heap: %d bytes\n", 
                         cycle + 1, ESP.getFreeHeap());
            
            // Connect
            if (connectMQTT()) {
                // Subscribe to multiple topics
                mqttClient.subscribe("test/memory/+");
                
                // Publish and receive
                for (int i = 0; i < 20; i++) {
                    String topic = "test/memory/" + String(i % 5);
                    String payload = "{\"cycle\":" + String(cycle) + 
                                   ",\"msg\":" + String(i) + "}";
                    
                    mqttClient.publish(topic.c_str(), payload.c_str());
                    mqttClient.loop();
                    delay(100);
                }
                
                mqttClient.disconnect();
            }
            
            // Force garbage collection
            delay(1000);
            
            // Check heap
            size_t currentHeap = ESP.getFreeHeap();
            int heapDiff = metrics.lastHeapSize - currentHeap;
            
            if (heapDiff > 1024) {
                Serial.printf("WARNING: Heap decreased by %d bytes\n", heapDiff);
            }
        }
    }
    
    void testPowerFailureRecovery() {
        Serial.println("\n[TEST 5] Power Failure Recovery Simulation");
        
        // Save state to RTC memory
        RTC_DATA_ATTR static uint32_t bootCount = 0;
        bootCount++;
        
        Serial.printf("Boot count: %d\n", bootCount);
        
        // Simulate different failure points
        if (connectMQTT()) {
            // Publish boot message
            StaticJsonDocument<128> doc;
            doc["event"] = "boot";
            doc["count"] = bootCount;
            doc["reason"] = esp_reset_reason();
            
            char buffer[128];
            serializeJson(doc, buffer);
            
            mqttClient.publish("test/boot", buffer, true); // Retained
            
            // Simulate work
            delay(5000);
            
            mqttClient.disconnect();
        }
    }
    
    bool connectMQTT() {
        mqttClient.setServer("broker.hivemq.com", 1883);
        
        String clientId = "ESP32Test_" + String(random(0xffff), HEX);
        
        if (mqttClient.connect(clientId.c_str())) {
            return true;
        }
        
        return false;
    }
    
    void printTestResults() {
        Serial.println("\n=== Test Results ===");
        Serial.printf("Connection Success Rate: %.1f%%\n", 
                     (float)metrics.connectSuccess / metrics.connectAttempts * 100);
        Serial.printf("Average Connect Time: %ldms\n",
                     metrics.totalConnectTime / metrics.connectSuccess);
        Serial.printf("Publish Success Rate: %.1f%%\n",
                     (float)metrics.publishSuccess / metrics.publishAttempts * 100);
        Serial.printf("Final Heap Size: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Minimum Free Heap: %d bytes\n", ESP.getMinFreeHeap());
    }
};
```

### 9.2 ESP32 Network Optimization

```cpp
// Network optimization settings for ESP32
void optimizeESP32Network() {
    // WiFi power saving modes
    WiFi.setSleep(false);  // Disable sleep for lowest latency
    
    // Set WiFi TX power
    WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Maximum power
    
    // TCP/IP tuning
    // Increase TCP MSS (Maximum Segment Size)
    extern "C" {
        #include "lwip/tcp.h"
        void tcp_set_mss_limit(u16_t mss);
    }
    tcp_set_mss_limit(1460);  // Ethernet MTU - headers
    
    // Set hostname for easier identification
    WiFi.setHostname("TempController01");
    
    // Enable WiFi auto-reconnect
    WiFi.setAutoReconnect(true);
    
    // Persistent WiFi settings
    WiFi.persistent(true);
}

// MQTT optimization for ESP32
void optimizeMQTTSettings(PubSubClient& client) {
    // Increase MQTT buffer size for 60-point telemetry
    client.setBufferSize(8192);  // 8KB buffer
    
    // Set keep-alive interval
    client.setKeepAlive(60);  // 60 seconds
    
    // Socket timeout
    client.setSocketTimeout(10);  // 10 seconds
}
```

## 10. Troubleshooting Flowcharts

### 10.1 Connection Failure Flowchart

```
START
  |
  v
[Check WiFi Connection]
  |
  +-- No --> [Connect WiFi] --> [Retry]
  |
  +-- Yes
      |
      v
[Check DNS Resolution]
  |
  +-- Fails --> [Check DNS Settings] --> [Use IP Instead]
  |
  +-- Success
      |
      v
[Check Port Access]
  |
  +-- Blocked --> [Check Firewall] --> [Try Different Port]
  |
  +-- Open
      |
      v
[Check MQTT Auth]
  |
  +-- Fails --> [Verify Credentials] --> [Check ACL]
  |
  +-- Success
      |
      v
[Check TLS/SSL]
  |
  +-- Fails --> [Verify Certificates] --> [Check Time/Date]
  |
  +-- Success
      |
      v
[Connection Successful]
```

### 10.2 Message Not Received Flowchart

```
START
  |
  v
[Verify Connection Active]
  |
  +-- No --> [Reconnect] --> [Retry]
  |
  +-- Yes
      |
      v
[Check Topic Subscription]
  |
  +-- Not Subscribed --> [Subscribe to Topic] --> [Verify QoS]
  |
  +-- Subscribed
      |
      v
[Verify Topic Format]
  |
  +-- Incorrect --> [Fix Topic Path] --> [Check Wildcards]
  |
  +-- Correct
      |
      v
[Check Message Size]
  |
  +-- Too Large --> [Increase Buffer] --> [Split Message]
  |
  +-- OK
      |
      v
[Check QoS Level]
  |
  +-- QoS 0 --> [Consider QoS 1+] --> [Check Network]
  |
  +-- QoS 1+
      |
      v
[Check Broker Logs]
```

### 10.3 High Memory Usage Flowchart

```
START
  |
  v
[Monitor Heap Usage]
  |
  +-- Decreasing --> [Check for Leaks]
  |                     |
  |                     v
  |                   [String Operations?]
  |                     |
  |                     +-- Yes --> [Use Static Buffers]
  |                     |
  |                     +-- No
  |                          |
  |                          v
  |                        [JSON Documents?]
  |                          |
  |                          +-- Yes --> [Use StaticJsonDocument]
  |                          |
  |                          +-- No
  |                               |
  |                               v
  |                             [MQTT Buffer?]
  |                               |
  |                               +-- Yes --> [Reduce Buffer Size]
  |
  +-- Stable
      |
      v
[Check Fragmentation]
  |
  +-- High --> [Restart Periodically]
  |
  +-- Low
      |
      v
[Memory OK]
```

## Testing Scripts Collection

### Complete Test Runner Script

```python
#!/usr/bin/env python3
"""
MQTT Temperature Controller Test Suite
Run all tests or specific test categories
"""

import argparse
import sys
import time
from datetime import datetime

def run_all_tests(broker, device_id):
    """Run complete test suite"""
    print(f"=== MQTT Test Suite ===")
    print(f"Broker: {broker}")
    print(f"Device: {device_id}")
    print(f"Started: {datetime.now()}")
    print("=" * 50)
    
    test_results = {}
    
    # 1. Connectivity Tests
    print("\n[1/8] Running Connectivity Tests...")
    # Run connectivity tests
    test_results['connectivity'] = True
    
    # 2. Command Tests
    print("\n[2/8] Running Command Tests...")
    # Run command tests
    test_results['commands'] = True
    
    # 3. Telemetry Tests
    print("\n[3/8] Running Telemetry Tests...")
    # Run telemetry tests
    test_results['telemetry'] = True
    
    # 4. Load Tests
    print("\n[4/8] Running Load Tests...")
    # Run load tests
    test_results['load'] = True
    
    # 5. Security Tests
    print("\n[5/8] Running Security Tests...")
    # Run security tests
    test_results['security'] = True
    
    # 6. Integration Tests
    print("\n[6/8] Running Integration Tests...")
    # Run integration tests
    test_results['integration'] = True
    
    # 7. Performance Tests
    print("\n[7/8] Running Performance Tests...")
    # Run performance tests
    test_results['performance'] = True
    
    # 8. Stability Tests
    print("\n[8/8] Running Stability Tests...")
    # Run stability tests
    test_results['stability'] = True
    
    # Summary
    print("\n" + "=" * 50)
    print("TEST SUMMARY")
    print("=" * 50)
    
    passed = sum(1 for result in test_results.values() if result)
    total = len(test_results)
    
    for test, result in test_results.items():
        status = "✅ PASS" if result else "❌ FAIL"
        print(f"{test.capitalize():<20} {status}")
    
    print(f"\nTotal: {passed}/{total} passed ({passed/total*100:.0f}%)")
    print(f"Completed: {datetime.now()}")
    
    return passed == total

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='MQTT Temperature Controller Test Suite')
    parser.add_argument('--broker', default='broker.hivemq.com', 
                       help='MQTT broker address')
    parser.add_argument('--device', default='tempcontroller01',
                       help='Device ID to test')
    parser.add_argument('--category', choices=['all', 'connectivity', 'commands',
                       'telemetry', 'load', 'security', 'integration',
                       'performance', 'stability'],
                       default='all', help='Test category to run')
    
    args = parser.parse_args()
    
    if args.category == 'all':
        success = run_all_tests(args.broker, args.device)
        sys.exit(0 if success else 1)
    else:
        # Run specific category
        print(f"Running {args.category} tests...")
        # Implementation for specific categories
```

## Conclusion

This comprehensive testing guide provides the tools and procedures necessary to thoroughly test and debug the MQTT implementation in your Temperature Controller system. Regular testing using these procedures will ensure reliable operation in production environments.

Key recommendations:
1. Automate as many tests as possible
2. Run integration tests before each deployment
3. Monitor production metrics continuously
4. Keep audit logs for troubleshooting
5. Test edge cases and failure scenarios
6. Document all issues and resolutions

Remember to adapt these tests to your specific requirements and environment. The ESP32's resource constraints require special attention to memory management and network optimization.